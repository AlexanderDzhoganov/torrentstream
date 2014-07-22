#ifndef __TORRENTSTREAM_METADATAFILE_H
#define __TORRENTSTREAM_METADATAFILE_H

namespace TorrentStream
{

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

		void PrintInfo();

		private:
		std::unique_ptr<Bencode::Dictionary> m_Contents;

	};

}

#endif
