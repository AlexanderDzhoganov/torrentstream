#ifndef __TORRENTSTREAM_LOG_H
#define __TORRENTSTREAM_LOG_H

#define LOG(S) LogInstance::Instance().LogMessage(S)

namespace TorrentStream
{

	class LogInstance
	{

		public:
		LogInstance()
		{
			m_Thread = std::make_unique<std::thread>(std::bind(&LogInstance::RunThread, this));
		}

		static LogInstance& Instance()
		{
			static LogInstance instance;
			return instance;
		}

		void LogMessage(const std::string& msg)
		{
			std::unique_lock<std::mutex> _(m_Mutex);
			m_Queue.push_back(msg);
		}

		private:
		void RunThread()
		{
			for(;;)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));

				std::unique_lock<std::mutex> _(m_Mutex);
				while(m_Queue.size() > 0)
				{
					std::cout << m_Queue.front() << std::endl;
					m_Queue.pop_front();
				}
			}
		}
			
		std::unique_ptr<std::thread> m_Thread = nullptr;
	
		std::mutex m_Mutex;
		std::deque<std::string> m_Queue;

	};

}

#endif

