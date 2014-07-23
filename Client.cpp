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

#include "Timer.h"
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

			std::ostringstream ss;

			ss << std::hex << std::uppercase << std::setfill('0');
			for (int c : idBytes) {
				ss << std::setw(2) << c;
			}

			std::string ip(ipBytes.data(), ipBytes.size());
			std::string id = ss.str();

			if (m_Peers.find(id) == m_Peers.end())
			{
				auto peer = std::make_unique<Peer>(ip, port, id, this);
				m_Peers[id] = std::move(peer);
			}
		}

		auto nextCheckTime = Timer::GetTime() + 1.0f;
		auto nextTrackerUpdateTime = Timer::GetTime() + 20.0f;

		for (;;)
		{
			for (auto& peer : m_Peers)
			{
				if (!peer.second->IsConnected())
				{
					m_Peers.erase(peer.first);
					break;
				}
			}

			if (Timer::GetTime() < nextCheckTime)
			{
				continue;
			}

			nextCheckTime = Timer::GetTime() + 1.0f;

			auto unchoked = 0;
			auto downloading = 0;

			for (auto& peer : m_Peers)
			{
				if (!peer.second->IsChoked())
				{
					unchoked++;
				}

				if (peer.second->IsDownloading())
				{
					downloading++;
				}
			}

			std::cout << "Total peers: " << m_Peers.size() << " unchoked: " << unchoked << ", downloading: " << downloading << std::endl;

			auto finished = 0;
			for (auto& piece : m_Pieces)
			{
				if (piece.IsComplete())
				{
					finished++;
				}
			}

			std::cout << xs("% of % pieces downloaded", finished, m_Pieces.size()) << std::endl;

			if (finished == m_Pieces.size())
			{
				std::cout << "download finished!" << std::endl;
				m_Peers.clear();
				return;
			}

			if (m_Peers.size() < 32 && Timer::GetTime() >= nextTrackerUpdateTime)
			{
				UpdateTracker();
				nextTrackerUpdateTime = Timer::GetTime() + 20.0f;
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

			if (m_Peers.find(id) == m_Peers.end())
			{
				auto peer = std::make_unique<Peer>(ip, port, id, this);
				m_Peers[id] = std::move(peer);
			}
		}

		std::cout << "got " << peersList->GetObjects().size() << " peers" << std::endl;
	}

	void Client::SubmitData(size_t pieceIndex, size_t pieceOffset, const std::vector<char>& data)
	{
		std::unique_lock<std::mutex> _(m_PiecesMutex);
		m_Pieces[pieceIndex].SubmitData(pieceOffset, data);
	}

}