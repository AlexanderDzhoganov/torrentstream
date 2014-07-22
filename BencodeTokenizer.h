#ifndef __TORRENTSTREAM_BENCODE_TOKENIZER_H
#define __TORRENTSTREAM_BENCODE_TOKENIZER_H

namespace TorrentStream
{

	namespace Bencode
	{

		template <typename T>
		inline T Sub(const T& v, size_t start, size_t end)
		{
			T r;
			r.resize(end - start);

			for (auto i = start; i < end; i++)
			{
				r[i - start] = v[i];
			}

			return r;
		}

		inline size_t ScanAhead(const std::vector<char>& stream, size_t start, char limit)
		{
			assert(start < stream.size());

			for (auto i = start; i < stream.size(); i++)
			{
				if (stream[i] == limit)
				{
					return i - start;
				}
			}

			return stream.size() - start;
		}

		enum class TokenType
		{
			INTEGER,
			LIST_BEGIN,
			LIST_END,
			DICTIONARY_BEGIN,
			DICTIONARY_END,
			BYTES,
		};

		class Token
		{

			public:
			virtual TokenType GetType() = 0;

		};
		
		class IntegerToken : public Token
		{
			public:
			IntegerToken(int value) : m_Value(value) {}

			TokenType GetType()
			{
				return TokenType::INTEGER;
			}

			int GetValue()
			{
				return m_Value;
			}

			private:
			int m_Value = 0;

		};

		class ListBegin : public Token
		{
			public:
			ListBegin(size_t id) : m_ID(id) {}

			TokenType GetType()
			{
				return TokenType::LIST_BEGIN;
			}

			size_t GetID()
			{
				return m_ID;
			}

			private:
			size_t m_ID = 0;

		};

		class ListEnd : public Token
		{
			public:
			ListEnd(size_t id) : m_ID(id) {}
					
			TokenType GetType()
			{
				return TokenType::LIST_END;
			}

			size_t GetID()
			{
				return m_ID;
			}

			private:
			size_t m_ID = 0;

		};
		
		class DictionaryBegin : public Token
		{
			public:
			DictionaryBegin(size_t id) : m_ID(id) {}
				
			TokenType GetType()
			{
				return TokenType::DICTIONARY_BEGIN;
			}

			size_t GetID()
			{
				return m_ID;
			}

			private:
			size_t m_ID = 0;

		};

		class DictionaryEnd : public Token
		{
			public:
			DictionaryEnd(size_t id) : m_ID(id) {}

			TokenType GetType()
			{
				return TokenType::DICTIONARY_END;
			}

			size_t GetID()
			{
				return m_ID;
			}

			private:
			size_t m_ID = 0;

		};

		class Bytes : public Token
		{

			public:
			Bytes(const std::vector<char>& bytes) : m_Bytes(bytes) {}

			TokenType GetType()
			{
				return TokenType::BYTES;
			}

			const std::vector<char>& Get()
			{
				return m_Bytes;
			}

			private:
			std::vector<char> m_Bytes;

		};

		class Tokenizer
		{

			public:
			static std::vector<std::shared_ptr<Token>> Tokenize(const std::vector<char>& stream);

		};

	}

}

#endif
