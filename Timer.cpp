#include <windows.h>

#include "Timer.h"

namespace TorrentStream
{

	double Timer::PCFreq = 0;
	__int64 Timer::CounterStart = 0;

	void Timer::Initialize()
	{
		LARGE_INTEGER li;
		QueryPerformanceFrequency(&li);
		PCFreq = double(li.QuadPart);
		QueryPerformanceCounter(&li);
		CounterStart = li.QuadPart;
	}

	double Timer::GetTime()
	{
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		return double(li.QuadPart - CounterStart) / PCFreq;
	}

}