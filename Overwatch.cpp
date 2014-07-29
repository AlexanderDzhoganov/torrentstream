#define _CRT_SECURE_NO_WARNINGS

#include <sstream>
#include <deque>
#include <vector>
#include <unordered_map>
#include <ctime>
#include <set>
#include <iostream>
#include <string>
#include <assert.h>
#include <tuple>
#include <memory>
#include <thread>
#include <mutex>

#include <boost/asio.hpp>
#include "ASIOThreadPool.h"
#include "ASIOPeerMessage.h"
#include "ASIOPeerComm.h"

#include "Log.h"

#include "extern/hashlib2plus/hashlibpp.h"

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
#include "File.h"
#include "Tracker.h"
#include "Overwatch.h"
#include "Client.h"

namespace TorrentStream
{

	Overwatch::Overwatch(size_t pieceCount, size_t pieceLength, Client* client, size_t pieceStart, size_t pieceEnd) : m_Client(client)
	{
		m_PieceStrategy = std::make_unique<DefaultPieceSelectionStrategy>(pieceCount, pieceLength, pieceStart, pieceEnd);
		m_PeerStrategy = std::make_unique<DefaultPeerSelectionStrategy>();
	}

	void Overwatch::Update()
	{
		m_PeerStrategy->Update();
		m_PieceStrategy->Update();

		if (m_Downloads.size() < m_MaxConcurrentDownloads)
		{
			if (m_PieceStrategy->HasPendingPieces())
			{
				auto piece = m_PieceStrategy->Select();
				auto peer = m_PeerStrategy->Select(piece);
				if (peer != nullptr)
				{
					peer->StartDownload(piece);
					m_Downloads.push_back(std::make_tuple(std::move(peer), piece, Timer::GetTime()));
				}
				else
				{
					m_PieceStrategy->Return(piece);
				}
			}
		}

		for (auto it = m_Downloads.begin(); it != m_Downloads.end(); ++it)
		{
			auto& peer = std::get<0>(*it);
			auto index = std::get<1>(*it);
			auto time = Timer::GetTime() - std::get<2>(*it);

			if (peer->IsFinished())
			{
				auto data = peer->GetPieceData();
				if (!data)
				{
					m_PieceStrategy->Return(index);
				}
				else
				{
					auto hash = sha1wrapper().getHashFromBytes((unsigned char*)data->GetData().data(), data->GetData().size());
					std::vector<char> hashBytes(hash.begin(), hash.end());

					if (!m_Client->CheckPieceHash(index, hashBytes))
					{
						m_PieceStrategy->Return(index);
						peer->Taint();
					}
					else
					{
						m_PieceStrategy->NotifyPieceComplete(index);
						data->SetComplete(true);
						m_Client->SubmitPiece(index, std::move(data));
					}
				}

				it = m_Downloads.erase(it);
				if (it == m_Downloads.end())
				{
					break;
				}

				continue;
			}

			if (peer->GetCommState() == ASIO::PeerCommState::Error)
			{
//				peer->StopDownload();
				m_PieceStrategy->Return(index);

				it = m_Downloads.erase(it);
				if (it == m_Downloads.end())
				{
					break;
				}

				continue;
			}
		}
	}

	void Overwatch::RegisterPeer(std::unique_ptr<Peer> peer)
	{
		m_PeerStrategy->RegisterPeer(std::move(peer));
	}

	void Overwatch::Announce()
	{
		LOG_F("[ % / % ] [ % KiB/s ]", m_PieceStrategy->GetCompletePieceCount(), m_PieceStrategy->GetPieceCount(), m_PieceStrategy->GetAverageBandwidth() / 1024);
	}

}