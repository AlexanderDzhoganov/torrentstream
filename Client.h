#ifndef __TORRENTSTREAM_CLIENT_H
#define __TORRENTSTREAM_CLIENT_H

namespace TorrentStream
{

	class Client
	{
		friend class Peer;

		public:
		Client(const std::shared_ptr<MetadataFile>& metadata, const std::string& rootPath);

		~Client() {}

		void Start();

		void Stop();

		void StartTracker();

		void UpdateTracker();

		size_t GetPiecesCount() const
		{
			return m_PieceCount;
		}

		size_t GetPieceLength() const
		{
			return m_PieceLength;
		}
		/*
		size_t CountFreshPeers(PeerState state) const
		{
			size_t count = 0;
			for (auto& peer : m_Fresh) if (peer.second->GetState() == state) count++;
			return count;
		}

		size_t CountWarmUpPeers(PeerState state) const
		{
			size_t count = 0;
			for (auto& peer : m_WarmUp) if (peer.second->GetState() == state) count++;
			return count;
		}

		size_t CountHotPeers(PeerState state) const
		{
			size_t count = 0;
			for (auto& peer : m_Hot) if (peer.second->GetState() == state) count++;
			return count;
		}

		size_t CountColdPeers(PeerState state) const
		{
			size_t count = 0;
			for (auto& peer : m_Cold) if (peer.second->GetState() == state) count++;
			return count;
		}
		*/

		private:
		void CleanUpPeers();

		std::shared_ptr<MetadataFile> m_Metadata;
		std::string m_RootPath;

		std::string m_AnnounceURL;
		std::string m_InfoHash;
		std::string m_PeerID;
		std::string m_Port = "6881";

		std::vector<std::unique_ptr<File>> m_Files;

		std::vector<Piece> m_Pieces;
		size_t m_PieceLength = 0;
		size_t m_PieceCount = 0;

		std::unordered_map<std::string, bool> m_Known;
		std::unordered_map<std::string, std::unique_ptr<Peer>> m_Fresh;
		std::unordered_map<std::string, std::unique_ptr<Peer>> m_WarmUp;
		std::unordered_map<std::string, std::unique_ptr<Peer>> m_Cold;
		std::unordered_map<std::string, std::unique_ptr<Peer>> m_Hot;

	};

}

#endif
