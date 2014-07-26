#include <thread>
#include <memory>

#include <boost/asio.hpp>

#include "ASIOThreadPool.h"

namespace TorrentStream
{

	namespace ASIO
	{

		ThreadPool::ThreadPool()
		{
			using boost::asio::io_service;

			m_Service = std::make_shared<io_service>();
			m_Work = std::make_shared<io_service::work>(*m_Service);

			for (auto i = 0u; i < std::thread::hardware_concurrency(); i++)
			{
				m_Threads.emplace_back(std::bind(&ThreadPool::RunThread, this));
			}
		}

		ThreadPool::~ThreadPool()
		{
			m_Work = nullptr;
			m_Service->stop();

			m_Service = nullptr;

			for (auto&& thread : m_Threads)
			{
				thread.join();
			}
		}

		void ThreadPool::Stop()
		{
			m_Work = nullptr;
			m_Service->stop();
		}

		void ThreadPool::RunThread()
		{
			for (;;)
			{
				try
				{
					if (m_Service)
						m_Service->run();
					break;
				}
				catch (const std::exception& e)
				{
					std::cout << "io_service::run() exception" << std::endl;
				}
			}
		}

	}

}
