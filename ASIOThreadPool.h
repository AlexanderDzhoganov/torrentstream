#ifndef __TORRENTSTREAM_ASIO_H
#define __TORRENTSTREAM_ASIO_H

namespace TorrentStream
{

	namespace ASIO
	{

		class ThreadPool
		{

			public:
			ThreadPool();
			~ThreadPool();

			void Stop();

			static ThreadPool& Instance()
			{
				static ThreadPool instance;
				return instance;
			}

			boost::asio::io_service& GetService()
			{
				return *m_Service;
			}

			private:
			void RunThread();

			std::vector<std::thread> m_Threads;
			std::shared_ptr<boost::asio::io_service> m_Service;
			std::shared_ptr<boost::asio::io_service::work> m_Work;
	
		};

	}

}

#endif

