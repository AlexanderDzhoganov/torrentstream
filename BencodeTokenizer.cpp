#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <sstream>
#include <memory>
#include <fstream>
#include <assert.h>

#include "StringFacility.h"
#include "BencodeTokenizer.h"

namespace TorrentStream
{

	namespace Bencode
	{

		std::vector<std::shared_ptr<Token>> Tokenizer::Tokenize(const std::vector<char>& stream)
		{
			std::vector<std::shared_ptr<Token>> tokenized;
			std::vector<std::pair<size_t, TokenType>> endStack;

			size_t id = 0;

			for (auto i = 0u; i < stream.size(); i++)
			{
				if (stream[i] == 'i')
				{
					auto len = ScanAhead(stream, i, 'e') - 1;

					std::string integerString;
					integerString.resize(len);

					i++;
					for (auto q = 0u; q < len; q++)
					{
						integerString[q] = stream[i + q];
					}

					i += len;

					size_t value = std::stoull(integerString);
					auto integer = std::make_unique<IntegerToken>(value);

					tokenized.push_back(std::move(integer));
				}
				else if (stream[i] == 'l')
				{
					tokenized.push_back(std::make_unique<ListBegin>(id));
					endStack.push_back(std::make_pair(id, TokenType::LIST_END));
					id++;
				}
				else if (stream[i] == 'd')
				{
					tokenized.push_back(std::make_unique<DictionaryBegin>(id));
					endStack.push_back(std::make_pair(id, TokenType::DICTIONARY_END));
					id++;
				}
				else if (stream[i] == 'e')
				{
					auto back = endStack.back();
					endStack.pop_back();

					switch (back.second)
					{
					case TokenType::LIST_END:
						tokenized.push_back(std::make_unique<ListEnd>(back.first));
						break;
					case TokenType::DICTIONARY_END:
						tokenized.push_back(std::make_unique<DictionaryEnd>(back.first));
						break;
					}
				}
				else
				{
					auto len = ScanAhead(stream, i, ':');
					std::string byteStringLength;
					byteStringLength.resize(len);

					for (auto q = 0u; q < len; q++)
					{
						byteStringLength[q] = stream[i + q];
					}

					i += len + 1;
					size_t size = std::stoi(byteStringLength);

					std::vector<char> byteStringData;
					byteStringData.resize(size);

					for (auto q = 0u; q < size; q++)
					{
						byteStringData[q] = stream[i + q];
					}

					i += size - 1;

					auto bytesToken = std::make_unique<Bytes>(byteStringData);
					tokenized.push_back(std::move(bytesToken));
				}
			}

			return tokenized;
		}

	}

}