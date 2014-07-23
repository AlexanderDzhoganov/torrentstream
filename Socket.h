#ifndef __TORRENTSTREAM_SOCKET_H
#define __TORRENTSTREAM_SOCKET_H

namespace TorrentStream
{

	namespace Socket
	{

		class SocketError : public std::exception {};

		struct SocketImpl;

		class Socket
		{
			public:
			Socket();
			~Socket();

			bool Open(const std::string& ip, const std::string& port);
			bool IsOpen();

			bool Send(const std::vector<char>& data);

			int Receive(void* data, int size);

			int Receive(std::vector<char>& data, int size);

			template <typename T>
			inline int Receive(T& data)
			{
				return Receive((void*)&data, sizeof(T));
			}

			int Receive(std::string& data, int size);

			private:
			SocketImpl* m_Impl;

		};

	}

}

#endif
