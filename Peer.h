#ifndef __TORRENTSTREAM_PEER_H
#define __TORRENTSTREAM_PEER_H

namespace TorrentStream
{

	class Client;

	class Peer
	{

		public:
		Peer(const std::string& ip, int port, const std::string& id, Client* client);

		~Peer()
		{
		}

		const std::string& GetID() const
		{
			return m_ID;
		}

		ASIO::PeerCommState GetCommState() const
		{
			return m_Comm->GetState();
		}

		void Connect();

		void Update();

		private:
		Client* m_Client;
		std::string m_ID;

		std::unique_ptr<ASIO::PeerComm> m_Comm = nullptr;

	};

}

#endif
