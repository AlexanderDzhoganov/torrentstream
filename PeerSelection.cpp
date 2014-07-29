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
			if (peer->GetCommState() == ASIO::PeerCommState::Error)
			{
				continue;
			}

			if (peer->IsDownloading())
			{
				continue;
			}

			if (peer->IsTainted())
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

	void PeerSelectionStrategyBestBandwidth::Update()
	{
		using ASIO::PeerCommState;

		for (auto it = m_Peers.begin(); it != m_Peers.end(); ++it)
		{
			auto& peer = *it;
			peer->Update();
		}
	}

	void PeerSelectionStrategyBestBandwidth::RegisterPeer(std::unique_ptr<Peer> peer)
	{
		m_Peers.push_back(std::move(peer));
	}

	Peer* PeerSelectionStrategyBestBandwidth::Select(size_t pieceIndex)
	{
		std::vector<std::pair<size_t, Peer*>> sortedPeers;
		for (auto&& peer : m_Peers)
		{
			auto bandwidth = peer->GetAverageBandwidth();
			sortedPeers.push_back(std::make_pair(bandwidth, peer.get()));
		}

		std::sort(sortedPeers.begin(), sortedPeers.end(), [](const std::pair<size_t, Peer*>& a, const std::pair<size_t, Peer*>& b)
		{
			return a.first > b.first;
		});

		for (auto&& peer : m_Peers)
		{
			if (peer->GetCommState() == ASIO::PeerCommState::Error)
			{
				continue;
			}

			if (peer->IsDownloading())
			{
				continue;
			}

			if (peer->IsTainted())
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

	size_t PeerSelectionStrategyBestBandwidth::GetPeersCount()
	{
		return m_Peers.size();
	}

}