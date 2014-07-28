#include <vector>
#include <iostream>
#include <string>
#include <deque>
#include <regex>
#include <mutex>
#include <thread>
#include <assert.h>
#include <sstream>
#include <unordered_map>

#include <boost/asio.hpp>

#include "Log.h"

#include "ASIOThreadPool.h"
#include "StringFacility.h"
#include "HTTP.h"

using boost::asio::ip::tcp;

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
			
			if (query.size() > 1)
			{
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
			}

			return result;
		}

		HTTPResponse DoHTTPRequest(const URL& url)
		{
			assert(url.scheme == "http");

			auto& service = ASIO::ThreadPool::Instance().GetService();
			tcp::socket socket(service);
			tcp::resolver resolver(service);

			auto ipPort = split(url.authority, ':');

			std::string ip = ipPort[0];
			std::string port;

			if (ipPort.size() == 1)
			{
				port = "80";
			}
			else
			{
				port = ipPort[1];
			}

			boost::asio::connect(socket, resolver.resolve({ ip, port }));

			boost::asio::streambuf request;
			std::ostream request_stream(&request);

			request_stream << "GET /" << url.path << "?" << url.GetArgumentsString() << " HTTP/1.0\r\n";
			request_stream << "Host: " << url.authority << "\r\n\r\n";

			boost::asio::write(socket, request);

			boost::system::error_code error;
			boost::asio::streambuf buffer;
			boost::asio::read_until(socket, buffer, "\r\n", error);

			std::istream stream(&buffer);
			
			std::string httpVersion;
			stream >> httpVersion;
			unsigned int statusCode;
			stream >> statusCode;

			boost::asio::read_until(socket, buffer, "\r\n\r\n", error);

			std::string header;
			while (std::getline(stream, header) && header != "\r");

			while (boost::asio::read(socket, buffer, boost::asio::transfer_at_least(1), error))
			{
				if (error == boost::asio::error::eof)
				{
					break;
				}
			}

			HTTPResponse result;
			result.statusCode = statusCode;
			result.content = std::vector<char>(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
			return result;
		}

	}

}
