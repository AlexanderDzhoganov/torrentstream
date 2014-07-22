#ifndef __TORRENTSTREAM_PEER_H
#define __TORRENTSTREAM_PEER_H

namespace TorrentStream
{

	struct PeerMessage
	{
		int length;
		char id;
		std::vector<char> payload;
	};

	class Client;

	class Peer
	{

		public:
		Peer(const std::string& ip, int port, const std::string& id, Client* client) : m_IP(ip), m_Port(port), m_ID(id), m_Client(client)
		{
			m_Thread = std::make_unique<std::thread>(std::bind(&Peer::RunThread, this));
		}

		~Peer()
		{
			if (m_Thread != nullptr)
			{
				m_Thread->join();
			}
		}

		bool IsConnected()
		{
			if (m_Socket == nullptr)
			{
				return false;
			}

			return m_Socket->IsOpen();
		}

		private:
		void RunThread();

		void SendHandshake();
		void ReceiveHandshake();

		void ReceiveMessage();

		Client* m_Client;

		std::string m_IP;
		int m_Port;
		std::string m_ID;

		bool m_PeerChoked = true;
		bool m_PeerInterested = false;

		bool m_Choked = true;
		bool m_Interested = false;

		std::unique_ptr<Socket::Socket> m_Socket = nullptr;
		std::unique_ptr<std::thread> m_Thread = nullptr;
		std::mutex m_Mutex;

	};

}

#endif
