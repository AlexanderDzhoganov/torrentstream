#include <vector>

#include "Timer.h"
#include "BandwidthTracker.h"

namespace TorrentStream
{

	void BandwidthTracker::AddSample(size_t size)
	{
		auto time = Timer::GetTime();
		m_Samples.push_back(std::make_pair(time, size));
	}

	size_t BandwidthTracker::CalculateBandwidth()
	{
		auto time = Timer::GetTime();
		auto wndStart = time - m_Window;

		auto bytesTotal = 0;

		for (auto it = m_Samples.begin(); it != m_Samples.end(); ++it)
		{
			if ((*it).first < wndStart)
			{
				it = m_Samples.erase(it);
				if (it == m_Samples.end())
				{
					break;
				}

				continue;
			}

			bytesTotal += (*it).second;
		}

		return (size_t)((double)bytesTotal / m_Window);
	}

}
