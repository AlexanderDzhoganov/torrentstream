#include <sstream>
#include <deque>
#include <vector>
#include <unordered_map>
#include <ctime>
#include <iostream>
#include <string>
#include <assert.h>
#include <memory>
#include <thread>
#include <mutex>

#include <boost/asio.hpp>

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
#include "Peer.h"
#include "Piece.h"
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
		for (auto it = m_Fresh.begin(); it != m_Fresh.end(); ++it)
		{
			if ((*it).second == nullptr || (*it).second->GetState() == PeerState::Error)
			{
				it = m_Fresh.erase(it);
				if (it == m_Fresh.end())
				{
					break;
				}
			}
		}

		for (auto it = m_WarmUp.begin(); it != m_WarmUp.end(); ++it)
		{
			if ((*it).second == nullptr || (*it).second->GetState() == PeerState::Error)
			{
				it = m_WarmUp.erase(it);
				if (it == m_WarmUp.end())
				{
					break;
				}
			}
		}

		for (auto it = m_Cold.begin(); it != m_Cold.end(); ++it)
		{
			if ((*it).second == nullptr || (*it).second->GetState() == PeerState::Error)
			{
				it = m_Cold.erase(it);
				if (it == m_Cold.end())
				{
					break;
				}
			}
		}

		for (auto it = m_Hot.begin(); it != m_Hot.end(); ++it)
		{
			if ((*it).second == nullptr || (*it).second->GetState() == PeerState::Error)
			{
				it = m_Hot.erase(it);
				if (it == m_Hot.end())
				{
					break;
				}
			}
		}
	}

	void Client::Start()
	{
		std::cout << "Starting client" << std::endl;
		
		StartTracker();

		auto nextCheckTime = Timer::GetTime() + 0.1f;

		// warm-up phase
		for (;;)
		{
			if (Timer::GetTime() < nextCheckTime)
			{
				continue;
			}

			nextCheckTime = Timer::GetTime() + 0.1f;
			CleanUpPeers();

			if (m_Cold.size() > 16)
			{
				break;
			}

			for (auto& peer : m_Fresh)
			{
				if (peer.second == nullptr)
				{
					continue;
				}

				if (peer.second->GetState() == PeerState::Disconnected)
				{
					peer.second->Connect();
				}
				else if (peer.second->GetState() == PeerState::Idle && m_WarmUp.size() < 8)
				{
					peer.second->WarmUp();
					m_WarmUp[peer.first] = std::move(peer.second);
				}
			}

			for (auto& peer : m_WarmUp)
			{
				if (peer.second == nullptr)
				{
					continue;
				}

				if (peer.second->GetState() == PeerState::Idle)
				{
					m_Cold[peer.first] = std::move(peer.second);
				}
			}
		}

		std::vector<std::pair<size_t, std::unique_ptr<Peer>>> cold;
		
		for (auto& peer : m_Cold)
		{
			cold.push_back(std::make_pair(peer.second->GetAverageBandwidth(), std::move(peer.second)));
		}

		m_Cold.clear();

		std::sort(cold.begin(), cold.end(), [](const std::pair<size_t, std::unique_ptr<Peer>>& left, const std::pair<size_t, std::unique_ptr<Peer>>& right) { return left.first < right.first; });

		for (auto i = 0u; i < 4; i++)
		{
			m_Hot[cold.back().second->GetID()] = std::move(cold.back().second);
			cold.pop_back();
		}

		for (auto& peer : cold)
		{
			m_Cold[peer.second->GetID()] = std::move(peer.second);
		}

		// download phase
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

			std::ostringstream ss;

			ss << std::hex << std::uppercase << std::setfill('0');
			for (int c : idBytes) {
				ss << std::setw(2) << c;
			}

			std::string ip(ipBytes.data(), ipBytes.size());
			std::string id = ss.str();

			if (m_Known.find(id) == m_Known.end())
			{
				m_Fresh[id] = std::make_unique<Peer>(ip, port, id, this);
				m_Known[id] = true;
			}
		}

		std::cout << "got " << peersList->GetObjects().size() << " peers" << std::endl;
	}

}