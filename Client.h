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

		void Start(size_t fileToPlay);

		void Stop();

		void StartTracker();

		void UpdateTracker();

		uint64_t GetPiecesCount() const
		{
			return m_PieceCount;
		}

		uint64_t GetPieceLength() const
		{
			return m_PieceLength;
		}

		uint64_t GetTotalSize() const
		{
			return m_Metadata->GetTotalSize();
		}

		bool CheckPieceHash(size_t index, const std::vector<char>& expected);
		
		void WriteOutPiece(size_t index);

		private:
		void CleanUpPeers();

		std::shared_ptr<MetadataFile> m_Metadata;
		std::string m_RootPath;

		std::string m_AnnounceURL;
		std::string m_InfoHash;
		std::string m_PeerID;
		std::string m_Port = "6881";

		std::vector<std::unique_ptr<File>> m_Files;
		size_t m_FileToPlay = 0;

		std::vector<Piece> m_Pieces;
		uint64_t m_PieceLength = 0;
		uint64_t m_PieceCount = 0;

		std::unordered_map<std::string, bool> m_Known;
		std::unordered_map<std::string, std::unique_ptr<Peer>> m_Fresh;
		std::unordered_map<std::string, std::unique_ptr<Peer>> m_WarmUp;
		std::unordered_map<std::string, std::unique_ptr<Peer>> m_Cold;
		std::unordered_map<std::string, std::unique_ptr<Peer>> m_Hot;

	};

}

#endif
