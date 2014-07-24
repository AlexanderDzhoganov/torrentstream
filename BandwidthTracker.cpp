#include <vector>

#include "Timer.h"
#include "BandwidthTracker.h"

namespace TorrentStream
{

	void BandwidthTracker::Start()
	{
		m_StartTime = Timer::GetTime();
		m_Bytes = 0;
	}

	void BandwidthTracker::AddPacket(size_t size)
	{
		m_Bytes += size;
	}

	size_t BandwidthTracker::GetAverageBandwidth() const
	{
		auto elapsed = Timer::GetTime() - m_StartTime;
		return (size_t)floor((double)m_Bytes / elapsed);
	}

}