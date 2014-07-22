#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <ctime>
#include <iostream>
#include <assert.h>
#include <memory>
#include <thread>
#include <mutex>

#include "StringFacility.h"
#include "Socket.h"
#include "HTTP.h"
#include "BencodeTokenizer.h"
#include "BencodeParser.h"
#include "MetadataFile.h"
#include "Peer.h"
#include "Piece.h"
#include "File.h"
#include "Tracker.h"

namespace TorrentStream
{

	void Peer::RunThread()
	{
		m_Socket = std::make_unique<Socket::Socket>();
		m_Socket->Open(m_IP, xs("%", m_Port));

		size_t retries = 3;
		while (!m_Socket->IsOpen() && retries > 0)
		{
			m_Socket = std::make_unique<Socket::Socket>();
			m_Socket->Open(m_IP, xs("%", m_Port));
			retries--;
		}

		if (!m_Socket->IsOpen())
		{
			std::cout << xs("failed to connect to peer %:%", m_IP, m_Port) << std::endl;
			return;
		}

		std::cout << xs("connected to peer %:%", m_IP, m_Port) << std::endl;

		SendHandshake();
		ReceiveHandshake();

		for (;;)
		{
			if (m_Socket == nullptr)
			{
				return;
			}
			
			ReceiveMessage();
		}
	}

	void Peer::SendHandshake()
	{
		std::vector<char> handshake;
		handshake.push_back(19);

		std::string proto = "BitTorrent protocol";
		for (auto c : proto)
		{
			handshake.push_back(c);
		}

		for (auto i = 0u; i < 8; i++)
		{
			handshake.push_back(0);
		}

		for (auto c : m_Client->m_InfoHash)
		{
			handshake.push_back(c);
		}

		for (auto c : m_Client->m_PeerID)
		{
			handshake.push_back(c);
		}

		m_Socket->Send(handshake);
	}

	void Peer::ReceiveHandshake()
	{
		std::vector<char> msgLenBytes;
		auto recvd = m_Socket->Receive(msgLenBytes, 1);

		if (recvd <= 0)
		{
			m_Socket = nullptr;
			return;
		}

		char msgLen = msgLenBytes[0];
		std::vector<char> pstr;

		recvd = m_Socket->Receive(pstr, msgLen);
		if (recvd != msgLen)
		{
			std::cout << "bad handshake" << std::endl;
			m_Socket = nullptr;
			return;
		}

		std::string pstrString(pstr.data(), pstr.size());
		
		std::vector<char> reserved;
		recvd = m_Socket->Receive(reserved, 8);

		std::vector<char> infoHash;
		recvd = m_Socket->Receive(infoHash, 20);

		if (m_Client->m_InfoHash != std::string(infoHash.data(), infoHash.size()))
		{
			std::cout << "wrong info_hash, bad handshake" << std::endl;
			m_Socket = nullptr;
			return;
		}

		std::vector<char> peerId;
		recvd = m_Socket->Receive(peerId, 20);
		std::cout << "got peer id: " << std::string(peerId.data(), peerId.size()) << std::endl;
		std::cout << "handshake complete" << std::endl;
	}

	void Peer::ReceiveMessage()
	{
		std::vector<char> msgLengthBytes;
		auto recvd = m_Socket->Receive(msgLengthBytes, 4);

		if (recvd <= 0)
		{
			m_Socket = nullptr;
			return;
		}

		//std::reverse(msgLengthBytes.begin(), msgLengthBytes.end());

		int msgLength = *((int*)msgLengthBytes.data());
		//std::cout << "len " << msgLength << std::endl;

	}

}