#include <boost/asio.hpp>
#include <boost/asio/windows/random_access_handle.hpp>

#include <Windows.h>

#include <fstream>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>

#include "ASIOThreadPool.h"

#include "Filesystem.h"

namespace TorrentStream
{

	namespace Filesystem
	{

		struct FileImpl
		{
			HANDLE fileHandle;
			std::unique_ptr<boost::asio::windows::random_access_handle> handle;
		};

		File::File(const std::string& path, size_t size)
		{
			m_Impl = new FileImpl();

			m_Impl->fileHandle = ::CreateFile(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING, NULL);

			if (m_Impl->fileHandle == INVALID_HANDLE_VALUE)
			{
				auto err = GetLastError();
				return;
			}

			m_Impl->handle = std::make_unique<boost::asio::windows::random_access_handle>(ASIO::ThreadPool::Instance().GetService(), m_Impl->fileHandle);
		}

		File::~File()
		{
			delete m_Impl;
		}

		void File::WriteBytes(size_t offset, const std::vector<char>& bytes)
		{
			boost::system::error_code ec;
			boost::asio::write_at(*m_Impl->handle, offset, boost::asio::buffer(bytes.data(), bytes.size()), ec);
			FlushFileBuffers(m_Impl->fileHandle);
		}

	}

}
