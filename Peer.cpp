#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <ctime>
#include <iostream>
#include <assert.h>
#include <memory>
#include <thread>
#include <mutex>
#include <intrin.h>

#include "Filesystem.h"
#include "StringFacility.h"
#include "BinaryString.h"
#include "Timer.h"

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

		m_Thread = std::make_unique<std::thread>(std::bind(&Peer::RunThread, this));
	}

	void Peer::RunThread()
	{
		{
			std::unique_lock<std::mutex> _(m_Mutex);

			m_Socket = std::make_unique<Socket::Socket>();
			m_Socket->Open(m_IP, xs("%", m_Port));

			if (!m_Socket->IsOpen())
			{
				std::cout << xs("failed to connect to peer %:%", m_IP, m_Port) << std::endl;
				return;
			}

			std::cout << xs("[%] connected to peer %:%", m_IP, m_IP, m_Port) << std::endl;

			SendHandshake();
			ReceiveHandshake();

			m_LastMessageTime = Timer::GetTime();
		}

		for (;;)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));

			std::unique_lock<std::mutex> _(m_Mutex);

			if (m_Socket == nullptr)
			{
				if (m_HasCurrentPiece)
				{
					m_Client->UnmapPiece(m_ID, m_CurrentPiece);
				}

				m_IsConnected = false;
				return;
			}
			
			if (!m_HasCurrentPiece)
			{
				for (auto i = 0u; i < m_PieceAvailability.size(); i++)
				{
					if (m_PieceAvailability[i] == true && m_Client->MapPiece(m_ID, i))
					{
						m_HasCurrentPiece = true;
						m_CurrentPiece = i;
						SendInterested();
						break;
					}
				}
			}

			if (m_LastMessageTime + 4.0 < Timer::GetTime())
			{
				m_Socket = nullptr;

				if (m_HasCurrentPiece)
				{
					m_Client->UnmapPiece(m_ID, m_CurrentPiece);
				}

				m_IsConnected = false;

				std::cout << "[" << m_IP << "] timed out" << std::endl;
				return;
			}

			ReceiveMessage();

			if (m_Choked)
			{
				continue;
			}

			if (!m_CurrentPieceRequested && m_HasCurrentPiece)
			{
				if (m_CurrentPieceData.size() == m_PieceLength)
				{
					std::cout << "[" << m_IP << "] piece finished" << std::endl;

					m_Client->SubmitData(m_CurrentPiece, 0, m_CurrentPieceData);

					m_CurrentPieceData.clear();
					m_HasCurrentPiece = false;

					for (auto i = 0u; i < m_PieceAvailability.size(); i++)
					{
						if (m_PieceAvailability[i] == true && m_Client->MapPiece(m_ID, i))
						{
							m_HasCurrentPiece = true;
							m_CurrentPiece = i;
							break;
						}
					}
				}

				Wire::SendRequest(m_Socket, m_CurrentPiece, m_CurrentPieceData.size(), m_PieceLength < 32768 ? m_PieceLength : 32768);
				m_CurrentPieceRequested = true;
			}
		}
	}

	void Peer::SendHandshake()
	{
		Wire::SendHandshake(m_Socket, m_Client->m_InfoHash, m_Client->m_PeerID);
	}

	void Peer::SendInterested()
	{
		Wire::SendInterested(m_Socket);
	}

	void Peer::ReceiveHandshake()
	{
		std::string infoHash;
		std::string peerId;

		try
		{
			Wire::ReceiveHandshake(m_Socket, infoHash, peerId);
		}
		catch (const std::exception& e)
		{
			m_Socket = nullptr;
			return;
		}

		if (m_Client->m_InfoHash != infoHash)
		{
			std::cout << "wrong info_hash, bad handshake" << std::endl;
			m_Socket = nullptr;
			return;
		}

		std::cout << "handshake complete, got peer id: " << peerId << std::endl;
	}

	void Peer::ReceiveMessage()
	{
		unsigned int len = 0;
		Wire::PeerMessageID id;
		
		try
		{
			id = Wire::ReceiveMessageHeader(m_Socket, len);
		}
		catch (const std::exception& e)
		{
			m_Socket = nullptr;
			return;
		}

		switch (id)
		{
		case Wire::PeerMessageID::Choke:
			OnChoke();
			break;
		case Wire::PeerMessageID::Unchoke:
			OnUnchoke();
			break;
		case Wire::PeerMessageID::Interested:
			OnInterested();
			break;
		case Wire::PeerMessageID::NotInterested:
			OnNotInterested();
			break;
		case Wire::PeerMessageID::Have:
			OnHave(Wire::ReceiveHaveMessage(m_Socket));
			break;
		case Wire::PeerMessageID::Bitfield:
			OnBitfield(Wire::ReceiveBitfieldMessage(m_Socket, len));
			break;
		case Wire::PeerMessageID::Request:
			OnRequest();
			break;
		case Wire::PeerMessageID::Piece:
			OnPiece(Wire::ReceivePiece(m_Socket, len));
			break;
		case Wire::PeerMessageID::Cancel:
			OnCancel();
			break;
		case Wire::PeerMessageID::Port:
			OnPort();
			break;
		default:
			int x = 0;
			break;
		}

		m_LastMessageTime = Timer::GetTime(); 
	}

	void Peer::OnKeepAlive()
	{
	}

	void Peer::OnChoke()
	{
		m_Choked = true;
	}

	void Peer::OnUnchoke()
	{
		m_Choked = false;
	}

	void Peer::OnInterested()
	{
		m_Interested = true;
	}

	void Peer::OnNotInterested()
	{
		m_Interested = false;
	}

	void Peer::OnHave(size_t pieceIndex)
	{
		if (pieceIndex > m_PieceAvailability.size())
		{
			std::cout << "invalid piece index: " << pieceIndex << std::endl;
			return;
		}

		m_PieceAvailability[pieceIndex] = true;
	}

	void Peer::OnBitfield(const std::vector<bool>& bitfield)
	{
		if (bitfield.size() < m_PieceAvailability.size())
		{
			std::cout << "bad bitfield" << std::endl;
			m_Socket = nullptr;
			return;
		}

		for (auto i = 0u; i < m_PieceAvailability.size(); i++)
		{
			if (bitfield[i] == true)
			{
				m_PieceAvailability[i] = true;
			}
		}
	}

	void Peer::OnRequest()
	{
		unsigned int index;
		m_Socket->Receive(index);
		index = ByteSwap(index);

		unsigned int begin;
		m_Socket->Receive(begin);
		begin = ByteSwap(begin);

		unsigned int len;
		m_Socket->Receive(len);
		len = ByteSwap(len);

		std::cout << "request" << std::endl;
	}

	void Peer::OnPiece(const Wire::PieceBlock& block)
	{
		for (auto c : block.data)
		{
			m_CurrentPieceData.push_back(c);
		}
		m_CurrentPieceRequested = false;
	}

	void Peer::OnCancel()
	{
		std::vector<char> payload;
		m_Socket->Receive(payload, 12);
		std::cout << "cancel" << std::endl;
	}

	void Peer::OnPort()
	{
		std::vector<char> payload;
		m_Socket->Receive(payload, 2);
		std::cout << "port" << std::endl;
	}

}