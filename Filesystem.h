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
			File(const std::string& path, size_t size);
			~File();

			void Submit(size_t offset, const std::vector<char>& bytes);

			void Close();

			private:
			void RunThread();
			void WriteBytes(size_t offset, const std::vector<char>& bytes);

			FileImpl* m_Impl = nullptr;

		};

	}

}

#endif
