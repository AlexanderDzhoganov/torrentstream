#ifndef __TORRENTSTREAM_FILESYSTEM_H
#define __TORRENTSTREAM_FILESYSTEM_H

namespace TorrentStream
{

	namespace Filesystem
	{

		class FileOpenError : public std::exception {};

		class FileWriteError : public std::exception {};

		struct FileImpl;

		class File
		{

			public:
			File(const std::string& path);
			~File();

			void WriteBytes(const std::vector<char>& bytes);
		
			private:
			FileImpl* m_Impl = nullptr;

		};

	}

}

#endif
