#ifndef __TORRENTSTREAM_TRACKER_H
#define __TORRENTSTREAM_TRACKER_H

namespace TorrentStream
{

	enum class TrackerEvent
	{
		None = 0,
		Started,
		Stopped,
		Completed
	};

	class Tracker
	{

		public:
		Tracker(const std::string& announceURL, const std::string& infoHash, const std::string& peerID, int port, Client* client);

		~Tracker();

		std::unique_ptr<Peer> FetchPeer()
		{
			std::unique_lock<std::mutex> _(m_Mutex);

			if (m_Peers.size() == 0)
			{
				return nullptr;
			}

			auto peer = std::move(m_Peers.back());
			m_Peers.pop_back();
			return peer;
		}

		private:
		Client* m_Client;

		void RunThread();

		void Update(TrackerEvent event = TrackerEvent::None);

		std::string m_AnnounceURL;
		std::string m_PeerID;
		std::string m_InfoHash;
	
		std::set<std::string> m_Known;
		std::vector<std::unique_ptr<Peer>> m_Peers;

		bool m_Started = false;

		int m_Port = 0;

		std::mutex m_Mutex;
		std::unique_ptr<std::thread> m_Thread;

	};

}

#endif
