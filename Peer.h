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

		size_t GetCurrentPieceIndex()
		{
			return m_PieceIndex;
		}

		bool IsSeed()
		{
			for (auto&& piece : m_HasPieces)
			{
				if (!piece)
				{
					return false;
				}
			}

			return true;
		}

		size_t GetAverageBandwidth()
		{
			return m_BandwidthTracker.CalculateBandwidth();
		}

		private:
		BandwidthTracker m_BandwidthTracker;

		std::string m_IP;

		Client* m_Client;
		std::string m_ID;

		bool m_Downloading = false;
		size_t m_PieceIndex = 0;

		std::deque<size_t> m_Requests;
		std::deque<size_t> m_InFlight;

		std::unique_ptr<Piece> m_PieceData = nullptr;

		std::unique_ptr<ASIO::PeerComm> m_Comm = nullptr;
		std::vector<bool> m_HasPieces;

	};

}

#endif
