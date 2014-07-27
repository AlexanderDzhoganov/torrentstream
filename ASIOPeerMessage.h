#ifndef __TORRENTSTREAM_ASIO_PEERMESSAGE_H
#define __TORRENTSTREAM_ASIO_PEERMESSAGE_H

namespace TorrentStream
{

	namespace ASIO
	{

		namespace Peer
		{
	
			enum class MessageType
			{
				Choke = 0,
				Unchoke = 1,
				Interested = 2,
				NotInterested = 3,
				Have = 4,
				Bitfield = 5,
				Request = 6,
				Piece = 7,
				Cancel = 8,
				Port = 9,
				KeepAlive = 10,
			};

			inline uint32_t ReadUint32(const std::vector<char>& data, size_t offset)
			{
				uint32_t result = 0;
				memcpy(&result, data.data() + offset, 4);
				result = _byteswap_ulong(result);
				return result;
			}

			inline std::vector<char> ReadData(const std::vector<char>& data, size_t offset, size_t length)
			{
				std::vector<char> result;
				result.resize(length);
				memcpy(result.data(), data.data() + offset, length);
				return result;
			}

			inline void WriteUint32(std::vector<char>& data, uint32_t value)
			{
				auto ptr = data.size();
				data.resize(ptr + 4);
				uint32_t ntvalue = _byteswap_ulong(value);
				memcpy(data.data() + ptr, &ntvalue, 4);
			}

			inline void WriteData(std::vector<char>& dstData, const std::vector<char>& srcData)
			{
				dstData.insert(dstData.end(), srcData.begin(), srcData.end());
			}

			class Message
			{

				public:
				Message(MessageType type) : m_Type(type)
				{
				}

				Message(MessageType type, const std::vector<char>& payload) : m_Type(type), m_Payload(payload)
				{
				}

				MessageType GetType() const
				{
					return m_Type;
				}

				const std::vector<char>& GetPayload() const
				{
					return m_Payload;
				}

				std::vector<char> m_Payload;
				std::vector<char> m_Header;

				private:
				MessageType m_Type = MessageType::KeepAlive;

			};

			class KeepAlive : public Message
			{

				public:
				KeepAlive() : Message(MessageType::KeepAlive)
				{
				}

			};

			class Choke : public Message
			{

				public:
				Choke() : Message(MessageType::Choke)
				{
				}

			};

			class Unchoke : public Message
			{

				public:
				Unchoke() : Message(MessageType::Unchoke)
				{
				}
	
			};

			class Interested : public Message
			{

				public:
				Interested() : Message(MessageType::Interested)
				{
				}

			};

			class NotInterested : public Message
			{

				public:
				NotInterested() : Message(MessageType::NotInterested)
				{
				}
	
			};

			class Have : public Message
			{

				public:
				Have(uint32_t index) : Message(MessageType::Have)
				{
					WriteUint32(m_Payload, index);
				}

				Have(const std::vector<char>& payload) : Message(MessageType::Have, payload)
				{
				}

				uint32_t GetIndex()
				{
					return ReadUint32(m_Payload, 0);
				}
	
			};

			class Bitfield : public Message
			{

				public:
				Bitfield(const std::vector<char>& payload) : Message(MessageType::Bitfield, payload)
				{
				}

				std::vector<bool> GetBitfield()
				{
					std::vector<bool> result;
					result.resize(m_Payload.size() * 8);

					for (auto i = 0u; i < m_Payload.size() * 8; i++)
					{
						result[i] = (m_Payload[i / 8] & (1 << (8 - i % 8))) != 0;
					}

					return result;
				}
		
			};

			class Request : public Message
			{

				public:
				Request(uint32_t index, uint32_t begin, uint32_t len) : Message(MessageType::Request)
				{
					WriteUint32(m_Payload, index);
					WriteUint32(m_Payload, begin);
					WriteUint32(m_Payload, len);
				}

				Request(const std::vector<char>& payload) : Message(MessageType::Request, payload)
				{
				}

				uint32_t GetIndex()
				{
					return ReadUint32(m_Payload, 0);
				}

				uint32_t GetBegin()
				{
					return ReadUint32(m_Payload, 4);
				}

				uint32_t GetLength()
				{
					return ReadUint32(m_Payload, 8);
				}

			};

			class Piece : public Message
			{

				public:
				Piece(uint32_t index, uint32_t begin, const std::vector<char>& block) : Message(MessageType::Piece)
				{
					WriteUint32(m_Payload, index);
					WriteUint32(m_Payload, begin);
					WriteData(m_Payload, block);
				}

				Piece(const std::vector<char>& payload) : Message(MessageType::Piece, payload)
				{
				}

				uint32_t GetIndex()
				{
					return ReadUint32(m_Payload, 0);
				}

				uint32_t GetBegin()
				{
					return ReadUint32(m_Payload, 4);
				}

				std::vector<char> GetBlock()
				{
					return ReadData(m_Payload, 8, m_Payload.size() - 8);
				}

			};

			class Cancel : public Message
			{

				public:
				Cancel(uint32_t index, uint32_t begin, uint32_t length) : Message(MessageType::Cancel)
				{
					WriteUint32(m_Payload, index);
					WriteUint32(m_Payload, begin);
					WriteUint32(m_Payload, length);
				}

				Cancel(const std::vector<char>& payload) : Message(MessageType::Cancel, payload)
				{
				}

				size_t GetIndex()
				{
					return ReadUint32(m_Payload, 0);
				}

				size_t GetBegin()
				{
					return ReadUint32(m_Payload, 4);
				}

				size_t GetLength()
				{
					return ReadUint32(m_Payload, 8);
				}

			};

			class Port : public Message
			{

				public:
				Port(uint32_t port) : Message(MessageType::Port)
				{
					WriteUint32(m_Payload, port);
				}

				Port(const std::vector<char>& payload) : Message(MessageType::Port, payload)
				{
				}

				size_t GetPort()
				{
					return ReadUint32(m_Payload, 0);
				}

			};

		}

	}

}

#endif
