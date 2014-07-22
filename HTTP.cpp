#include <vector>
#include <iostream>
#include <string>
#include <regex>
#include <assert.h>
#include <sstream>
#include <unordered_map>

#include "StringFacility.h"
#include "Socket.h"
#include "HTTP.h"

namespace TorrentStream
{

	namespace HTTP
	{

		URL ParseURL(const std::string& url)
		{
			URL result;

			std::regex regex("^(?:http://)?([^/]+)(?:/?.*/?)/(.*)$");
		
			std::match_results<std::string::const_iterator> what;
			if (!std::regex_search(url, what, regex))
			{
				throw InvalidURL();
				
			}

			std::string base_url(what[1].first, what[1].second);
			auto query = split(std::string(what[2].first, what[2].second), '?');

			result.scheme = "http";
			result.authority = base_url;

			result.path = query[0];
			
			auto args = split(query[1], '&');
			for (auto& keyval : args)
			{
				auto kv = split(keyval, '=');
				auto key = kv[0];
				std::string val = "";
				if (kv.size() > 1)
				{
					val = kv[1];
				}

				result.arguments[key] = val;
			}

			return result;
		}

		HTTPResponse DoHTTPRequest(const URL& url)
		{
			assert(url.scheme == "http");

			Socket::Socket socket;
			if (!socket.Open(url.authority, "80"))
			{
				throw FailedToConnect();
			}

			std::string request = xs("GET /%?% HTTP/1.0\r\nHost: %\r\nUser-Agent: Unspecified\r\n\r\n", url.path, url.GetArgumentsString(), url.authority);

			auto sent = socket.Send(std::vector<char>(request.begin(), request.end()));

			std::vector<char> resp;
			resp.resize(32000);

			auto bytesReceived = socket.Receive(resp, 32000);

			auto rawResponse = std::string(resp.data(), bytesReceived);

			auto response = split(rawResponse, '\n');
			auto status = split(response[0], ' ');
			auto statusCode = std::stoi(status[1]);

			bool inContent = false;
			std::string content = "";

			for (auto& line : response)
			{
				if (inContent)
				{
					content = content + line + "\n";
				}
				else
				{
					if (line == "")
					{
						inContent = true;
					}
				}
			}

			bool inData = false;
			std::vector<char> responseV;
			for (auto i = 0u; i < bytesReceived; i++)
			{
				if (resp[i] == '\r' && resp[i + 1] == '\n' && resp[i + 2] == '\r' && resp[i + 3] == '\n')
				{
					inData = true;
					i += 4;
				}

				if (inData)
				{
					responseV.push_back(resp[i]);
				}
			}

			HTTPResponse result;
			result.statusCode = statusCode;
			result.content = responseV;

			return result;
		}

	}

}
