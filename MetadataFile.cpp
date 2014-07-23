#define _CRT_SECURE_NO_WARNINGS

#include <vector>
#include <string>
#include <sstream>
#include <memory>
#include <unordered_map>
#include <assert.h>
#include <iostream>

#include "StringFacility.h"
#include "BencodeTokenizer.h"
#include "BencodeParser.h"
#include "MetadataFile.h"
#include "../extern/hashlib2plus/hashlibpp.h"

namespace TorrentStream
{

	MetadataFile::MetadataFile(const std::vector<char>& contents)
	{
		auto tokenized = Bencode::Tokenizer::Tokenize(contents);
		m_Contents = std::make_unique<Bencode::Dictionary>(tokenized);
	}

	std::string MetadataFile::GetInfoHash()
	{
		auto info = GetInfoSection();
		auto data = info->Encode();
		return sha1wrapper().getHashFromBytes((unsigned char*)data.data(), data.size());
	}

	std::string MetadataFile::GetAnnounceURL()
	{
		auto announceKey = m_Contents->GetKey<Bencode::ByteString>("announce");
		auto announceStringBytes = announceKey->GetBytes();
		return std::string(announceStringBytes.data(), announceStringBytes.size());
	}

	size_t MetadataFile::GetPieceLength()
	{
		auto info = GetInfoSection();
		auto pieceLength = info->GetKey<Bencode::Integer>("piece length");
		return pieceLength->GetValue();
	}

	size_t MetadataFile::GetPieceCount()
	{
		return (size_t)ceil((float)GetTotalSize() / (float)GetPieceLength());
	}

	size_t MetadataFile::GetTotalSize()
	{
		auto info = GetInfoSection();
		size_t totalSize = 0;
		auto files = info->GetKey<Bencode::List>("files");
		
		for (auto& fileObj : files->GetObjects())
		{
			assert(fileObj->GetType() == Bencode::ObjectType::DICTIONARY);

			auto file = (Bencode::Dictionary*)fileObj.get();
			auto fileLength = file->GetKey<Bencode::Integer>("length")->GetValue();
			totalSize += fileLength;
		}

		return totalSize;
	}

	size_t MetadataFile::GetFilesCount()
	{
		auto info = GetInfoSection();
		auto files = info->GetKey<Bencode::List>("files");
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

	size_t MetadataFile::GetFileSize(size_t index)
	{
		auto info = GetInfoSection();
		auto files = info->GetKey<Bencode::List>("files");

		if (index >= files->GetObjects().size())
		{
			throw InvalidFileIndex();
		}

		auto fileObj = files->GetObjects()[index];
		assert(fileObj->GetType() == Bencode::ObjectType::DICTIONARY);
		auto file = (Bencode::Dictionary*)fileObj.get();
		return file->GetKey<Bencode::Integer>("length")->GetValue();
	}

	std::pair<size_t, size_t> MetadataFile::GetFileStart(size_t index)
	{
		auto info = GetInfoSection();
		auto files = info->GetKey<Bencode::List>("files");

		if (index >= files->GetObjects().size())
		{
			throw InvalidFileIndex();
		}

		size_t pieceSize = GetPieceLength();

		std::vector<size_t> fileStarts;

		size_t fileCount = GetFilesCount();
		auto currentPos = 0u;
		for (auto i = 0u; i < fileCount; i++)
		{
			fileStarts.push_back(currentPos);
			currentPos += GetFileSize(i);
		}
		
		size_t startPiece = fileStarts[index] / pieceSize;
		size_t pieceOffset = fileStarts[index] % pieceSize;

		return std::make_pair(startPiece, pieceOffset);
	}

	std::pair<size_t, size_t> MetadataFile::GetFileEnd(size_t index)
	{
		auto info = GetInfoSection();
		auto files = info->GetKey<Bencode::List>("files");

		if (index >= files->GetObjects().size())
		{
			throw InvalidFileIndex();
		}

		size_t pieceSize = GetPieceLength();

		std::vector<size_t> fileEnds;

		size_t fileCount = GetFilesCount();
		auto currentPos = 0u;
		for (auto i = 0u; i < fileCount; i++)
		{
			currentPos += GetFileSize(i);
			fileEnds.push_back(currentPos);
		}

		size_t endPiece = fileEnds[index] / pieceSize;
		size_t pieceOffset = fileEnds[index] % pieceSize;

		return std::make_pair(endPiece, pieceOffset);
	}

	void MetadataFile::PrintInfo()
	{
		std::cout << m_Contents->ToString() << std::endl;
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