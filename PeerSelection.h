#ifndef __TORRENTSTREAM_PEER_SELECTION_H
#define __TORRENTSTREAM_PEER_SELECTION_H

namespace TorrentStream
{

	class IPeerSelectionStrategy
	{

		public:
		virtual void Update() = 0;
		virtual void RegisterPeer(std::unique_ptr<Peer> peer) = 0;
		virtual Peer* Select(size_t pieceIndex) = 0;
		virtual size_t GetPeersCount() = 0;
		
	};

	class PeerSelectionStrategyRandom : public IPeerSelectionStrategy
	{

		public:
		void Update();
		void RegisterPeer(std::unique_ptr<Peer> peer);
		Peer* Select(size_t pieceIndex);
		size_t GetPeersCount();

		private:
		std::vector<std::unique_ptr<Peer>> m_Peers;

	};

	typedef PeerSelectionStrategyRandom DefaultPeerSelectionStrategy;

}

#endif
