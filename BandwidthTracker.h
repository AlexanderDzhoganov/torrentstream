#ifndef __TORRENTSTREAM_BANDWIDTH_TRACKER_H
#define __TORRENTSTREAM_BANDWIDTH_TRACKER_H

namespace TorrentStream
{

	class BandwidthTracker
	{

		public:
		void AddSample(size_t size);

		size_t CalculateBandwidth();

		void SetWindow(double seconds)
		{
			m_Window = seconds;
		}

		private:
		double m_Window = 20.0;
		std::vector<std::pair<double, size_t>> m_Samples;

	};

}

#endif
