#ifndef __TORRENTSTREAM_H
#define __TORRENTSTREAM_H

namespace TorrentStream
{

	namespace Bencode
	{

		enum class ObjectType
		{
			INTEGER = 0,
			BYTESTRING,
			LIST,
			DICTIONARY,
		};

		class Object
		{
			public:
			virtual ObjectType GetType() = 0;
			virtual std::string ToString() = 0;
			virtual std::vector<char> Encode() = 0;
		};

		class Integer : public Object
		{

			public:
			Integer(int value) : m_Value(value) {}

			Integer(const std::vector<std::shared_ptr<Token>>& stream);

			ObjectType GetType()
			{
				return ObjectType::INTEGER;
			}

			std::string ToString()
			{
				std::stringstream s;
				s << m_Value;
				return s.str();
			}

			int GetValue() const
			{
				return m_Value;
			}

			std::vector<char> Encode()
			{
				auto encoded = xs("i%e", m_Value);
				return std::vector<char>(encoded.begin(), encoded.end());
			}

			private:
			int m_Value = 0;

		};

		class ByteString : public Object
		{

			public:
			ByteString(const std::vector<char>& bytes) : m_Bytes(bytes) {}

			ByteString(const std::vector<std::shared_ptr<Token>>& stream);

			ObjectType GetType()
			{
				return ObjectType::BYTESTRING;
			}

			std::string ToString()
			{
				if (m_Bytes.size() > 256)
				{
					return "ByteString too long";
				}

				std::stringstream s;
				s << "\"";
				s << std::string(m_Bytes.data(), m_Bytes.size());
				s << "\"";
				return s.str();
			}

			const std::vector<char>& GetBytes() const
			{
				return m_Bytes;
			}

			std::vector<char> Encode()
			{
				auto len = xs("%:", m_Bytes.size());
				std::vector<char> encoded;
				encoded.reserve(len.size() + m_Bytes.size());

				for (auto c : len)
				{
					encoded.push_back(c);
				}

				for (auto c : m_Bytes)
				{
					encoded.push_back(c);
				}

				return encoded;
			}

			private:
			std::vector<char> m_Bytes;

		};

		class List : public Object
		{

			public:
			List(const std::vector<std::shared_ptr<Token>>& stream);

			ObjectType GetType()
			{
				return ObjectType::LIST;
			}

			std::string ToString()
			{
				std::stringstream s;

				s << "[ ";

				for (auto i = 0u; i < m_Objects.size() - 1; i++)
				{
					s << m_Objects[i]->ToString() << ", ";
				}

				if (m_Objects.size() != 0)
				{
					s << m_Objects[m_Objects.size() - 1]->ToString();
				}

				s << " ]";

				return s.str();
			}

			void Insert(std::shared_ptr<Object> object)
			{
				m_Objects.push_back(std::move(object));
			}

			const std::vector<std::shared_ptr<Object>>& GetObjects() const
			{
				return m_Objects;
			}

			std::vector<char> Encode()
			{
				std::vector<char> encoded;
				encoded.push_back('l');

				for (auto& obj : m_Objects)
				{
					auto encodedObj = obj->Encode();
					for (auto c : encodedObj)
					{
						encoded.push_back(c);
					}
				}

				encoded.push_back('e');
				return encoded;
			}

			private:
			std::vector<std::shared_ptr<Object>> m_Objects;
		};

		class Dictionary : public Object
		{

			public:
			Dictionary(const std::vector<std::shared_ptr<Token>>& stream);

			ObjectType GetType()
			{
				return ObjectType::DICTIONARY;
			}

			std::string ToString()
			{
				std::stringstream s;

				s << "{ ";

				auto counter = 0u;
				for (auto& obj : m_Keys)
				{
					s << "\"" << obj.first << "\": " << obj.second->ToString();
					if (counter != m_Keys.size() - 1)
					{
						s << ", ";
					}

					counter++;
				}

				s << " }";

				return s.str();
			}

			const std::vector<std::pair<std::string, std::shared_ptr<Object>>>& GetKeys()
			{
				return m_Keys;
			}

			std::shared_ptr<Object> GetKey(const std::string& key)
			{
				for (auto& pair : m_Keys)
				{
					if (pair.first == key)
					{
						return pair.second;
					}
				}

				return nullptr;
			}

			std::vector<char> Encode()
			{
				std::vector<char> encoded;
				encoded.push_back('d');

				for (auto& pair : m_Keys)
				{
					auto& key = pair.first;
					auto& obj = pair.second;

					auto len = xs("%:", key.length());

					for (auto c : len)
					{
						encoded.push_back(c);
					}

					for (auto c : key)
					{
						encoded.push_back(c);
					}

					auto encodedObj = obj->Encode();
					for (auto c : encodedObj)
					{
						encoded.push_back(c);
					}
				}

				encoded.push_back('e');
				return encoded;
			}

			private:
			void Insert(const std::string& key, std::shared_ptr<Object> object)
			{
				m_Keys.push_back(std::make_pair(key, std::move(object)));
			}

			std::vector<std::pair<std::string, std::shared_ptr<Object>>> m_Keys;

		};

	}

}

#endif
