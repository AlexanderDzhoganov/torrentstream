#ifndef __TORRENTSTREAM_LOG_H
#define __TORRENTSTREAM_LOG_H

#define LOG(S) LogInstance::Instance().LogMessage(S)
#define LOG_F(S, ...) LogInstance::Instance().LogMessage(xs("%> %", LogInstance::ParseModuleName(__FUNCTION__), xs(S, __VA_ARGS__)))

namespace TorrentStream
{

	class LogInstance
	{

		public:
		LogInstance();

		static LogInstance& Instance()
		{
			static LogInstance instance;
			return instance;
		}

		void LogMessage(const std::string& msg);

		static std::string ParseModuleName(const std::string& module)
		{
			return module.substr(15, module.length() - 15);
		}

		private:
		void RunThread();
			
		std::unique_ptr<std::thread> m_Thread = nullptr;
	
		std::mutex m_Mutex;
		std::deque<std::string> m_Queue;

	};

}

#endif

