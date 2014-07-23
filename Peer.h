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
		Peer(const std::string& ip, int port, const std::string& id, Client* client);

		~Peer()
		{
			std::unique_lock<std::mutex> _(m_Mutex);

			if (m_Thread != nullptr)
			{
				m_Thread->join();
			}
		}

		bool IsConnected()
		{
			return m_IsConnected;
		}

		const std::string& GetID()
		{
			return m_ID;
		}

		bool IsChoked()
		{
			return m_Choked;
		}

		bool IsDownloading()
		{
			return m_HasCurrentPiece && m_CurrentPieceRequested;
		}

		private:
		void RunThread();

		void SendHandshake();
		void SendInterested();

		void ReceiveHandshake();
		void ReceiveMessage();

		void OnKeepAlive();
		void OnChoke();
		void OnUnchoke();
		void OnInterested();
		void OnNotInterested();
		void OnHave(size_t pieceIndex);
		void OnBitfield(const std::vector<bool>& bitfield);
		void OnRequest();
		void OnPiece(const Wire::PieceBlock& block);
		void OnCancel();
		void OnPort();

		Client* m_Client;

		std::string m_IP;
		int m_Port;
		std::string m_ID;

		bool m_Choked = true;
		bool m_Interested = false;

		bool m_IsConnected = true;

		std::vector<bool> m_PieceAvailability;

		size_t m_PieceLength = 0;
		std::vector<char> m_CurrentPieceData;
		size_t m_CurrentPiece = 0;
		bool m_HasCurrentPiece = false;
		bool m_CurrentPieceRequested = false;

		bool m_ExpectResponse = false;

		double m_LastMessageTime = 0.0;

		std::unique_ptr<Socket::Socket> m_Socket = nullptr;
		std::unique_ptr<std::thread> m_Thread = nullptr;
		std::mutex m_Mutex;

	};

}

#endif
