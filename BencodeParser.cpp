#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <sstream>
#include <memory>
#include <assert.h>

#include "StringFacility.h"
#include "BencodeTokenizer.h"
#include "BencodeParser.h"

namespace TorrentStream
{

	namespace Bencode
	{

		size_t ScanAhead(const std::vector<std::shared_ptr<Token>>& stream, size_t start, TokenType type, size_t id)
		{
			for (auto i = start; i < stream.size(); i++)
			{
				if (stream[i]->GetType() != type)
				{
					continue;
				}
				
				switch (type)
				{
				case TokenType::LIST_END:
					{
						auto lst = (ListEnd*)stream[i].get();
						if (id == lst->GetID())
						{
							return i;
						}
					}
					break;
				case TokenType::DICTIONARY_END:
					{
						auto dct = (DictionaryEnd*)stream[i].get();
						if (id == dct->GetID())
						{
							return i;
						}
					}
					break;
				}
			}

			return stream.size() - 1;
		}

		Integer::Integer(const std::vector<std::shared_ptr<Token>>& stream)
		{
			assert(stream[0]->GetType() == TokenType::INTEGER);
			m_Value = ((IntegerToken*)(stream[0].get()))->GetValue();
		}

		ByteString::ByteString(const std::vector<std::shared_ptr<Token>>& stream)
		{
			assert(stream[0]->GetType() == TokenType::BYTES);

			m_Bytes = ((Bytes*)(stream[0].get()))->Get();
		}

		List::List(const std::vector<std::shared_ptr<Token>>& stream)
		{
			assert(stream[0]->GetType() == TokenType::LIST_BEGIN);
			assert(stream[stream.size() - 1]->GetType() == TokenType::LIST_END);

			for (auto i = 1u; i < stream.size() - 1; i++)
			{
				auto& token = stream[i];
				if (token->GetType() == TokenType::INTEGER)
				{
					m_Objects.push_back(std::make_unique<Integer>(Sub(stream, i, i + 1)));
				}
				else if (token->GetType() == TokenType::BYTES)
				{
					m_Objects.push_back(std::make_unique<ByteString>(Sub(stream, i, i + 1)));
				}
				else if (token->GetType() == TokenType::LIST_BEGIN)
				{
					auto lst = (ListBegin*)token.get();
					auto end = ScanAhead(stream, i, TokenType::LIST_END, lst->GetID());
					m_Objects.push_back(std::make_unique<List>(Sub(stream, i, end + 1)));
					i = end;
				}
				else if (token->GetType() == TokenType::DICTIONARY_BEGIN)
				{
					auto dct = (DictionaryBegin*)token.get();
					auto end = ScanAhead(stream, i, TokenType::DICTIONARY_END, dct->GetID());
					m_Objects.push_back(std::make_unique<Dictionary>(Sub(stream, i, end + 1)));
					i = end;
				}
			}
		}

		Dictionary::Dictionary(const std::vector<std::shared_ptr<Token>>& stream)
		{
			assert(stream[0]->GetType() == TokenType::DICTIONARY_BEGIN);
			assert(stream[stream.size() - 1]->GetType() == TokenType::DICTIONARY_END);

			for (auto i = 1u; i < stream.size() - 1; i++)
			{
				auto& key = stream[i];
				assert(key->GetType() == TokenType::BYTES);
				auto& keyBytes = ((Bytes*)key.get())->Get();
				std::string keyString(keyBytes.data(), keyBytes.size());
				i++;

				auto& token = stream[i];
				if (token->GetType() == TokenType::INTEGER)
				{
					m_Keys.push_back(std::make_pair(keyString, std::make_unique<Integer>(Sub(stream, i, i + 1))));
				}
				else if (token->GetType() == TokenType::BYTES)
				{
					m_Keys.push_back(std::make_pair(keyString, std::make_unique<ByteString>(Sub(stream, i, i + 1))));
				}
				else if (token->GetType() == TokenType::LIST_BEGIN)
				{
					auto lst = (ListBegin*)token.get();
					auto end = ScanAhead(stream, i, TokenType::LIST_END, lst->GetID());
					m_Keys.push_back(std::make_pair(keyString, std::make_unique<List>(Sub(stream, i, end + 1))));
					i = end;
				}
				else if (token->GetType() == TokenType::DICTIONARY_BEGIN)
				{
					auto dct = (DictionaryBegin*)token.get();
					auto end = ScanAhead(stream, i, TokenType::DICTIONARY_END, dct->GetID());
					m_Keys.push_back(std::make_pair(keyString, std::make_unique<Dictionary>(Sub(stream, i, end + 1))));
					i = end;
				}
			}
		}

	}

}