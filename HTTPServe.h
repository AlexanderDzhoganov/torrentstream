#ifndef __TORRENTSTREAM_HTTP_SERVE_H
#define __TORRENTSTREAM_HTTP_SERVE_H

namespace TorrentStream
{

	namespace HTTP
	{

		class HTTPServer
		{

			public:
			HTTPServer(size_t port);

			void EnqueueData(const std::vector<char>& data)
			{
				std::unique_lock<std::mutex> _(m_Mutex);
				m_Data.push_back(data);
			}

			private:
			void RunThread();

			std::mutex m_Mutex;
			size_t m_Port = 0;
			std::unique_ptr<std::thread> m_Thread;

			std::deque<std::vector<char>> m_Data;

			std::unique_ptr<boost::asio::ip::tcp::acceptor> m_Acceptor;
			std::unique_ptr<boost::asio::ip::tcp::socket> m_Socket;

		};

	}

}

#endif
