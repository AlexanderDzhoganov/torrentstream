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
		if (info == nullptr)
		{
			return "";
		}

		auto data = info->Encode();

		sha1wrapper sha1;
		return sha1.getHashFromBytes((unsigned char*)data.data(), data.size());
	}

	std::string MetadataFile::GetAnnounceURL()
	{
		auto announceKey = m_Contents->GetKey("announce");
		assert(announceKey->GetType() == Bencode::ObjectType::BYTESTRING);
		auto announceStringBytes = ((Bencode::ByteString*)announceKey.get())->GetBytes();
		return std::string(announceStringBytes.data(), announceStringBytes.size());
	}

	size_t MetadataFile::GetPieceLength()
	{
		auto info = GetInfoSection();
		if (info == nullptr)
		{
			return 0;
		}

		auto pieceLengthKey = info->GetKey("piece length");
		if (pieceLengthKey == nullptr)
		{
			return 0;
		}

		assert(pieceLengthKey->GetType() == Bencode::ObjectType::INTEGER);

		return ((Bencode::Integer*)pieceLengthKey.get())->GetValue();
	}

	size_t MetadataFile::GetPieceCount()
	{
		auto info = GetInfoSection();
		if (info == nullptr)
		{
			return 0;
		}

		size_t totalSize = GetTotalSize();
		size_t pieceLength = GetPieceLength();

		return (size_t)ceil((float)totalSize / (float)pieceLength);
	}

	size_t MetadataFile::GetTotalSize()
	{
		auto info = GetInfoSection();

		size_t totalSize = 0;

		auto filesKey = info->GetKey("files");
		assert(filesKey->GetType() == Bencode::ObjectType::LIST);
		auto files = (Bencode::List*)filesKey.get();
		
		for (auto& fileObj : files->GetObjects())
		{
			assert(fileObj->GetType() == Bencode::ObjectType::DICTIONARY);

			auto file = (Bencode::Dictionary*)fileObj.get();

			auto fileLengthKey = file->GetKey("length");
			assert(fileLengthKey->GetType() == Bencode::ObjectType::INTEGER);

			auto fileLength = ((Bencode::Integer*)fileLengthKey.get())->GetValue();

			totalSize += fileLength;
		}

		return totalSize;
	}

	size_t MetadataFile::GetFilesCount()
	{
		auto info = GetInfoSection();

		size_t filesCount = 0;

		auto filesKey = info->GetKey("files");
		assert(filesKey->GetType() == Bencode::ObjectType::LIST);
		auto files = (Bencode::List*)filesKey.get();

		return files->GetObjects().size();
	}

	std::vector<char> MetadataFile::GetPieceHash(size_t index)
	{
		auto info = GetInfoSection();

		auto piecesKey = info->GetKey("pieces");
		assert(piecesKey->GetType() == Bencode::ObjectType::BYTESTRING);
		auto pieces = (Bencode::ByteString*)piecesKey.get();

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

		auto filesKey = info->GetKey("files");
		assert(filesKey->GetType() == Bencode::ObjectType::LIST);
		auto files = (Bencode::List*)filesKey.get();

		if (index >= files->GetObjects().size())
		{
			return "";
		}

		auto fileObj = files->GetObjects()[index];
		assert(fileObj->GetType() == Bencode::ObjectType::DICTIONARY);
		auto file = (Bencode::Dictionary*)fileObj.get();
		
		auto pathKey = file->GetKey("path");
		assert(pathKey->GetType() == Bencode::ObjectType::LIST);

		auto path = (Bencode::List*)pathKey.get();

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