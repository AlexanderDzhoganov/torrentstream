#ifndef __TORRENTSTREAM_FILE_H
#define __TORRENTSTREAM_FILE_H

namespace TorrentStream
{

	struct File
	{

		uint64_t size;
		uint64_t startPiece;
		uint64_t startPieceOffset;

		uint64_t endPiece;
		uint64_t endPieceOffset;

		std::string filename;

		std::unique_ptr<Filesystem::File> handle;

	};

}

#endif
