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

			file->handle = std::make_unique<Filesystem::File>(m_RootPath + file->filename, file->size);
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

		std::set<size_t> pendingPieces;
		for (auto i = 0u; i < m_PieceCount; i++)
		{
			pendingPieces.insert(i);
		}

		auto nextAnnounceTime = 0.0;
		auto nextTrackerUpdateTime = 0.0;
		auto pieceCounter = 0;

		std::string fastestPeer;
		double fastestPeerTime = 99999999999.0;

		for (;;)
		{
			auto complete = 0;
			for (auto index = 0u; index < m_Pieces.size(); index++)
			{
				auto& piece = m_Pieces[index];

				if (piece.IsComplete())
				{
					pendingPieces.erase(index);
					complete++;

					if (!piece.IsWrittenOut())
					{
						for (auto&& file : m_Files)
						{
							if (file->startPiece > index || file->endPiece < index)
							{
								continue;
							}

							if (index == file->startPiece)
							{
								std::vector<char> data;
								data.insert(data.end(), piece.GetData().begin() + file->startPieceOffset, piece.GetData().end());
								file->handle->WriteBytes(0, data);
							}
							else if (index == file->endPiece)
							{
								std::vector<char> data;
								data.insert(data.end(), piece.GetData().begin(), piece.GetData().begin() + file->endPieceOffset);
								file->handle->WriteBytes(index * m_PieceLength, data);
							}
							else
							{
								file->handle->WriteBytes(index * m_PieceLength, piece.GetData());
							}

							piece.SetWrittenOut(true);
						}
					}
				}
			}

			if (complete == m_PieceCount)
			{
				LOG("complete");
				break;
			}

			if (nextAnnounceTime <= Timer::GetTime())
			{
				auto newlyComplete = complete - pieceCounter;
				pieceCounter = complete;

				size_t avgKbps = (newlyComplete * m_PieceLength) / 1024;

				LOG(xs("% / % [% kbps avg.]", complete, m_PieceCount, avgKbps));

				nextAnnounceTime = Timer::GetTime() + 1.0;
			}

			if (nextTrackerUpdateTime <= Timer::GetTime())
			{
			//	UpdateTracker();
				nextTrackerUpdateTime = Timer::GetTime() + 20.0;
			}

			for (auto&& peerPair : m_Fresh)
			{
				auto&& peer = peerPair.second;

				peer->Update();

				if (peer->GetCommState() == ASIO::PeerCommState::Offline)
				{
					peer->Connect();
				}

				if (!peer->IsDownloading())
				{
					auto& availablePieces = peer->GetAvailablePieces();
					for (auto it = pendingPieces.begin(); it != pendingPieces.end(); ++it)
					{
						if (availablePieces[*it] == true)
						{
							peer->StartTimer();
							peer->StartDownload(*it);
							pendingPieces.erase(*it);
							break;
						}
					}
				}
				else if (peer->IsDownloading() && !peer->IsTainted())
				{
					if (peer->GetElapsedTime() > 5.0)
					{
						peer->Taint();
					}
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

			std::ostringstream oss;
			oss << std::setfill('0');
			std::for_each(ipBytes.begin(), ipBytes.end(), bin2hex_str(oss));
			auto id = oss.str();

			if (m_Known.find(ip) == m_Known.end())
			{
				m_Fresh[ip] = std::make_unique<Peer>(ip, port, id, this);
				m_Known[ip] = true;
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
		announce.arguments["numwant"] = 100;

		auto response = HTTP::DoHTTPRequest(announce);

		if (response.statusCode != 200)
		{
			std::cout << xs("Client::Start(): failed, http response: %", response.statusCode) << std::endl;
			return;
		}

		auto parsed = std::make_unique<Bencode::Dictionary>(Bencode::Tokenizer::Tokenize(response.content));

		auto failureReason = parsed->GetKey<Bencode::ByteString>("failure reason");
		if (failureReason != nullptr)
		{
			std::string reason(failureReason->GetBytes().data(), failureReason->GetBytes().size());
			LOG(xs("Tracker request failed: %", reason));
			return;
		}

		auto peersList = (Bencode::List*)(parsed->GetKey("peers").get());
		if (peersList == nullptr)
		{
			return;
		}

		for (auto& peerObject : peersList->GetObjects())
		{
			auto peerDict = (Bencode::Dictionary*)peerObject.get();

			auto ipBytes = ((Bencode::ByteString*)peerDict->GetKey("ip").get())->GetBytes();
			auto port = ((Bencode::Integer*)peerDict->GetKey("port").get())->GetValue();
			auto idBytes = ((Bencode::ByteString*)peerDict->GetKey("peer id").get())->GetBytes();

			std::string ip(ipBytes.data(), ipBytes.size());
			std::string id(idBytes.data(), idBytes.size());

			if (m_Known.find(ip) == m_Known.end())
			{
				m_Fresh[ip] = std::make_unique<Peer>(ip, port, id, this);
				m_Known[ip] = true;
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
