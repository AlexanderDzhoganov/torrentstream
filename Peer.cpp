#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <ctime>
#include <deque>
#include <set>
#include <iostream>
#include <assert.h>
#include <memory>
#include <thread>
#include <mutex>
#include <intrin.h>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include "ASIOThreadPool.h"

#include "Filesystem.h"
#include "StringFacility.h"
#include "BinaryString.h"
#include "Timer.h"
#include "BandwidthTracker.h"

#include "Socket.h"
#include "Wire.h"
#include "HTTP.h"
#include "BencodeTokenizer.h"
#include "BencodeParser.h"
#include "MetadataFile.h"
#include "Peer.h"
#include "Piece.h"
#include "File.h"
#include "Client.h"

namespace TorrentStream
{

	Peer::Peer(const std::string& ip, int port, const std::string& id, Client* client) :
		m_IP(ip), m_Port(port), m_ID(id), m_Client(client), m_PieceLength(m_Client->GetPieceLength())
	{
		auto piecesCount = client->GetPiecesCount();
		m_PieceAvailability.resize(piecesCount);
	}

	void Peer::Connect()
	{
		using boost::asio::ip::tcp;

		m_State = PeerState::Waiting;

		auto& service = ASIO::ThreadPool::Instance().GetService();
		m_Socket = std::make_unique<tcp::socket>(service);
		m_Resolver = std::make_unique<tcp::resolver>(service);

		tcp::resolver::query query(m_IP, xs("%", m_Port));
		m_Resolver->async_resolve(query, [&](const boost::system::error_code& error, boost::asio::ip::tcp::resolver::iterator endpoint)
		{
			if (!error)
			{
				tcp::endpoint endpt = *endpoint;
				m_Socket->async_connect(endpt, boost::bind(&Peer::HandleConnect, this, boost::asio::placeholders::error));
			}
			else
			{
				m_State = PeerState::Error;
				std::cout << "async_resolve error: " << error << std::endl;
			}
		});
	}

	void Peer::WarmUp()
	{
		m_State = PeerState::Downloading;
		m_WarmUp = true;
		HandleSendInterested();
	}

	void Peer::RequestPiece(size_t index)
	{
		m_CurrentPiece = index;
		m_CurrentPieceOffset = 0;

		for (auto i = 0u; i < 5; i++)
		{
			HandleSendRequest(m_CurrentPiece, m_CurrentPieceOffset, m_RequestSize);

			m_CurrentPieceOffset += m_RequestSize;
			if (m_CurrentPieceOffset >= m_PieceLength)
			{
				break;
			}
		}
	}

	void Peer::HandleSendInterested()
	{
		boost::asio::streambuf request;
		std::ostream request_stream(&request);

		int one = _byteswap_ulong(1);
		request_stream.write((char*)&one, sizeof(int));
		request_stream << (char)PeerMessageType::Interested;

		boost::system::error_code error;
		boost::asio::write(*m_Socket, request, error);
	}

	void Peer::HandleSendNotInterested()
	{
		boost::asio::streambuf request;
		std::ostream request_stream(&request);

		int one = _byteswap_ulong(1);
		request_stream.write((char*)&one, sizeof(int));
		request_stream << (char)PeerMessageType::NotInterested;

		boost::system::error_code error;
		boost::asio::write(*m_Socket, request, error);
	}

	void Peer::HandleSendRequest(size_t index_, size_t offset_, size_t length_)
	{
		boost::asio::streambuf request;
		std::ostream request_stream(&request);

		int len = _byteswap_ulong(13);
		auto index = _byteswap_ulong(index_);
		auto offset = _byteswap_ulong(offset_);
		auto length = _byteswap_ulong(length_);

		request_stream.write((char*)&len, sizeof(int));
		request_stream << (char)PeerMessageType::Request;
		request_stream.write((char*)&index, sizeof(int));
		request_stream.write((char*)&offset, sizeof(int));
		request_stream.write((char*)&length, sizeof(int));

		boost::system::error_code error;
		boost::asio::write(*m_Socket, request, error);
	}

	void Peer::HandleConnect(const boost::system::error_code& error)
	{
		if (!error)
		{
			std::cout << "connected to peer " << m_IP << std::endl;

			boost::asio::streambuf request;
			std::ostream request_stream(&request);

			char protocolLength = 19;
			char zero = 0;
			request_stream << protocolLength;
			request_stream << "BitTorrent protocol";
			
			for (auto q = 0u; q < 8; q++)
			{
				request_stream << zero;
			}

			request_stream << m_Client->m_InfoHash;
			request_stream << m_Client->m_PeerID;

			boost::asio::write(*m_Socket, request);

			auto handshakeBuffer = std::make_shared<std::array<char, 68>>();
					
			boost::asio::async_read
			(
				*m_Socket,
				boost::asio::buffer(*handshakeBuffer),
				boost::bind(&Peer::HandleHandshake, this, handshakeBuffer, boost::asio::placeholders::error)
			);
		}
		else
		{
			m_State = PeerState::Error;
		}
	}

	void Peer::HandleHandshake(std::shared_ptr<std::array<char, 68>> buffer, const boost::system::error_code& error)
	{
		if ((*buffer)[0] != 19)
		{
			m_State = PeerState::Error;
			return;
		}
		std::string protocolId(buffer->data() + 1, 19);
		if (protocolId != "BitTorrent protocol")
		{
			m_State = PeerState::Error;
			return;
		}

		std::string infoHash(buffer->data() + 28, 20);
		if (infoHash != m_Client->m_InfoHash)
		{
			std::cout << "bad info_hash" << std::endl;
			m_State = PeerState::Error;
			return;
		}

		std::string peerId(buffer->data() + 48, 20);
		m_SelfReportedID = peerId;

		QueueReceiveMessage();

		m_State = PeerState::Idle;
	}

	void Peer::QueueReceiveMessage()
	{
		auto headerBuffer = std::make_shared<std::array<char, 4>>();

		boost::asio::async_read
		(
			*m_Socket,
			boost::asio::buffer(*headerBuffer),
			boost::bind(&Peer::HandleReceiveMessageHeader, this, headerBuffer, boost::asio::placeholders::error)
		);
	}

	void Peer::HandleReceiveMessageHeader(std::shared_ptr<std::array<char, 4>> buffer, const boost::system::error_code& error)
	{
		auto len = *((unsigned int*)buffer->data());
		len = _byteswap_ulong(len);

		auto idBuffer = std::make_shared<char>();

		boost::asio::async_read
		(
			*m_Socket,
			boost::asio::buffer(idBuffer.get(), sizeof(char)),
			boost::bind(&Peer::HandleReceiveMessageID, this, idBuffer, len, boost::asio::placeholders::error)
		);
	}

	void Peer::HandleReceiveMessageID(std::shared_ptr<char> id_, size_t len, const boost::system::error_code& error)
	{
		auto id = *id_;

		switch ((PeerMessageType)id)
		{
		case PeerMessageType::KeepAlive:
			QueueReceiveMessage();
			break;
		case PeerMessageType::Choke:
			m_State = PeerState::Choked;
			QueueReceiveMessage();
			break;
		case PeerMessageType::Unchoke:
			m_State = PeerState::Idle;

			if (m_WarmUp)
			{
				m_State = PeerState::Downloading;

				for (auto i = 0u; i < m_PieceAvailability.size(); i++)
				{
					if (!m_PieceAvailability[i])
					{
						continue;
					}

					m_BandwidthDown.Start();
					RequestPiece(i);
					break;
				}
			}

			QueueReceiveMessage();
			break;
		case PeerMessageType::Interested:
			m_Interested = true;
			QueueReceiveMessage();
			break;
		case PeerMessageType::NotInterested:
			m_Interested = false;
			QueueReceiveMessage();
			break;
		case PeerMessageType::Have:
			HandleReceiveHaveHeader();
			break;
		case PeerMessageType::Bitfield:
			HandleReceiveBitfieldHeader(len - 1);
			break;
		case PeerMessageType::Request:
			QueueReceiveMessage();
			break;
		case PeerMessageType::Piece:
			HandleReceivePieceHeader(len - 1);
			break;
		case PeerMessageType::Cancel:
			QueueReceiveMessage();
			break;
		case PeerMessageType::Port:
			QueueReceiveMessage();
			break;
		default:
			std::cout << "invalid msg id:" << (int)id << std::endl;
			break;
		}
	}

	void Peer::HandleReceiveHaveHeader()
	{
		auto haveBuffer = std::make_shared<int>();

		boost::asio::async_read
		(
			*m_Socket,
			boost::asio::buffer(haveBuffer.get(), sizeof(int)),
			boost::bind(&Peer::HandleReceiveHave, this, haveBuffer, boost::asio::placeholders::error)
		);
	}

	void Peer::HandleReceiveHave(std::shared_ptr<int> index, const boost::system::error_code& error)
	{
		auto idx = _byteswap_ulong(*index);
		m_PieceAvailability[idx] = true;

		QueueReceiveMessage();
	}

	void Peer::HandleReceiveBitfieldHeader(size_t len)
	{
		auto bitfieldBuffer = std::make_shared<std::vector<char>>(len);
		bitfieldBuffer->resize(len);

		boost::asio::async_read
		(
			*m_Socket,
			boost::asio::buffer(bitfieldBuffer->data(), bitfieldBuffer->size()),
			boost::bind(&Peer::HandleReceiveBitfield, this, bitfieldBuffer, boost::asio::placeholders::error)
		);
	}

	void Peer::HandleReceiveBitfield(std::shared_ptr<std::vector<char>> bitfield, const boost::system::error_code& error)
	{
		for (auto i = 0u; i < m_PieceAvailability.size(); i++)
		{
			if ((*bitfield)[i / 8] & (1 << ((8 - i % 8) - 1)))
			{
				m_PieceAvailability[i] = true;
			}
		}

		QueueReceiveMessage();
	}

	void Peer::HandleReceivePieceHeader(size_t len)
	{
		auto pieceBuffer = std::make_shared<std::vector<char>>(len);
		pieceBuffer->resize(len);

		boost::asio::async_read
		(
			*m_Socket,
			boost::asio::buffer(pieceBuffer->data(), pieceBuffer->size()),
			boost::bind(&Peer::HandleReceivePiece, this, pieceBuffer, boost::asio::placeholders::error)
		);
	}

	void Peer::HandleReceivePiece(std::shared_ptr<std::vector<char>> data, const boost::system::error_code& error)
	{
		m_BandwidthDown.AddPacket(data->size());
		QueueReceiveMessage();

		m_CurrentPieceOffset += m_RequestSize;
		if (m_CurrentPieceOffset < m_PieceLength)
		{
			HandleSendRequest(m_CurrentPiece, m_CurrentPieceOffset, m_RequestSize);
		}
		else
		{
			std::cout << "got piece: " << m_CurrentPiece << std::endl;

			if (m_WarmUp)
			{
				m_AverageBandwidth = m_BandwidthDown.GetAverageBandwidth();
				m_WarmUp = false;
				m_State = PeerState::Idle;
			}
		}
	}

}