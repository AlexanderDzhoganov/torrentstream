#ifndef __TORRENTSTREAM_PEER_H
#define __TORRENTSTREAM_PEER_H

namespace TorrentStream
{

	class Client;

	class Peer
	{

		public:
		static const size_t RequestSize = 16 * 1024;

		Peer
		(
			const std::string& ip,
			int port,
			const std::string& id,
			size_t pieceCount,
			size_t pieceLength,
			const std::string& infoHash,
			const std::string& selfId
		);

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

		bool IsFinished()
		{
			return m_Finished;
		}

		bool IsTainted()
		{
			return m_Tainted;
		}

		void Taint()
		{
			m_Tainted = true;
		}

		std::unique_ptr<Piece> GetPieceData()
		{
			auto data = std::move(m_PieceData);
			m_PieceData = nullptr;
			return data;
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
		std::string m_ID;

		bool m_Tainted = false;

		bool m_Downloading = false;
		bool m_Finished = false;
		size_t m_PieceIndex = 0;

		size_t m_PieceCount = 0;
		size_t m_PieceLength = 0;

		std::deque<size_t> m_Requests;
		std::deque<size_t> m_InFlight;

		std::unique_ptr<Piece> m_PieceData = nullptr;

		std::unique_ptr<ASIO::PeerComm> m_Comm = nullptr;
		std::vector<bool> m_HasPieces;

	};

}

#endif
