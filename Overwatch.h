#ifndef __TORRENTSTREAM_OVERWATCH_H
#define __TORRENTSTREAM_OVERWATCH_H

namespace TorrentStream
{

	class Client;

	class Overwatch
	{

		public:
		Overwatch(size_t pieceCount, size_t pieceLength, Client* client, size_t pieceStart, size_t pieceEnd);

		void Update();

		void RegisterPeer(std::unique_ptr<Peer> peer);

		void Announce();

		private:
		std::unique_ptr<IPeerSelectionStrategy> m_PeerStrategy;
		std::unique_ptr<IPieceSelectionStrategy> m_PieceStrategy;

		std::vector<std::tuple<Peer*, size_t, double>> m_Downloads;
		size_t m_MaxConcurrentDownloads = 16;

		Client* m_Client = nullptr;

	};

}

#endif
