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

		void UpdateTracker();

		size_t GetActivePeersCount()
		{
			size_t count = 0;

			for (auto& pair : m_Peers)
			{
				if (pair.second->IsConnected())
				{
					count++;
				}
			}

			return count;
		}

		size_t GetPiecesCount()
		{
			return m_PieceCount;
		}

		private:
		std::shared_ptr<MetadataFile> m_Metadata;
		std::string m_RootPath;

		std::string m_AnnounceURL;
		std::string m_InfoHash;
		std::string m_PeerID;
		std::string m_Port = "6881";

		std::vector<File> m_Files;
		std::vector<Piece> m_Pieces;
		size_t m_PieceLength = 0;
		size_t m_PieceCount = 0;
	
		std::unordered_map<std::string, std::unique_ptr<Peer>> m_Peers;
		size_t m_PeersComplete = 0;
		size_t m_PeersIncomplete = 0;

		size_t m_DownloadedBytes = 0;
		size_t m_UploadedBytes = 0;

	};

}

#endif
