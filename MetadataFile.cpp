#define _CRT_SECURE_NO_WARNINGS

#include <vector>
#include <string>
#include <sstream>
#include <deque>
#include <memory>
#include <unordered_map>
#include <assert.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <algorithm>

#include "Log.h"

#include "StringFacility.h"
#include "BencodeTokenizer.h"
#include "BencodeParser.h"
#include "MetadataFile.h"
#include "extern/hashlib2plus/hashlibpp.h"

namespace TorrentStream
{

	MetadataFile::MetadataFile(const std::vector<char>& contents)
	{
		ExtractInfoHash(contents);

		auto tokenized = Bencode::Tokenizer::Tokenize(contents);
		m_Contents = std::make_unique<Bencode::Dictionary>(tokenized);
	}

	void MetadataFile::ExtractInfoHash(const std::vector<char>& contents)
	{
		size_t infoStart = 0;
		for (auto i = 0u; i < contents.size(); i++)
		{
			if (contents[i] == 'i' && contents[i + 1] == 'n' && contents[i + 2] == 'f' && contents[i + 3] == 'o')
			{
				infoStart = i + 4;
				break;
			}
		}

		std::vector<char> infoData;
		infoData.insert(infoData.end(), contents.begin() + infoStart, contents.end() - 1);

		m_InfoHash = sha1wrapper().getHashFromBytes((unsigned char*)infoData.data(), infoData.size());
	}

	std::string MetadataFile::GetAnnounceURL()
	{
		auto announceKey = m_Contents->GetKey<Bencode::ByteString>("announce");
		auto announceStringBytes = announceKey->GetBytes();
		return std::string(announceStringBytes.data(), announceStringBytes.size());
	}

	uint64_t MetadataFile::GetPieceLength()
	{
		auto info = GetInfoSection();
		auto pieceLength = info->GetKey<Bencode::Integer>("piece length");
		return pieceLength->GetValue();
	}

	uint64_t MetadataFile::GetPieceCount()
	{
		return (uint64_t)ceil((double)GetTotalSize() / (double)GetPieceLength());
	}

	uint64_t MetadataFile::GetTotalSize()
	{
		auto info = GetInfoSection();
		uint64_t totalSize = 0;
		auto files = info->GetKey<Bencode::List>("files");
		
		if (files != nullptr)
		{
			for (auto& fileObj : files->GetObjects())
			{
				assert(fileObj->GetType() == Bencode::ObjectType::DICTIONARY);

				auto file = (Bencode::Dictionary*)fileObj.get();
				auto fileLength = file->GetKey<Bencode::Integer>("length")->GetValue();
				totalSize += fileLength;
			}
		}
		else
		{
			auto fileLength = info->GetKey<Bencode::Integer>("length")->GetValue();
			totalSize += fileLength;
		}

		return totalSize;
	}

	size_t MetadataFile::GetFilesCount()
	{
		auto info = GetInfoSection();
		auto files = info->GetKey<Bencode::List>("files");
		if (files == nullptr)
		{
			return 1;
		}

		return files->GetObjects().size();
	}

	std::vector<char> MetadataFile::GetPieceHash(size_t index)
	{
		auto info = GetInfoSection();
		auto pieces = info->GetKey<Bencode::ByteString>("pieces");
		auto& bytes = pieces->GetBytes();

		std::vector<char> result;
		for (auto i = index * 20; i < (index + 1) * 20; i++)
		{
			result.push_back(bytes[i]);
		}

		return result;
	}

	std::string MetadataFile::GetFileName(size_t index)
	{
		auto info = GetInfoSection();
		auto files = info->GetKey<Bencode::List>("files");

		if (files != nullptr)
		{
			if (index >= files->GetObjects().size())
			{
				throw InvalidFileIndex();
			}

			auto fileObj = files->GetObjects()[index];
			assert(fileObj->GetType() == Bencode::ObjectType::DICTIONARY);
			auto file = (Bencode::Dictionary*)fileObj.get();
			auto path = file->GetKey<Bencode::List>("path");

			std::string filename;
			auto count = path->GetObjects().size();

			for (auto i = 0u; i < count; i++)
			{
				auto pathPieceObj = path->GetObjects()[i];
				assert(pathPieceObj->GetType() == Bencode::ObjectType::BYTESTRING);
				auto pathPiece = (Bencode::ByteString*)pathPieceObj.get();

				std::string pathPieceString(pathPiece->GetBytes().begin(), pathPiece->GetBytes().end());
				filename += pathPieceString;

				if (i < count - 1)
				{
					filename += "\\";
				}
			}

			return filename;
		}
		else
		{
			auto filenameKey = info->GetKey<Bencode::ByteString>("name");
			std::string filename(filenameKey->GetBytes().begin(), filenameKey->GetBytes().end());
			return filename;
		}
	}

	uint64_t MetadataFile::GetFileSize(size_t index)
	{
		auto info = GetInfoSection();
		auto files = info->GetKey<Bencode::List>("files");

		if (files != nullptr)
		{
			if (index >= files->GetObjects().size())
			{
				throw InvalidFileIndex();
			}

			auto fileObj = files->GetObjects()[index];
			assert(fileObj->GetType() == Bencode::ObjectType::DICTIONARY);
			auto file = (Bencode::Dictionary*)fileObj.get();
			return file->GetKey<Bencode::Integer>("length")->GetValue();

		}
		else
		{
			return info->GetKey<Bencode::Integer>("length")->GetValue();
		}
	}

	std::pair<uint64_t, uint64_t> MetadataFile::GetFileStart(size_t index)
	{
		auto info = GetInfoSection();
		auto files = info->GetKey<Bencode::List>("files");

		if (files != nullptr)
		{
			if (index >= files->GetObjects().size())
			{
				throw InvalidFileIndex();
			}

			uint64_t pieceSize = GetPieceLength();

			std::vector<uint64_t> fileStarts;

			size_t fileCount = GetFilesCount();
			uint64_t currentPos = 0u;
			for (auto i = 0u; i < fileCount; i++)
			{
				fileStarts.push_back(currentPos);
				currentPos += GetFileSize(i);
			}

			uint64_t startPiece = fileStarts[index] / pieceSize;
			uint64_t pieceOffset = fileStarts[index] % pieceSize;

			return std::make_pair(startPiece, pieceOffset);
		}
		else
		{
			return std::make_pair(0, 0);
		}
	}

	std::pair<uint64_t, uint64_t> MetadataFile::GetFileEnd(size_t index)
	{
		auto info = GetInfoSection();
		auto files = info->GetKey<Bencode::List>("files");

		if (files != nullptr)
		{
			if (index >= files->GetObjects().size())
			{
				throw InvalidFileIndex();
			}

			uint64_t pieceSize = GetPieceLength();

			std::vector<uint64_t> fileEnds;

			size_t fileCount = GetFilesCount();
			uint64_t currentPos = 0u;
			for (auto i = 0u; i < fileCount; i++)
			{
				currentPos += GetFileSize(i);
				fileEnds.push_back(currentPos);
			}

			uint64_t endPiece = fileEnds[index] / pieceSize;
			uint64_t pieceOffset = fileEnds[index] % pieceSize;

			return std::make_pair(endPiece, pieceOffset);
		}
		else
		{
			uint64_t pieceSize = GetPieceLength();
			uint64_t pieceOffset = GetFileSize(0) % pieceSize;
			return std::make_pair(GetPieceCount() - 1, pieceOffset);
		}
	}

	void MetadataFile::PrintInfo()
	{
		LOG(m_Contents->ToString());
	}

	Bencode::Dictionary* MetadataFile::GetInfoSection()
	{
		auto& keys = m_Contents->GetKeys();
		Bencode::Dictionary* info = nullptr;

		for (auto& key : keys)
		{
			if (key.first == "info")
			{
				info = (Bencode::Dictionary*)key.second.get();
				break;
			}
		}

		if (info == nullptr)
		{
			std::cout << "GetInfoSection(): error 'info' section is not present" << std::endl;
			throw InvalidInfoSection();
		}

		return info;
	}

}
