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
#include "Tracker.h"
#include "File.h"
#include "Client.h"

#include "LaunchProcess.h"

#define VLC_PATH "C:\\Program Files (x86)\\VideoLAN\\VLC\\vlc.exe"

namespace TorrentStream
{

	Client::Client(const std::shared_ptr<MetadataFile>& metadata, const std::string& rootPath) : m_Metadata(metadata)
	{
		m_RootPath = GetCurrentWorkingDirectory();
		std::cout << "Initializing client" << std::endl;

		m_AnnounceURL = m_Metadata->GetAnnounceURL();
		m_InfoHash = m_Metadata->GetInfoHash();
		m_PeerID = xs("-TS1001-%0000000000000000", time(0)).substr(0, 20);

		m_Tracker = std::make_unique<Tracker>(m_AnnounceURL, m_InfoHash, m_PeerID, 6642, this);

		m_PieceCount = metadata->GetPieceCount();
		m_PieceLength = metadata->GetPieceLength();

		std::cout << "Announce: " << m_AnnounceURL << ", info_hash: " << m_InfoHash << ", peer_id: " << m_PeerID << std::endl;
		std::cout << "Torrent total size: " << (metadata->GetTotalSize() / 1024) / 1024 << " MiB in " << metadata->GetFilesCount() << " files" << std::endl;
		std::cout << "Piece length: " << m_PieceLength / 1024 << "KiB, " << m_PieceCount << " pieces" << std::endl;

		for (auto i = 0u; i < metadata->GetPieceCount(); i++)
		{
			m_Pieces.emplace_back((size_t)m_PieceLength, metadata->GetPieceHash(i));
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

			file->handle = std::make_unique<Filesystem::File>(m_RootPath + "\\" + file->filename, (size_t)file->size);
			m_Files.push_back(std::move(file));
		}
	}

	void Client::Start(size_t fileToPlay)
	{
		m_FileToPlay = fileToPlay;
		std::cout << "Starting client" << std::endl;

		bool warmUp = true;

		std::deque<size_t> pieces;
		for (auto i = 0u; i < m_PieceCount; i++)
		{
			pieces.push_back(i);
		}

		std::set<size_t> pendingPieces;
		for (auto i = 0u; i < 16; i++)
		{
			pendingPieces.insert(pieces.front());
			pieces.pop_front();
		}

		auto nextAnnounceTime = 0.0;

		auto timeStarted = Timer::GetTime();
		uint64_t downloadedBytes = 0;

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
						downloadedBytes += m_PieceLength;
						WriteOutPiece(index);
					}
				}
			}

			if (complete >= 16 && warmUp)
			{
				auto cwd = GetCurrentWorkingDirectory();
				auto path = xs("%\\%", cwd, m_Files[m_FileToPlay]->filename);
				LaunchProcess(VLC_PATH, xs("\"%\" --fullscreen", path));
				warmUp = false;
			}

			if (pendingPieces.size() < 16 && pieces.size() > 0)
			{
				pendingPieces.insert(pieces.front());
				pieces.pop_front();
			}

			if (complete == m_PieceCount)
			{
				LOG("complete");
				break;
			}

			auto offline = 0u;
			auto downloading = 0u;
			auto idle = 0u;
			auto connecting = 0u;
			auto choked = 0u;
			auto error = 0u;

			if (m_Fresh.size() < 32)
			{
				auto peer = m_Tracker->FetchPeer();
				if (peer)
				{
					auto ip = peer->GetIP();
					m_Fresh[ip] = std::move(peer);
				}
			}

			for (auto it = m_Fresh.begin(); it != m_Fresh.end(); ++it)
			{
				auto&& peer = (*it).second;

				peer->Update();

				if (peer->GetCommState() == ASIO::PeerCommState::Offline)
				{
					peer->Connect();
					offline++;
				}
				else if (peer->GetCommState() == ASIO::PeerCommState::Connecting)
				{
					connecting++;
				}
				else if (peer->GetCommState() == ASIO::PeerCommState::Working)
				{
					if (peer->IsDownloading())
					{
						downloading++;
					}
					else
					{
						idle++;
					}
				}
				else if (peer->GetCommState() == ASIO::PeerCommState::Choked)
				{
					choked++;
				}
				else if (peer->GetCommState() == ASIO::PeerCommState::Error)
				{
					error++;
					it = m_Fresh.erase(it);
					
					if (it == m_Fresh.end())
					{
						break;
					}

					continue;
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
					if (peer->GetElapsedTime() > 8.0)
					{
						peer->Taint();
						pendingPieces.insert(peer->GetCurrentPieceIndex());
					}
				}
			}

			if (nextAnnounceTime <= Timer::GetTime())
			{
				auto avgBandwidth = downloadedBytes / (Timer::GetTime() - timeStarted);

				std::string msg = "";
				if (warmUp)
				{
					msg = xs("Buffering: % %", ((double)complete / 16.0) * 100.0, "%");
				}

				LOG(xs("% (%/%) [% kbps] (i: % d: % ch: %) [%]", 
					msg, complete, m_PieceCount, avgBandwidth / 1024,
					idle, downloading, choked, pendingPieces.size()));
				nextAnnounceTime = Timer::GetTime() + 1.0;
			}
		}
	}

	void Client::Stop()
	{
		m_Tracker = nullptr;
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

	void Client::WriteOutPiece(size_t index)
	{
		auto& piece = m_Pieces[index];

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
