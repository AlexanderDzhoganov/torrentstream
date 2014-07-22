#ifndef __TORRENTSTREAM_FILE_H
#define __TORRENTSTREAM_FILE_H

namespace TorrentStream
{

	struct File
	{

		size_t size;
		size_t startPiece;
		size_t startPieceOffset;

		size_t endPiece;
		size_t endPieceOffset;

		std::string filename;

	};

}

#endif
