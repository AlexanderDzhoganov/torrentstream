#include <winsock2.h>
#include <ws2tcpip.h>

#include <memory>
#include <iostream>
#include <vector>
#include <string>
#include <assert.h>

#include "Socket.h"

#pragma comment(lib, "Ws2_32.lib")

namespace TorrentStream
{

	namespace Socket
	{
			
		namespace WSAInternals
		{
			WSAData wsaData;
		}

		struct SocketImpl
		{
			SOCKET m_Socket;
			bool m_IsOpen = false;
		};

		Socket::Socket()
		{
			m_Impl = new SocketImpl;

			static bool m_WSAInitialized = false;

			if (!m_WSAInitialized)
			{
				auto result = WSAStartup(MAKEWORD(2, 2), &WSAInternals::wsaData);
				if (result != 0)
				{
					std::cout << "WSAStartup failed" << std::endl;
				}
			}
		}

		Socket::~Socket()
		{
			if (m_Impl->m_IsOpen)
			{
				closesocket(m_Impl->m_Socket);
			}

			delete m_Impl;
		}

		bool Socket::Open(const std::string& ip, const std::string& port)
		{
			struct addrinfo* address = nullptr, hints;

			ZeroMemory(&hints, sizeof(hints));
			hints.ai_family = AF_UNSPEC;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_protocol = IPPROTO_TCP;

			auto result = getaddrinfo(ip.c_str(), port.c_str(), &hints, &address);
			if (result != 0)
			{
				WSACleanup();
				return false;
			}

			m_Impl->m_Socket = socket(address->ai_family, address->ai_socktype, address->ai_protocol);

			if (m_Impl->m_Socket == INVALID_SOCKET)
			{
				freeaddrinfo(address);
				WSACleanup();
				return 1;
			}

			result = connect(m_Impl->m_Socket, address->ai_addr, (int)address->ai_addrlen);
			if (result == SOCKET_ERROR)
			{
				closesocket(m_Impl->m_Socket);
				m_Impl->m_Socket = INVALID_SOCKET;
				return false;
			}

			freeaddrinfo(address);

			if (m_Impl->m_Socket == INVALID_SOCKET)
			{
				WSACleanup();
				return false;
			}

			m_Impl->m_IsOpen = true;
			return true;
		}

		bool Socket::IsOpen()
		{
			return m_Impl->m_IsOpen;
		}

		bool Socket::Send(const std::vector<char>& data)
		{
			int sent = 0;
			while (sent < (int)data.size())
			{
				auto result = send(m_Impl->m_Socket, data.data() + sent, data.size() - sent, 0);

				if (result == 0)
				{
					m_Impl->m_IsOpen = false;
					return false;
				}

				if (result < 0)
				{
					std::cout << "Socket send() error: " << WSAGetLastError() << std::endl;
					return false;
				}

				sent += result;
			}

			return true;
		}

		int Socket::Receive(void* data, int size)
		{
			size_t total = 0;

			while (total < size)
			{
				auto result = recv(m_Impl->m_Socket, (char*)data + total, size - total, 0);
				if (result <= 0)
				{
					return total;
				}

				total += result;
			}

			return total;
		}

		int Socket::Receive(std::vector<char>& data, int size)
		{
			if ((int)data.size() < size)
			{
				data.resize(size);
			}

			auto result = Receive((void*)data.data(), size);
			data.resize(result);
			return result;
		}

		int Socket::Receive(std::string& data, int size)
		{
			data.resize(size);
			return Receive((void*)data.c_str(), size);
		}

	}

}
