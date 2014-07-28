#ifndef __TORRENTSTREAM_PIECESELECTION_H
#define __TORRENTSTREAM_PIECESELECTION_H

namespace TorrentStream
{

	class IPieceSelectionStrategy
	{
		
		public:
		virtual void Update() = 0;
		virtual size_t Select() = 0;

		virtual size_t GetPieceCount() = 0;
		virtual size_t GetPieceLength() = 0;

		virtual void NotifyPieceComplete(size_t index) = 0;
		virtual void SubmitPieceData(size_t index, size_t offset, const std::vector<char>& data) = 0;

	};

	class PieceSelectionStrategyRandom : public IPieceSelectionStrategy
	{

		public:
		void Update();

		size_t Select();

	};

	typedef PieceSelectionStrategyRandom DefaultPieceSelectionStrategy;

}

#endif
