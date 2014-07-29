#ifndef __TORRENTSTREAM_STRINGFACILITY_H
#define __TORRENTSTREAM_STRINGFACILITY_H

#include <iomanip>

namespace TorrentStream
{

	inline std::string url_encode(unsigned char* data)
	{
		std::ostringstream escaped;
		escaped.fill('0');
		escaped << std::hex;

		for (auto i = 0u; i < 20; i++)
		{
			escaped << '%' << std::setw(2) << int((unsigned char)data[i]);
		}

		return escaped.str();
	}

	inline std::string bytesToHex(const char* data, size_t size)
	{
		std::ostringstream hex;
		hex.fill('0');
		hex << std::hex;

		for (auto i = 0u; i < 20; i++)
		{
			hex << std::setw(2) << int((unsigned char)data[i]);
		}

		return hex.str();
	}

	static inline std::vector<std::string> split(const std::string &s, char delim)
	{
		std::vector<std::string> elems;
		std::stringstream ss(s);
		std::string item;
		while (std::getline(ss, item, delim))
		{
			if (delim == '\n')
			{
				if (item[item.length() - 1] == '\r')
				{
					item = item.substr(0, item.length() - 1);
				}
			}

			elems.push_back(item);
		}

		return elems;
	}

	void xs(std::string& result, const char* s);

	template<typename T, typename... Args>
	inline void xs(std::string& result, const char* s, T value, Args... args)
	{
		while (*s)
		{
			if (*s == '%')
			{
				if (*(s + 1) == '%')
				{
					++s;
				}
				else
				{
					std::stringstream stream;
					stream << value;
					result += stream.str();
					xs(result, s + 1, args...); // call even when *s == 0 to detect extra arguments
					return;
				}
			}
			result += *s++;
		}
		throw std::logic_error("extra arguments provided to printf");
	}

	template<typename... Args>
	inline std::string xs(const char* s, Args... args)
	{
		std::string buf;
		xs(buf, s, args...);
		return buf;
	}

	inline void xs(std::string& result, const char* s)
	{
		while (*s)
		{
			if (*s == '%')
			{
				if (*(s + 1) == '%')
				{
					++s;
				}
				else
				{
					throw std::runtime_error("invalid format string: missing arguments");
				}
			}

			result += *s++;
		}
	}

}

#endif
