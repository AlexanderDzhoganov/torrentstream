#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <ctime>
#include <deque>
#include <set>
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

#include "Socket.h"
#include "Wire.h"
#include "HTTP.h"
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
	}

	void Peer::Connect()
	{
		m_Comm->Connect();
	}

	void Peer::Update()
	{
		m_Comm->DispatchAllMessages();

		while (auto msg = m_Comm->PopIncomingMessage())
		{
			if (msg->GetType() == ASIO::Peer::MessageType::Bitfield)
			{
				LOG("bitfield");
			}
		}
	}

}
