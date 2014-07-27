#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <memory>
#include <deque>

#include <boost/asio.hpp>
#include "ASIOThreadPool.h"

#include "HTTPServe.h"

namespace TorrentStream
{

	namespace HTTP
	{

		HTTPServer::HTTPServer(size_t port) : m_Port(port)
		{
			m_Thread = std::make_unique<std::thread>(std::bind(&HTTPServer::RunThread, this));
		}

		void HTTPServer::RunThread()
		{
			using boost::asio::ip::tcp;

			m_Acceptor = std::make_unique<tcp::acceptor>(ASIO::ThreadPool::Instance().GetService(), tcp::endpoint(tcp::v4(), m_Port));
			m_Socket = std::make_unique<tcp::socket>(ASIO::ThreadPool::Instance().GetService());
			m_Acceptor->accept(*m_Socket);

			boost::asio::streambuf buf;
			std::ostream stream(&buf);

			stream << "HTTP/1.1 200 OK\r\n";
			stream << "Content-Type: application/octet-stream\r\n";
			stream << "Content-Length: 734003200\r\n";
			stream << "\r\n";
			
			boost::asio::write(*m_Socket, buf);
			
			for (;;)
			{
				std::unique_lock<std::mutex> _(m_Mutex);

				if (m_Data.size() == 0)
				{
					continue;
				}

				auto item = m_Data.front();
				m_Data.pop_front();

				if (item.size() == 0)
				{
					continue;
				}

				auto buffer = boost::asio::buffer(item.data(), item.size());
				boost::system::error_code ec;
				boost::asio::write(*m_Socket, buffer, ec);

				if (ec)
				{
					std::cout << "Http error: " << ec << std::endl;
					return;
				}

				std::this_thread::yield();
			}
		}


	}

}