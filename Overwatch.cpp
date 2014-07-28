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

#include "PeerSelection.h"
#include "PieceSelection.h"
#include "Overwatch.h"

namespace TorrentStream
{

	Overwatch::Overwatch()
	{
		m_PieceStrategy = std::make_unique<DefaultPieceSelectionStrategy>();
		m_PeerStrategy = std::make_unique<DefaultPeerSelectionStrategy>();
	}

	void Overwatch::Update()
	{
		m_PeerStrategy->Update();
		m_PieceStrategy->Update();
	}

	void Overwatch::RegisterPeer(std::unique_ptr<Peer> peer)
	{
		m_PeerStrategy->RegisterPeer(std::move(peer));
	}

	void Overwatch::Announce()
	{

	}

}