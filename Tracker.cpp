#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <ctime>
#include <iostream>
#include <assert.h>
#include <memory>
#include <thread>
#include <mutex>

#include "StringFacility.h"
#include "BinaryString.h"
#include "Filesystem.h"
#include "Socket.h"
#include "Wire.h"
#include "HTTP.h"
#include "BencodeTokenizer.h"
#include "BencodeParser.h"
#include "MetadataFile.h"
#include "Peer.h"
#include "Piece.h"
#include "File.h"
#include "Tracker.h"
#include <iomanip>

std::string url_encode(unsigned char* data)
{
	std::ostringstream escaped;
	escaped.fill('0');
	escaped << std::hex;

	for (auto i = 0u; i < 20; i++)
	{
		unsigned char c = data[i];

		// Keep alphanumeric and other accepted characters intact
		if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
		{
			escaped << c;
			continue;
		}

		// Any other characters are percent-encoded
		escaped << '%' << std::setw(2) << int((unsigned char)c);
	}

	return escaped.str();
}

namespace TorrentStream
{

	Client::Client(const std::shared_ptr<MetadataFile>& metadata) : m_Metadata(metadata)
	{
		m_AnnounceURL = m_Metadata->GetAnnounceURL();
		m_InfoHash = m_Metadata->GetInfoHash();
		m_PeerID = xs("-TS1001-%0000000000000000", time(0)).substr(0, 20);
	}

	void Client::Start()
	{
		std::cout << "Starting client" << std::endl;

		auto announce = HTTP::ParseURL(m_AnnounceURL);

		std::cout << "Tracker: " << announce.authority << std::endl;
		std::cout << "Peer ID: " << m_PeerID << std::endl;
		std::cout << "Port: " << m_Port << std::endl;

		announce.arguments["info_hash"] = url_encode((unsigned char*)m_InfoHash.c_str());
		announce.arguments["peer_id"] = m_PeerID;
		announce.arguments["port"] = m_Port;
		announce.arguments["event"] = "started";

		auto response = HTTP::DoHTTPRequest(announce);

		if (response.statusCode != 200)
		{
			std::cout << xs("Client::Start(): failed, http response: %", response.statusCode) << std::endl;
			return;
		}

		auto parsed = std::make_unique<Bencode::Dictionary>(Bencode::Tokenizer::Tokenize(response.content));

		auto peersList = (Bencode::List*)(parsed->GetKey("peers").get());

		for (auto& peerObject : peersList->GetObjects())
		{
			auto peerDict = (Bencode::Dictionary*)peerObject.get();

			auto ipBytes = ((Bencode::ByteString*)peerDict->GetKey("ip").get())->GetBytes();
			auto port = ((Bencode::Integer*)peerDict->GetKey("port").get())->GetValue();
			auto idBytes = ((Bencode::ByteString*)peerDict->GetKey("peer id").get())->GetBytes();

			std::string ip(ipBytes.data(), ipBytes.size());
			std::string id(idBytes.data(), idBytes.size());

			if (m_Peers.find(id) == m_Peers.end())
			{
				auto peer = std::make_unique<Peer>(ip, port, id, this);
				m_Peers[id] = std::move(peer);
			}
		}
	}

	void Client::Stop()
	{
		auto announce = HTTP::ParseURL(m_AnnounceURL);

		announce.arguments["info_hash"] = m_InfoHash;
		announce.arguments["peer_id"] = m_PeerID;
		announce.arguments["port"] = m_Port;
		announce.arguments["event"] = "stopped";
		HTTP::DoHTTPRequest(announce);
	}

	void Client::UpdateTracker()
	{

	}

}