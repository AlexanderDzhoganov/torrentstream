#ifndef __TORRENTSTREAM_PIECE_H
#define __TORRENTSTREAM_PIECE_H

namespace TorrentStream
{

	class Piece
	{

		public:
		Piece(size_t size) : m_Size(size)
		{
			m_Data.resize(m_Size);
		}

		Piece(size_t size, const std::vector<char>& hash) : m_Size(size), m_Hash(hash)
		{
		}

		void SubmitData(size_t offset, const std::vector<char>& data)
		{
			if (offset + data.size() > m_Size)
			{
				std::cout << "data out of bounds" << std::endl;
				return;
			}

			memcpy(m_Data.data() + offset, data.data(), data.size());
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
			return m_Data;
		}

		bool IsComplete() const
		{
			return m_IsComplete;
		}

		void SetComplete(bool state)
		{
			m_IsComplete = state;
		}

		private:
		size_t m_Size = 0;
		std::vector<char> m_Hash;

		std::vector<char> m_Data;
		bool m_IsComplete = false;

	};

}

#endif
