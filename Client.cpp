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

#include "Timer.h"
#include "BandwidthTracker.h"
#include "Filesystem.h"
#include "StringFacility.h"
#include "BinaryString.h"
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

	Client::Client(const std::shared_ptr<MetadataFile>& metadata, const std::string& rootPath) : m_Metadata(metadata), m_RootPath(rootPath)
	{
		std::cout << "Initializing client" << std::endl;

		m_AnnounceURL = m_Metadata->GetAnnounceURL();
		m_InfoHash = m_Metadata->GetInfoHash();
		m_PeerID = xs("-TS1001-%0000000000000000", time(0)).substr(0, 20);

		m_PieceCount = metadata->GetPieceCount();
		m_PieceLength = metadata->GetPieceLength();

		std::cout << "Announce: " << m_AnnounceURL << ", info_hash: " << m_InfoHash << ", peer_id: " << m_PeerID << std::endl;
		std::cout << "Torrent total size: " << (metadata->GetTotalSize() / 1024) / 1024 << " MiB in " << metadata->GetFilesCount() << " files" << std::endl;
		std::cout << "Piece length: " << m_PieceLength / 1024 << "KiB, " << m_PieceCount << " pieces" << std::endl;

		for (auto i = 0u; i < metadata->GetPieceCount(); i++)
		{
			m_Pieces.emplace_back(m_PieceLength, metadata->GetPieceHash(i));
		}

		for (auto i = 0u; i < metadata->GetFilesCount(); i++)
		{
			auto file = std::make_unique<File>();
			file->filename = metadata->GetFileName(i);
			file->size = metadata->GetFileSize(i);
			
			auto fileStart = metadata->GetFileStart(i);
			file->startPiece = fileStart.first;
			file->startPieceOffset = fileStart.second;

			auto fileEnd = metadata->GetFileEnd(i);
			file->endPiece = fileEnd.first;
			file->endPieceOffset = fileEnd.second;

			file->handle = std::make_unique<Filesystem::File>(m_RootPath + file->filename);
			m_Files.push_back(std::move(file));
		}
	}

	void Client::CleanUpPeers()
	{
		
	}

	void Client::Start()
	{
		std::cout << "Starting client" << std::endl;
		
		StartTracker();

		for (;;)
		{
			for (auto&& peerPair : m_Fresh)
			{
				auto&& peer = peerPair.second;
				if (peer->GetCommState() == ASIO::PeerCommState::Offline)
				{
					peer->Connect();
				}
				else if (peer->GetCommState() == ASIO::PeerCommState::Working)
				{
					peer->Update();
				}
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

	void Client::StartTracker()
	{
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
			std::string id(idBytes.data(), idBytes.size());

			if (m_Known.find(id) == m_Known.end())
			{
				m_Fresh[id] = std::make_unique<Peer>(ip, port, id, this);
				m_Known[id] = true;
			}
		}
	}

	void Client::UpdateTracker()
	{
		std::cout << "Updating tracker..";

		auto announce = HTTP::ParseURL(m_AnnounceURL);

		std::cout << "Tracker: " << announce.authority << std::endl;
		std::cout << "Peer ID: " << m_PeerID << std::endl;
		std::cout << "Port: " << m_Port << std::endl;

		announce.arguments["info_hash"] = url_encode((unsigned char*)m_InfoHash.c_str());
		announce.arguments["peer_id"] = m_PeerID;
		announce.arguments["port"] = m_Port;

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

			if (m_Known.find(id) == m_Known.end())
			{
				m_Fresh[id] = std::make_unique<Peer>(ip, port, id, this);
				m_Known[id] = true;
			}
		}

		std::cout << "got " << peersList->GetObjects().size() << " peers" << std::endl;
	}

	bool Client::CheckPieceHash(size_t index, const std::vector<char>& expected)
	{
		auto& hash = m_Pieces[index].GetHash();

		for (auto i = 0u; i < 20; i++)
		{
			if (expected[i] != hash[i])
			{
				return false;
			}
		}

		return true;
	}

}
