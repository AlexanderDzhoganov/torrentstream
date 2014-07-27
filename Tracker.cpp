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
#include "File.h"
#include "Tracker.h"

namespace TorrentStream
{
	
	struct bin2hex_str
	{
		std::ostream& os;
		bin2hex_str(std::ostream& os) : os(os) {}
		void operator ()(unsigned char ch)
		{
			os << std::hex
				<< std::setw(2)
				<< static_cast<int>(ch);
		}
	};

	Tracker::Tracker(const std::string& announceURL, const std::string& infoHash, const std::string& peerID, int port, Client* client) :
		m_AnnounceURL(announceURL), m_InfoHash(infoHash), m_PeerID(peerID), m_Port(port), m_Client(client)
	{
		m_Thread = std::make_unique<std::thread>(std::bind(&Tracker::RunThread, this));
	}

	Tracker::~Tracker()
	{
		Update(TrackerEvent::Stopped);
	}

	void Tracker::RunThread()
	{
		auto nextUpdate = Timer::GetTime();

		for (;;)
		{
			if (Timer::GetTime() < nextUpdate)
			{
				continue;
			}

			nextUpdate = Timer::GetTime() + 30.0;

			if (!m_Started)
			{
				Update(TrackerEvent::Started);
				m_Started = true;
			}
			else
			{
				Update();
			}
		}
	}

	void Tracker::Update(TrackerEvent event)
	{
		auto announce = HTTP::ParseURL(m_AnnounceURL);

		std::cout << "Tracker: " << announce.authority << std::endl;
		std::cout << "Peer ID: " << m_PeerID << std::endl;
		std::cout << "Port: " << m_Port << std::endl;

		announce.arguments["info_hash"] = url_encode((unsigned char*)m_InfoHash.c_str());
		announce.arguments["peer_id"] = m_PeerID;
		announce.arguments["port"] = "6881";

		switch (event)
		{
		case TrackerEvent::Started:
			announce.arguments["event"] = "started";
			break;
		case TrackerEvent::Stopped:
			announce.arguments["event"] = "stopped";
			break;
		case TrackerEvent::Completed:
			announce.arguments["event"] = "completed";
			break;
		}

		auto response = HTTP::DoHTTPRequest(announce);

		if (response.statusCode != 200)
		{
			std::cout << xs("Client::Start(): failed, http response: %", response.statusCode) << std::endl;
			return;
		}

		auto parsed = std::make_unique<Bencode::Dictionary>(Bencode::Tokenizer::Tokenize(response.content));

		if (parsed->GetKey("failure reason") != nullptr)
		{
			auto reason = parsed->GetKey<Bencode::ByteString>("failure reason");
			auto bytes = reason->GetBytes();
			std::string reasonString(bytes.begin(), bytes.end());

			std::cout << "tracker request failed: " << reasonString << std::endl;
			return;
		}

		auto peersList = (Bencode::List*)(parsed->GetKey("peers").get());
		for (auto& peerObject : peersList->GetObjects())
		{
			auto peerDict = (Bencode::Dictionary*)peerObject.get();

			auto ipBytes = ((Bencode::ByteString*)peerDict->GetKey("ip").get())->GetBytes();
			auto port = ((Bencode::Integer*)peerDict->GetKey("port").get())->GetValue();
			auto idBytes = ((Bencode::ByteString*)peerDict->GetKey("peer id").get())->GetBytes();

			std::string ip(ipBytes.data(), ipBytes.size());

			std::ostringstream oss;
			oss << std::setfill('0');
			std::for_each(ipBytes.begin(), ipBytes.end(), bin2hex_str(oss));
			auto id = oss.str();

			if (m_Known.find(ip) == m_Known.end())
			{
				m_Peers.push_back(std::make_unique<Peer>(ip, (int)port, id, m_Client));
				m_Known.insert(ip);
			}
		}
	}

}
