#ifndef __TORRENTSTREAM_PIECE_H
#define __TORRENTSTREAM_PIECE_H

namespace TorrentStream
{

	class Piece
	{

		public:
		Piece(size_t size) : m_Size(size)
		{
		}

		Piece(size_t size, const std::vector<char>& hash) : m_Size(size), m_Hash(hash)
		{
		}

		Piece(const Piece&) = delete;

		Piece(Piece&& p)
		{
			m_Data = std::move(p.m_Data);
			m_Hash = p.m_Hash;
			m_Size = p.m_Size;
			m_IsComplete = p.m_IsComplete;
			m_IsWrittenOut = p.m_IsWrittenOut;
		}

		void SubmitData(size_t offset, const std::vector<char>& data)
		{
			if (m_Data == nullptr)
			{
				m_Data = std::make_unique<std::vector<char>>();
			}

			if (offset + data.size() > m_Data->size())
			{
				m_Data->resize(offset + data.size());
			}

			if (offset + data.size() > m_Size)
			{
				std::cout << "data out of bounds" << std::endl;
				return;
			}

			memcpy(m_Data->data() + offset, data.data(), data.size());
			m_SubmittedBytes += data.size();
			m_SubmittedOffsets.push_back(offset);
		}

		const std::vector<char>& GetHash() const
		{
			return m_Hash;
		}

		void SetHash(const std::vector<char>& hash)
		{
			m_Hash = hash;
		}
		
		const std::vector<char>& GetData() const
		{
			return *m_Data;
		}

		bool IsComplete() const
		{
			return m_IsComplete;
		}

		void SetComplete(bool state)
		{
			m_IsComplete = state;
		}

		bool IsWrittenOut()
		{
			return m_IsWrittenOut;
		}

		void SetWrittenOut(bool state)
		{
			m_IsWrittenOut = state;
			if (m_IsWrittenOut)
			{
				m_Data = nullptr;
			}
		}

		private:
		size_t m_Size = 0;
		size_t m_SubmittedBytes = 0;
		std::vector<size_t> m_SubmittedOffsets;
		std::vector<char> m_Hash;

		std::unique_ptr<std::vector<char>> m_Data;
		bool m_IsComplete = false;
		bool m_IsWrittenOut = false;

	};

}

#endif
