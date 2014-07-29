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
#include "PieceSelection.h"
#include "PeerSelection.h"
#include "Overwatch.h"
#include "Client.h"

#include "LaunchProcess.h"

#define VLC_PATH "C:\\Program Files (x86)\\VideoLAN\\VLC\\vlc.exe"

namespace TorrentStream
{

	Client::Client(const std::shared_ptr<MetadataFile>& metadata, const std::string& rootPath, size_t fileToPlay) :
		m_Metadata(metadata), m_FileToPlay(fileToPlay), m_RootPath(GetCurrentWorkingDirectory())
	{
		Initialize();
	}

	void Client::Initialize()
	{
		LOG_F("Initializing");

		m_AnnounceURL = m_Metadata->GetAnnounceURL();
		m_InfoHash = m_Metadata->GetInfoHash();
		m_PeerID = xs("-TS1001-%0000000000000000", time(0)).substr(0, 20);

		m_Tracker = std::make_unique<Tracker>(m_AnnounceURL, m_InfoHash, m_PeerID, 6642, this);

		m_PieceCount = m_Metadata->GetPieceCount();
		m_PieceLength = m_Metadata->GetPieceLength();

		LOG_F("announce: %", m_AnnounceURL);
		LOG_F("info_hash: %", bytesToHex(m_InfoHash.c_str(), 20));
		LOG_F("total size: % MiB", (m_Metadata->GetTotalSize() / 1024) / 1024);
		LOG_F("files: %", m_Metadata->GetFilesCount());
		LOG_F("piece length: %", m_PieceLength);
		LOG_F("pieces count: %", m_PieceCount);

		for (auto i = 0u; i < m_Metadata->GetPieceCount(); i++)
		{
			m_Pieces.emplace_back((size_t)m_PieceLength, m_Metadata->GetPieceHash(i));
		}

		auto file = std::make_unique<File>();
		file->filename = m_Metadata->GetFileName(m_FileToPlay);
		file->size = m_Metadata->GetFileSize(m_FileToPlay);

		auto fileStart = m_Metadata->GetFileStart(m_FileToPlay);
		file->startPiece = fileStart.first;
		file->startPieceOffset = fileStart.second;

		auto fileEnd = m_Metadata->GetFileEnd(m_FileToPlay);
		file->endPiece = fileEnd.first;
		file->endPieceOffset = fileEnd.second;

		auto filename = split(file->filename, '\\');
		file->filename = filename[filename.size() - 1];
		file->handle = std::make_unique<Filesystem::File>(m_RootPath + "\\" + filename[filename.size() - 1], (size_t)file->size);
		m_File = std::move(file);

		m_Overwatch = std::make_unique<Overwatch>(m_PieceCount, m_PieceLength, this, m_File->startPiece, m_File->endPiece);
	}

	void Client::Start()
	{
		auto nextAnnounce = Timer::GetTime() + 2.0;

		for (;;)
		{
			auto peer = m_Tracker->FetchPeer();
			if (peer)
			{
				peer->Connect();
				m_Overwatch->RegisterPeer(std::move(peer));
			}

			m_Overwatch->Update();

			if (nextAnnounce < Timer::GetTime())
			{
				m_Overwatch->Announce();
				nextAnnounce = Timer::GetTime() + 2.0;
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
		auto& file = m_File;

		if (file->startPiece > index || file->endPiece < index)
		{
			piece.SetWrittenOut(true);
			return;
		}

		if (index == file->startPiece)
		{
			std::vector<char> data;
			data.insert(data.end(), piece.GetData().begin() + (size_t)file->startPieceOffset, piece.GetData().end());
			file->handle->Submit(0, data);
		}
		else if (index == file->endPiece)
		{
			std::vector<char> data;
			data.insert(data.end(), piece.GetData().begin(), piece.GetData().begin() + (size_t)file->endPieceOffset);
			file->handle->Submit(index * (size_t)m_PieceLength, data);
		}
		else
		{
			file->handle->Submit(index * (size_t)m_PieceLength, piece.GetData());
		}

		piece.SetWrittenOut(true);
	}

}
