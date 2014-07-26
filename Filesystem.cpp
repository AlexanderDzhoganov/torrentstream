#include <fstream>
#include <vector>
#include <memory>

#include "Filesystem.h"

namespace TorrentStream
{

	namespace Filesystem
	{

		struct FileImpl
		{
			std::unique_ptr<std::ofstream> m_Handle;
		};

		File::File(const std::string& path)
		{
			m_Impl = new FileImpl();
			m_Impl->m_Handle = std::make_unique<std::ofstream>(path, std::ios::binary);
			if (!m_Impl->m_Handle->is_open())
			{
				throw FileOpenError();
			}
		}

		File::~File()
		{
			delete m_Impl;
		}

		void File::WriteBytes(const std::vector<char>& bytes)
		{
			m_Impl->m_Handle->write(bytes.data(), bytes.size());

			if (m_Impl->m_Handle->failbit || m_Impl->m_Handle->badbit)
			{
				throw FileWriteError();
			}
		}

	}

}
