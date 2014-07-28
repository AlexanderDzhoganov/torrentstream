#ifndef __TORRENTSTREAM_PIECESELECTION_H
#define __TORRENTSTREAM_PIECESELECTION_H

namespace TorrentStream
{

	class IPieceSelectionStrategy
	{
		
		public:
		virtual void Update() = 0;
		virtual size_t Select() = 0;

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
