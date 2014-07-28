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

namespace TorrentStream
{

	void PeerSelectionStrategyRandom::Update()
	{
		using ASIO::PeerCommState;

		for (auto it = m_Peers.begin(); it != m_Peers.end(); ++it)
		{
			auto& peer = *it;
			peer->Update();

			if (peer->GetCommState() == PeerCommState::Error)
			{
				it = m_Peers.erase(it);
				if (it == m_Peers.end())
				{
					break;
				}

				continue;
			}
		}
	}

	void PeerSelectionStrategyRandom::RegisterPeer(std::unique_ptr<Peer> peer)
	{
		m_Peers.push_back(std::move(peer));
	}

	Peer* PeerSelectionStrategyRandom::Select(size_t pieceIndex)
	{
		for (auto&& peer : m_Peers)
		{
			if (peer->IsDownloading())
			{
				continue;
			}

			auto& available = peer->GetAvailablePieces();
			if (available[pieceIndex] == false)
			{
				continue;
			}

			return peer.get();
		}

		return nullptr;
	}

	size_t PeerSelectionStrategyRandom::GetPeersCount()
	{
		return m_Peers.size();
	}

}