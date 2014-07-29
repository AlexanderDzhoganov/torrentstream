#include <sstream>
#include <deque>
#include <vector>
#include <unordered_map>
#include <ctime>
#include <set>
#include <iostream>
#include <string>
#include <assert.h>
#include <memory>
#include <thread>
#include <mutex>

#include <boost/asio.hpp>
#include "ASIOThreadPool.h"
#include "ASIOPeerMessage.h"
#include "ASIOPeerComm.h"

#include "Log.h"

#include "Timer.h"
#include "BandwidthTracker.h"
#include "Filesystem.h"
#include "StringFacility.h"
#include "BinaryString.h"
#include "HTTPServe.h"
#include "HTTP.h"
#include "BencodeTokenizer.h"
#include "BencodeParser.h"
#include "MetadataFile.h"
#include "Piece.h"
#include "Peer.h"
#include "PieceSelection.h"

namespace TorrentStream
{

	PieceSelectionStrategyRandom::PieceSelectionStrategyRandom(size_t pieceCount, size_t pieceLength, size_t pieceStart, size_t pieceEnd) : m_PieceCount(pieceCount), m_PieceLength(pieceLength)
	{
		for (auto i = pieceStart; i <= pieceEnd; i++)
		{
			m_PendingPieces.push_back(i);
		}
	}

	void PieceSelectionStrategyRandom::Update()
	{

	}

	bool PieceSelectionStrategyRandom::HasPendingPieces()
	{
		return m_PendingPieces.size() > 0;
	}

	size_t PieceSelectionStrategyRandom::Select()
	{
		auto rnd = std::rand() % m_PendingPieces.size();
		auto index = m_PendingPieces[rnd];
		m_PendingPieces.erase(m_PendingPieces.begin() + rnd);
		return index;
	}

	void PieceSelectionStrategyRandom::Return(size_t index)
	{
		m_PendingPieces.push_back(index);
	}

	size_t PieceSelectionStrategyRandom::GetPieceCount()
	{
		return m_PieceCount;
	}

	size_t PieceSelectionStrategyRandom::GetCompletePieceCount()
	{
		return m_CompletePieces.size();
	}

	size_t PieceSelectionStrategyRandom::GetPieceLength()
	{
		return m_PieceLength;
	}

	void PieceSelectionStrategyRandom::NotifyPieceComplete(size_t index)
	{
		m_BandwidthTracker.AddSample(m_PieceLength);
		m_CompletePieces.push_back(index);
	}

}