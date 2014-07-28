#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <ctime>
#include <deque>
#include <set>
#include <algorithm>
#include <iostream>
#include <assert.h>
#include <memory>
#include <thread>
#include <mutex>
#include <deque>
#include <intrin.h>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include "ASIOThreadPool.h"
#include "ASIOPeerMessage.h"
#include "ASIOPeerComm.h"

#include "extern/hashlib2plus/hashlibpp.h"

#include "Filesystem.h"
#include "StringFacility.h"
#include "Log.h"
#include "BinaryString.h"
#include "Timer.h"
#include "BandwidthTracker.h"

#include "HTTP.h"
#include "HTTPServe.h"
#include "BencodeTokenizer.h"
#include "BencodeParser.h"
#include "MetadataFile.h"
#include "Piece.h"
#include "Peer.h"
#include "Tracker.h"
#include "File.h"
#include "PieceSelection.h"
#include "PeerSelection.h"
#include "Overwatch.h"
#include "Client.h"

namespace TorrentStream
{

	Peer::Peer(const std::string& ip, int port, const std::string& id, Client* client) :
		m_ID(id), m_Client(client), m_IP(ip)
	{
		m_Comm = std::make_unique<ASIO::PeerComm>(ip, xs("%", port), client->m_InfoHash, client->m_PeerID);
		m_HasPieces.resize((size_t)m_Client->GetPiecesCount());
	}

	void Peer::Connect()
	{
		m_Comm->Connect();
	}

	void Peer::Update()
	{
		using ASIO::Peer::MessageType;

		m_Comm->DispatchAllMessages();

		if (m_Downloading && m_Comm->GetState() == ASIO::PeerCommState::Working)
		{
			while (m_InFlight.size() < 10 && m_Requests.size() > 0)
			{
				auto offset = m_Requests.front();
				m_Requests.pop_front();

				auto size = RequestSize;

				auto msg = std::make_unique<ASIO::Peer::Request>(m_PieceIndex, offset, size);
				m_Comm->PushOutgoingMessage(std::move(msg));

				m_InFlight.push_back(offset);
			}

			if (m_Requests.size() == 0 && m_InFlight.size() == 0)
			{
				m_Downloading = false;

				auto hash = sha1wrapper().getHashFromBytes((unsigned char*)m_PieceData->GetData().data(), (size_t)m_Client->GetPieceLength());
				std::vector<char> hashBytes(hash.begin(), hash.end());

				if (!m_Client->CheckPieceHash(m_PieceIndex, hashBytes))
				{
					LOG("bad hash");
				}
				else
				{
					if (!m_Client->m_Pieces[m_PieceIndex].IsComplete())
					{
						m_Client->m_Pieces[m_PieceIndex].SubmitData(0, m_PieceData->GetData());
						m_Client->m_Pieces[m_PieceIndex].SetComplete(true);
					}
					
					m_PieceData = nullptr;
				}
			}
		}

		while (auto msg = m_Comm->PopIncomingMessage())
		{
			if (msg->GetType() == MessageType::Bitfield)
			{
				auto bitfieldMessage = (ASIO::Peer::Bitfield*)msg.get();
				auto bitfield = bitfieldMessage->GetBitfield();

				assert(m_HasPieces.size() <= bitfield.size());

				for (auto i = 0u; i < m_HasPieces.size(); i++)
				{
					m_HasPieces[i] = bitfield[i];
				}
			}
			else if (msg->GetType() == MessageType::Have)
			{
				auto haveMessage = (ASIO::Peer::Have*)msg.get();
				auto index = haveMessage->GetIndex();

				assert(index >= 0 && index < m_Client->GetPiecesCount());
				m_HasPieces[index] = true;
			}
			else if (msg->GetType() == MessageType::Piece)
			{
				auto pieceMessage = (ASIO::Peer::Piece*)msg.get();
				
				if (pieceMessage->GetIndex() != m_PieceIndex)
				{
					LOG("bad piece index");
					continue;
				}
			
				if (std::find(m_InFlight.begin(), m_InFlight.end(), pieceMessage->GetBegin()) == m_InFlight.end())
				{
					LOG("bad piece offset");
					continue;
				}

				std::remove(m_InFlight.begin(), m_InFlight.end(), pieceMessage->GetBegin());
				m_InFlight.pop_back();

				auto block = pieceMessage->GetBlock();

				if (block.size() != RequestSize && m_PieceIndex != m_Client->GetPiecesCount() - 1)
				{
					LOG("bad piece size");
					continue;
				}

				m_BandwidthTracker.AddSample(block.size());
				m_PieceData->SubmitData(pieceMessage->GetBegin(), block);
			}
			else if (msg->GetType() == MessageType::Unchoke)
			{
			}
		}
	}

	void Peer::StartDownload(size_t pieceIndex)
	{
		if (m_HasPieces[pieceIndex] == false)
		{
			LOG("cannot StartDownload(), peer doesn't have piece");
		}

		m_Downloading = true;
		m_PieceIndex = pieceIndex;
		m_PieceData = std::make_unique<Piece>((size_t)m_Client->GetPieceLength());

		m_Requests.clear();
		m_InFlight.clear();
		for (auto i = 0u; i < m_Client->GetPieceLength(); i += RequestSize)
		{
			m_Requests.push_back(i);
		}

		auto msg = std::make_unique<ASIO::Peer::Interested>();
		m_Comm->PushOutgoingMessage(std::move(msg));
	}

}
