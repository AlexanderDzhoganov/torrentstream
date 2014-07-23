#ifndef __TORRENTSTREAM_TIMER_H
#define __TORRENTSTREAM_TIMER_H

namespace TorrentStream
{

	class Timer
	{

		static double PCFreq;
		static __int64 CounterStart;

		public:
		static void Initialize();
		static double GetTime();

	};

};

#endif
