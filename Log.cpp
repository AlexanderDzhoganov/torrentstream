#include <thread>
#include <mutex>
#include <vector>
#include <deque>
#include <string>
#include <memory>
#include <sstream>
#include <iostream>

#include "StringFacility.h"
#include "Log.h"

namespace TorrentStream
{

	LogInstance::LogInstance()
	{
		m_Thread = std::make_unique<std::thread>(std::bind(&LogInstance::RunThread, this));
		m_Thread->detach();
	}

	void LogInstance::LogMessage(const std::string& msg)
	{
		std::unique_lock<std::mutex> _(m_Mutex);
		m_Queue.push_back(msg);
	}

	void LogInstance::RunThread()
	{
		for (;;)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));

			std::unique_lock<std::mutex> _(m_Mutex);
			while (m_Queue.size() > 0)
			{
				std::cout << m_Queue.front() << std::endl;
				m_Queue.pop_front();
			}
		}
	}

}