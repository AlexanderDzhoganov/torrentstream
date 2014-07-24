#ifndef __TORRENTSTREAM_BANDWIDTH_TRACKER_H
#define __TORRENTSTREAM_BANDWIDTH_TRACKER_H

namespace TorrentStream
{

	class BandwidthTracker
	{

		public:
		void Start();

		void AddPacket(size_t size);

		size_t GetAverageBandwidth() const;

		private:
		double m_StartTime = 0.0;
		size_t m_Bytes = 0;

	};

}

#endif
