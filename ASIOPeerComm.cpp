#include <thread>
#include <memory>
#include <mutex>
#include <vector>
#include <deque>
#include <iostream>
#include <string>
#include <sstream>

#include "StringFacility.h"
#include "Log.h"

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "ASIOThreadPool.h"
#include "ASIOPeerMessage.h"
#include "ASIOPeerComm.h"

namespace TorrentStream
{

	namespace ASIO
	{

		PeerComm::PeerComm(const std::string& ip, const std::string& port, const std::string& infoHash, const std::string& selfPeerID) :
			m_IP(ip), m_Port(port), m_InfoHash(infoHash), m_SelfPeerID(selfPeerID)
		{
		}
		
		PeerCommState PeerComm::GetState()
		{
			PeerCommState result;

			{
				std::unique_lock<std::mutex> _(m_Mutex);
				result = m_State;
			}

			return result;
		}

		void PeerComm::Connect()
		{
			using boost::asio::ip::tcp;
			std::unique_lock<std::mutex> _(m_Mutex);
			
			if (m_State != PeerCommState::Offline)
			{
				std::cout << "PeerComm::Connect() error: Peer already online" << std::endl;
				return;
			}

			m_State = PeerCommState::Connecting;

			auto& service = ASIO::ThreadPool::Instance().GetService();
			m_Socket = std::make_unique<tcp::socket>(service);
			m_Resolver = std::make_unique<tcp::resolver>(service);
			m_Strand = std::make_unique<boost::asio::strand>(ThreadPool::Instance().GetService());

			tcp::resolver::query query(m_IP, m_Port);
			m_Resolver->async_resolve(query, [&](const boost::system::error_code& error, boost::asio::ip::tcp::resolver::iterator endpoint)
			{
				if (!error)
				{
					tcp::endpoint endpt = *endpoint;
					m_Socket->async_connect
					(
						endpt,
						m_Strand->wrap(boost::bind(&PeerComm::HandleConnect, this, boost::asio::placeholders::error))
					);
				}
				else
				{
					m_State = PeerCommState::Error;
					std::cout << "async_resolve error: " << error << std::endl;
				}
			});
		}

		void PeerComm::Disconnect()
		{

		}

		std::unique_ptr<Peer::Message> PeerComm::PopIncomingMessage()
		{
			std::unique_lock<std::mutex> _(m_Mutex);
			if (m_Incoming.size() == 0)
			{
				return nullptr;
			}

			auto message = std::move(m_Incoming.front());
			m_Incoming.pop_front();
			return message;
		}

		void PeerComm::PushOutgoingMessage(std::unique_ptr<Peer::Message> message)
		{
			std::unique_lock<std::mutex> _(m_Mutex);
			m_Outgoing.push_back(std::move(message));
		}

		void PeerComm::DispatchAllMessages()
		{
			std::unique_lock<std::mutex> _(m_Mutex);

			if (!m_DispatchActive)
			{
				QueueDispatchSingleMessage();
			}
		}

		void PeerComm::HandleConnect(const boost::system::error_code& error)
		{
			std::unique_lock<std::mutex> _(m_Mutex);

			if (!error)
			{
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

				request_stream << m_InfoHash;
				request_stream << m_SelfPeerID;

				boost::asio::write(*m_Socket, request);

				auto handshakeBuffer = std::make_shared<std::array<char, 68>>();

				boost::asio::async_read
				(
					*m_Socket,
					boost::asio::buffer(*handshakeBuffer),
					m_Strand->wrap(boost::bind(&PeerComm::HandleHandshake, this, handshakeBuffer, boost::asio::placeholders::error))
				);
			}
			else
			{
				m_State = PeerCommState::Error;
			}
		}

		void PeerComm::HandleHandshake(std::shared_ptr<std::array<char, 68>> buffer, const boost::system::error_code& error)
		{
			std::unique_lock<std::mutex> _(m_Mutex);

			if ((*buffer)[0] != 19)
			{
				m_State = PeerCommState::Error;
				return;
			}
			std::string protocolId(buffer->data() + 1, 19);
			if (protocolId != "BitTorrent protocol")
			{
				m_State = PeerCommState::Error;
				return;
			}

			std::string infoHash(buffer->data() + 28, 20);
			if (infoHash != m_InfoHash)
			{
				std::cout << "bad info_hash" << std::endl;
				m_State = PeerCommState::Error;
				return;
			}

			std::string peerId(buffer->data() + 48, 20);
			m_PeerID = peerId;

			LOG(xs("connected to peer %", m_IP));

			m_State = PeerCommState::Choked;
			QueueReceiveMessage();
		}

		void PeerComm::QueueDispatchSingleMessage()
		{
			if (m_Outgoing.size() == 0)
			{
				m_DispatchActive = false;
				return;
			}

			auto message = std::shared_ptr<Peer::Message>(std::move(m_Outgoing.front()));
			m_Outgoing.pop_front();

			if (message->GetType() == Peer::MessageType::Request && m_State == PeerCommState::Choked)
			{
				LOG("cannot send request when choked");
				QueueDispatchSingleMessage();
				return;
			}

			uint32_t messageLength = 1 + message->GetPayload().size();
			messageLength = _byteswap_ulong(messageLength);

			message->m_Header.resize(5);
			memcpy(message->m_Header.data(), &messageLength, 4);
			message->m_Header[4] = (char)message->GetType();

			std::vector<boost::asio::const_buffer> buffers;
			buffers.push_back(boost::asio::buffer(message->m_Header.data(), 5));
			buffers.push_back(boost::asio::buffer(message->GetPayload().data(), message->GetPayload().size()));

			boost::asio::async_write
			(
				*m_Socket,
				buffers,
				m_Strand->wrap(boost::bind(&PeerComm::HandleSendMessage, this, message, boost::asio::placeholders::error))
			);
		}

		void PeerComm::HandleSendMessage(std::shared_ptr<Peer::Message> message, const boost::system::error_code& error)
		{
			if (!error)
			{
				std::unique_lock<std::mutex> _(m_Mutex);
				QueueDispatchSingleMessage();
			}
			else
			{
				std::unique_lock<std::mutex> _(m_Mutex);
				std::cout << "HandleSendMessage() error: " << error << std::endl;
				m_State = PeerCommState::Error;
			}
		}

		void PeerComm::QueueReceiveMessage()
		{
			auto length = std::make_shared<uint32_t>();

			boost::asio::async_read
			(
				*m_Socket,
				boost::asio::buffer(length.get(), sizeof(uint32_t)),
				m_Strand->wrap(boost::bind(&PeerComm::HandleMessageLength, this, length, boost::asio::placeholders::error))
			);
		}

		void PeerComm::HandleMessageLength(std::shared_ptr<uint32_t> length_, const boost::system::error_code& error)
		{
			if (!error)
			{
				auto length = _byteswap_ulong(*length_);

				if (length == 0)
				{
					std::unique_lock<std::mutex> _(m_Mutex);
					m_Incoming.push_back(std::make_unique<Peer::KeepAlive>());
					return;
				}

				auto buffer = std::make_shared<std::vector<char>>();
				buffer->resize(length);

				boost::asio::async_read
				(
					*m_Socket,
					boost::asio::buffer(buffer->data(), length),
					m_Strand->wrap(boost::bind(&PeerComm::HandleMessagePayload, this, buffer, boost::asio::placeholders::error))
				);
			}
			else
			{
				std::unique_lock<std::mutex> _(m_Mutex);
				m_State = PeerCommState::Error;
			}
		}
		
		void PeerComm::HandleMessagePayload(std::shared_ptr<std::vector<char>> payload_, const boost::system::error_code& error)
		{
			if (!error)
			{
				QueueReceiveMessage();

				auto id = (*payload_)[0];
					
				std::vector<char> payload;
				payload.reserve(payload_->size() - 1);
				payload.insert(payload.end(), payload_->begin() + 1, payload_->end());

				std::unique_lock<std::mutex> _(m_Mutex);

				switch ((Peer::MessageType)id)
				{
				case Peer::MessageType::Choke:
					m_State = PeerCommState::Choked;
					m_Incoming.push_back(std::make_unique<Peer::Choke>());
					break;
				case Peer::MessageType::Unchoke:
					m_State = PeerCommState::Working;
					m_Incoming.push_back(std::make_unique<Peer::Unchoke>());
					break;
				case Peer::MessageType::Interested:
					m_Incoming.push_back(std::make_unique<Peer::Interested>());
					break;
				case Peer::MessageType::NotInterested:
					m_Incoming.push_back(std::make_unique<Peer::NotInterested>());
					break;
				case Peer::MessageType::Have:
					m_Incoming.push_back(std::make_unique<Peer::Have>(payload));
					break;
				case Peer::MessageType::Bitfield:
					m_Incoming.push_back(std::make_unique<Peer::Bitfield>(payload));
					break;
				case Peer::MessageType::Request:
					m_Incoming.push_back(std::make_unique<Peer::Request>(payload));
					break;
				case Peer::MessageType::Piece:
					m_Incoming.push_back(std::make_unique<Peer::Piece>(payload));
					break;
				case Peer::MessageType::Cancel:
					m_Incoming.push_back(std::make_unique<Peer::Cancel>(payload));
					break;
				case Peer::MessageType::Port:
					m_Incoming.push_back(std::make_unique<Peer::Port>(payload));
					break;
				default:
					std::cout << "invalid message id" << std::endl;
					m_State = PeerCommState::Error;
					return;
				}
			}
			else
			{
				std::unique_lock<std::mutex> _(m_Mutex);
				m_State = PeerCommState::Error;
			}
		}

	}

}
