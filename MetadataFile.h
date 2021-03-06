#ifndef __TORRENTSTREAM_METADATAFILE_H
#define __TORRENTSTREAM_METADATAFILE_H

namespace TorrentStream
{

	class InvalidInfoSection : public std::exception {};
	
	class InvalidFileIndex : public std::exception {};

	class MetadataFile
	{

		public:
		MetadataFile(const std::vector<char>& contents);

		std::vector<char> Encode()
		{
			return m_Contents->Encode();
		}

		std::string GetInfoHash()
		{
			return m_InfoHash;
		}

		std::string GetAnnounceURL();

		uint64_t GetPieceLength();

		uint64_t GetPieceCount();

		uint64_t GetTotalSize();

		size_t GetFilesCount();

		std::vector<char> GetPieceHash(size_t index);

		std::string GetFileName(size_t index);

		uint64_t GetFileSize(size_t index);

		std::pair<uint64_t, uint64_t> GetFileStart(size_t index);

		std::pair<uint64_t, uint64_t> GetFileEnd(size_t index);

		void PrintInfo();

		private:
		void ExtractInfoHash(const std::vector<char>& contents);

		Bencode::Dictionary* GetInfoSection();

		std::string m_InfoHash;

		std::unique_ptr<Bencode::Dictionary> m_Contents;

	};

}

#endif
