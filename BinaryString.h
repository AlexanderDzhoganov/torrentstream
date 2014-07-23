#ifndef __TORRENTSTREAM_BINARYSTRING_H
#define __TORRENTSTREAM_BINARYSTRING_H

namespace TorrentStream
{

	enum Endianess
	{
		BigEndian,
		LittleEndian,
	};

	template <typename T>
	T ByteSwap(T v)
	{
		return _byteswap_ulong(v);
	}

	class BinaryString
	{

		public:
		void PushInt32(int value, Endianess endianess = LittleEndian)
		{
			int val = value;
			auto dataSize = m_Data.size();
			m_Data.resize(dataSize + sizeof(int));

			if (endianess == BigEndian)
			{
				val = _byteswap_ulong(val);
			}

			memcpy(m_Data.data() + dataSize, &val , sizeof(int));
		}

		void PushUint8(char value)
		{
			m_Data.push_back(value);
		}

		void PushString(const std::string& value)
		{
			auto dataSize = m_Data.size();
			m_Data.resize(dataSize + value.length());
			std::copy(value.begin(), value.end(), m_Data.begin() + dataSize);
		}

		const std::vector<char>& Get() const
		{
			return m_Data;
		}

		private:		
		std::vector<char> m_Data;

	};

}

#endif
