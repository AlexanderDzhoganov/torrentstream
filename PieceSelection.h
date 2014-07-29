#ifndef __TORRENTSTREAM_PIECESELECTION_H
#define __TORRENTSTREAM_PIECESELECTION_H

namespace TorrentStream
{

	class IPieceSelectionStrategy
	{
		
		public:
		virtual void Update() = 0;

		virtual bool HasPendingPieces() = 0;
		virtual size_t Select() = 0;
		virtual void Return(size_t index) = 0;

		virtual size_t GetPieceCount() = 0;
		virtual size_t GetCompletePieceCount() = 0;

		virtual size_t GetPieceLength() = 0;

		virtual void NotifyPieceComplete(size_t index) = 0;

		virtual size_t GetAverageBandwidth() = 0;

	};

	class PieceSelectionStrategyRandom : public IPieceSelectionStrategy
	{

		public:
		PieceSelectionStrategyRandom(size_t pieceCount, size_t pieceLength, size_t pieceStart, size_t pieceEnd);

		void Update();

		bool HasPendingPieces();
		size_t Select();
		void Return(size_t index);

		size_t GetPieceCount();
		size_t GetCompletePieceCount();
		
		size_t GetPieceLength();

		void NotifyPieceComplete(size_t index);

		size_t GetAverageBandwidth()
		{
			return m_BandwidthTracker.CalculateBandwidth();
		}

		private:
		BandwidthTracker m_BandwidthTracker;

		size_t m_PieceCount = 0;
		size_t m_PieceLength = 0;

		std::vector<size_t> m_PendingPieces;
		std::vector<size_t> m_CompletePieces;
		
	};

	typedef PieceSelectionStrategyRandom DefaultPieceSelectionStrategy;

}

#endif
