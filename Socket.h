#ifndef __TORRENTSTREAM_SOCKET_H
#define __TORRENTSTREAM_SOCKET_H

namespace TorrentStream
{

	namespace Socket
	{

		struct SocketImpl;

		class Socket
		{
			public:
			Socket();
			~Socket();

			bool Open(const std::string& ip, const std::string& port);
			bool IsOpen();

			bool Send(const std::vector<char>& data);
			int Receive(std::vector<char>& data, int size);

			private:
			SocketImpl* m_Impl;

		};

	}

}

#endif
