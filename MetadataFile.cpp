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
			std::cout << "GetInfoHash(): error 'info' section is not present" << std::endl;
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

	void MetadataFile::PrintInfo()
	{
		std::cout << m_Contents->ToString() << std::endl;
	}

}