#ifndef __TORRENTSTREAM_METADATAFILE_H
#define __TORRENTSTREAM_METADATAFILE_H

namespace TorrentStream
{

	class InvalidInfoSection : public std::exception {};
	
	class MetadataFile
	{

		public:
		MetadataFile(const std::vector<char>& contents);

		std::vector<char> Encode()
		{
			return m_Contents->Encode();
		}

		std::string GetInfoHash();

		std::string GetAnnounceURL();

		size_t GetPieceLength();

		size_t GetPieceCount();

		size_t GetTotalSize();

		size_t GetFilesCount();

		std::vector<char> GetPieceHash(size_t index);

		std::string GetFileName(size_t index);

		void PrintInfo();

		private:
		Bencode::Dictionary* GetInfoSection();

		std::unique_ptr<Bencode::Dictionary> m_Contents;

	};

}

#endif
