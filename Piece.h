#ifndef __TORRENTSTREAM_PIECE_H
#define __TORRENTSTREAM_PIECE_H

namespace TorrentStream
{

	class Piece
	{

		public:
		Piece(size_t size, const std::vector<char>& hash) : m_Size(size), m_Hash(hash)
		{
		}

		void SubmitData(size_t offset, const std::vector<char>& data)
		{
			m_Data.resize(data.size());
			std::copy(data.begin(), data.end(), m_Data.begin() + offset);
			m_IsComplete = true;
		}

		bool CheckHash()
		{
			return true;
		}
		
		std::vector<char> GetData()
		{
			return m_Data;
		}

		bool IsComplete()
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
