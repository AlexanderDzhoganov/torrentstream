#ifndef __TORRENTSTREAM_ASIO_PEERCOMM_H
#define __TORRENTSTREAM_ASIO_PEERCOMM_H

namespace TorrentStream
{

	namespace ASIO
	{

		enum class PeerCommState
		{
			Offline = 0,
			Connecting,
			Choked,
			Working,
			Error
		};

		class PeerComm
		{

			public:
			PeerComm(const std::string& ip, const std::string& port, const std::string& infoHash, const std::string& selfPeerID);

			PeerCommState GetState();

			void Connect();
			void Disconnect();

			std::unique_ptr<Peer::Message> PopIncomingMessage();
			void PushOutgoingMessage(std::unique_ptr<Peer::Message> message);

			void DispatchAllMessages();

			private:
			void HandleResolve(const boost::system::error_code& error, boost::asio::ip::tcp::resolver::iterator endpoint);
			void HandleConnect(const boost::system::error_code& error);
			void HandleHandshake(std::shared_ptr<std::array<char, 68>> buffer, const boost::system::error_code& error);

			void QueueDispatchSingleMessage();
			void HandleSendMessage(std::shared_ptr<Peer::Message> message, const boost::system::error_code& error);
				
			void QueueReceiveMessage();
			void HandleMessageLength(std::shared_ptr<uint32_t> length, const boost::system::error_code& error);
			void HandleMessagePayload(std::shared_ptr<std::vector<char>> payload, const boost::system::error_code& error);

			std::mutex m_Mutex;

			std::string m_PeerID;
			std::string m_SelfPeerID;
			std::string m_InfoHash;

			bool m_DispatchActive = false;
			bool m_ReceiveActive = false;

			std::unique_ptr<boost::asio::ip::tcp::socket> m_Socket = nullptr;
			std::unique_ptr<boost::asio::ip::tcp::resolver> m_Resolver = nullptr;
			std::unique_ptr<boost::asio::strand> m_Strand = nullptr;

			std::string m_IP;
			std::string m_Port;

			PeerCommState m_State = PeerCommState::Offline;
			std::deque<std::unique_ptr<Peer::Message>> m_Incoming;
			std::deque<std::unique_ptr<Peer::Message>> m_Outgoing;

		};

	}

}

#endif
