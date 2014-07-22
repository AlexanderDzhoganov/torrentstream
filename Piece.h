#ifndef __TORRENTSTREAM_PIECE_H
#define __TORRENTSTREAM_PIECE_H

namespace TorrentStream
{

	class Piece
	{

		public:
		Piece(size_t size, const std::string& hash) : m_Size(size), m_Hash(hash) {}

		void SubmitData(const std::vector<char>& data)
		{
			m_Data.reserve(m_Data.size() + data.size());

			for (auto c : data)
			{
				m_Data.push_back(c);
				m_DataPtr++;
			}
		}

		size_t GetDataPtr()
		{
			return m_DataPtr;
		}

		bool IsCompleted()
		{
			return (m_Size - m_DataPtr) == 0;
		}

		bool CheckHash()
		{
			return true;
		}
		
		const std::vector<char> GetData()
		{
			return m_Data;
		}

		private:
		size_t m_Size = 0;
		std::string m_Hash;

		size_t m_DataPtr = 0;
		std::vector<char> m_Data;

	};

}

#endif
