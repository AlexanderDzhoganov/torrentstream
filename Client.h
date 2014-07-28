#ifndef __TORRENTSTREAM_CLIENT_H
#define __TORRENTSTREAM_CLIENT_H

namespace TorrentStream
{

	class Client
	{
		friend class Peer;

		public:
		Client(const std::shared_ptr<MetadataFile>& metadata, const std::string& rootPath, size_t fileToPlay);

		~Client() {}

		void Start();

		void Stop();

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
		void Initialize();

		std::shared_ptr<MetadataFile> m_Metadata;
		std::string m_RootPath;

		std::unique_ptr<Tracker> m_Tracker;
		std::unique_ptr<Overwatch> m_Overwatch;

		std::string m_AnnounceURL;
		std::string m_InfoHash;
		std::string m_PeerID;
		std::string m_Port = "6881";

		std::unique_ptr<HTTP::HTTPServer> m_HTTPServer;

		std::unique_ptr<File> m_File;
		size_t m_FileToPlay = 0;

		std::vector<Piece> m_Pieces;
		uint64_t m_PieceLength = 0;
		uint64_t m_PieceCount = 0;

	};

}

#endif
