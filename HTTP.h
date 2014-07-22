#ifndef __TORRENTSTREAM_HTTP_H
#define __TORRENTSTREAM_HTTP_H

namespace TorrentStream
{

	namespace HTTP
	{
		
		class InvalidURL : public std::exception {};

		class FailedToConnect : public std::exception {};

		struct URL
		{
			std::string scheme;
			std::string authority;
			std::string path;
			std::unordered_map<std::string, std::string> arguments;

			std::string GetArgumentsString() const
			{
				std::string result = "";

				if (arguments.size() == 0)
				{
					return result;
				}

				for (auto& pair : arguments)
				{
					result = xs("%&%=%", result, pair.first, pair.second);
				}

				return result.substr(1, result.size() - 1);
			}
		};

		struct HTTPResponse
		{
			int statusCode = 0;
			std::vector<char> content;
		};

		URL ParseURL(const std::string& url);

		HTTPResponse DoHTTPRequest(const URL& url);

	}

}

#endif
