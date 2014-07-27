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
#include "File.h"
#include "Client.h"

namespace TorrentStream
{

	Peer::Peer(const std::string& ip, int port, const std::string& id, Client* client) :
		m_ID(id), m_Client(client)
	{
		m_Comm = std::make_unique<ASIO::PeerComm>(ip, xs("%", port), client->m_InfoHash, client->m_PeerID);
		m_HasPieces.resize(m_Client->GetPiecesCount());
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
			while (m_PieceOffsetRequestsInFlight.size() < 10 && m_PieceOffsetRequests.size() > 0)
			{
				auto offset = m_PieceOffsetRequests.front();
				m_PieceOffsetRequests.pop_front();

				auto size = RequestSize;

				if (m_PieceIndex == m_Client->GetPiecesCount() - 1)
				{
				//	size = m_Client->GetTotalSize() - (m_Client->GetTotalSize() / RequestSize) * RequestSize;
				}

				auto msg = std::make_unique<ASIO::Peer::Request>(m_PieceIndex, offset, size);
				m_Comm->PushOutgoingMessage(std::move(msg));

				m_PieceOffsetRequestsInFlight.push_back(offset);
			}

			if (m_PieceOffsetRequests.size() == 0 && m_PieceOffsetRequestsInFlight.size() == 0)
			{
				m_Downloading = false;

				auto hash = sha1wrapper().getHashFromBytes((unsigned char*)m_PieceData->GetData().data(), m_Client->GetPieceLength());
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
			
				if (std::find(m_PieceOffsetRequestsInFlight.begin(), m_PieceOffsetRequestsInFlight.end(), pieceMessage->GetBegin()) == m_PieceOffsetRequestsInFlight.end())
				{
					LOG("bad piece offset");
					continue;
				}

				std::remove(m_PieceOffsetRequestsInFlight.begin(), m_PieceOffsetRequestsInFlight.end(), pieceMessage->GetBegin());
				m_PieceOffsetRequestsInFlight.pop_back();

				auto block = pieceMessage->GetBlock();

				if (block.size() != RequestSize && m_PieceIndex != m_Client->GetPiecesCount() - 1)
				{
					LOG("bad piece size");
					continue;
				}

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
		m_PieceData = std::make_unique<Piece>(m_Client->GetPieceLength());

		m_PieceOffsetRequests.clear();
		m_PieceOffsetRequestsInFlight.clear();
		for (auto i = 0u; i < m_Client->GetPieceLength(); i += RequestSize)
		{
			m_PieceOffsetRequests.push_back(i);
		}

		auto msg = std::make_unique<ASIO::Peer::Interested>();
		m_Comm->PushOutgoingMessage(std::move(msg));
	}

}
