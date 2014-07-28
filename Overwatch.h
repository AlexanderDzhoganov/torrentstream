#ifndef __TORRENTSTREAM_OVERWATCH_H
#define __TORRENTSTREAM_OVERWATCH_H

namespace TorrentStream
{

	class Overwatch
	{

		public:
		Overwatch();

		void Update();

		void RegisterPeer(std::unique_ptr<Peer> peer);

		void Announce();

		private:
		std::unique_ptr<IPeerSelectionStrategy> m_PeerStrategy;
		std::unique_ptr<IPieceSelectionStrategy> m_PieceStrategy;

	};

}

#endif
