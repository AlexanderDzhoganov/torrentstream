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
#include <intrin.h>

#include "Filesystem.h"
#include "StringFacility.h"
#include "BinaryString.h"

#include "Socket.h"
#include "Wire.h"
#include "HTTP.h"
#include "BencodeTokenizer.h"
#include "BencodeParser.h"
#include "MetadataFile.h"
#include "Peer.h"
#include "Piece.h"
#include "File.h"
#include "Client.h"

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
		Wire::SendHandshake(m_Socket, m_Client->m_InfoHash, m_Client->m_PeerID);
	}

	void Peer::SendInterested()
	{
		Wire::SendInterested(m_Socket);
	}

	void Peer::ReceiveHandshake()
	{
		std::string infoHash;
		std::string peerId;

		try
		{
			Wire::ReceiveHandshake(m_Socket, infoHash, peerId);
		}
		catch (const std::exception& e)
		{
			std::cout << "Exception in ReceiveHandshake, closing connection.." << std::endl;
			m_Socket = nullptr;
			return;
		}

		if (m_Client->m_InfoHash != infoHash)
		{
			std::cout << "wrong info_hash, bad handshake" << std::endl;
			m_Socket = nullptr;
			return;
		}

		std::cout << "handshake complete, got peer id: " << peerId << std::endl;

		SendInterested();
	}

	void Peer::ReceiveMessage()
	{
		unsigned int len = 0;
		Wire::PeerMessageID id;
		
		try
		{
			id = Wire::ReceiveMessageHeader(m_Socket, len);
		}
		catch (const std::exception& e)
		{
			std::cout << "Exception in ReceiveMessageHeader, closing connection.." << std::endl;
			m_Socket = nullptr;
			return;
		}

		switch (id)
		{
		case Wire::PeerMessageID::Choke:
			OnChoke();
			break;
		case Wire::PeerMessageID::Unchoke:
			OnUnchoke();
			break;
		case Wire::PeerMessageID::Interested:
			OnInterested();
			break;
		case Wire::PeerMessageID::NotInterested:
			OnNotInterested();
			break;
		case Wire::PeerMessageID::Have:
			OnHave();
			break;
		case Wire::PeerMessageID::Bitfield:
			OnBitfield(len);
			break;
		case Wire::PeerMessageID::Request:
			OnRequest();
			break;
		case Wire::PeerMessageID::Piece:
			OnPiece(len);
			break;
		case Wire::PeerMessageID::Cancel:
			OnCancel();
			break;
		case Wire::PeerMessageID::Port:
			OnPort();
			break;
		default:
			break;
		}
	}

	void Peer::OnKeepAlive()
	{
	}

	void Peer::OnChoke()
	{
		m_Choked = true;
	}

	void Peer::OnUnchoke()
	{
		m_Choked = false;
	}

	void Peer::OnInterested()
	{
		m_Interested = true;
	}

	void Peer::OnNotInterested()
	{
		m_Interested = false;
	}

	void Peer::OnHave()
	{
		unsigned int piece;
		m_Socket->Receive(piece);
		piece = ByteSwap(piece);
		std::cout << "have: " << piece << std::endl;
	}

	void Peer::OnBitfield(size_t len)
	{
		std::vector<char> bitfield;
		m_Socket->Receive(bitfield, len - 1);

		std::cout << "bitfield: " << len << std::endl;
	}

	void Peer::OnRequest()
	{
		unsigned int index;
		m_Socket->Receive(index);
		index = ByteSwap(index);

		unsigned int begin;
		m_Socket->Receive(begin);
		begin = ByteSwap(begin);

		unsigned int len;
		m_Socket->Receive(len);
		len = ByteSwap(len);

		std::cout << "request" << std::endl;
	}

	void Peer::OnPiece(size_t len)
	{
		std::vector<char> payload;
		m_Socket->Receive(payload, len - 1);
		std::cout << "piece" << std::endl;
	}

	void Peer::OnCancel()
	{
		std::vector<char> payload;
		m_Socket->Receive(payload, 12);
		std::cout << "cancel" << std::endl;
	}

	void Peer::OnPort()
	{
		std::vector<char> payload;
		m_Socket->Receive(payload, 2);
		std::cout << "port" << std::endl;
	}

}