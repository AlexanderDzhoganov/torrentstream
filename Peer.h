#ifndef __TORRENTSTREAM_PEER_H
#define __TORRENTSTREAM_PEER_H

namespace TorrentStream
{

	class Client;

	class Peer
	{

		public:
		static const size_t RequestSize = 16 * 1024;

		Peer(const std::string& ip, int port, const std::string& id, Client* client);

		~Peer()
		{
		}

		const std::string& GetID() const
		{
			return m_ID;
		}

		const std::string& GetIP() const
		{
			return m_IP;
		}

		ASIO::PeerCommState GetCommState() const
		{
			return m_Comm->GetState();
		}

		void Connect();

		void Update();

		bool IsDownloading()
		{
			return m_Downloading;
		}

		void StartDownload(size_t pieceIndex);

		const std::vector<bool>& GetAvailablePieces()
		{
			return m_HasPieces;
		}

		void StartTimer()
		{
			m_TimerStart = Timer::GetTime();
		}

		double GetElapsedTime()
		{
			return Timer::GetTime() - m_TimerStart;
		}

		size_t GetCurrentPieceIndex()
		{
			return m_PieceIndex;
		}

		void Taint()
		{
			m_Tainted = true;
		}

		bool IsTainted()
		{
			return m_Tainted;
		}

		private:
		std::string m_IP;

		Client* m_Client;
		std::string m_ID;

		bool m_Tainted = false;

		double m_TimerStart = 0.0;

		bool m_Downloading = false;
		size_t m_PieceIndex = 0;
		std::deque<size_t> m_PieceOffsetRequests;
		std::deque<size_t> m_PieceOffsetRequestsInFlight;
		std::unique_ptr<Piece> m_PieceData = nullptr;

		std::unique_ptr<ASIO::PeerComm> m_Comm = nullptr;

		std::vector<bool> m_HasPieces;

	};

}

#endif
