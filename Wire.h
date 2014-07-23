#ifndef __TORRENTSTREAM_WIRE_H
#define __TORRENTSTREAM_WIRE_H

namespace TorrentStream
{

	namespace Wire
	{
		enum class PeerMessageID
		{
			Choke = 0,
			Unchoke = 1,
			Interested = 2,
			NotInterested = 3,
			Have = 4,
			Bitfield = 5,
			Request = 6,
			Piece = 7,
			Cancel = 8,
			Port = 9,
			KeepAlive = 10,
		};

		class SocketError : public std::exception {};

		class BadHandshake : public std::exception {};

		class BadMessageLength : public std::exception {};

		class BadMessageID : public std::exception {};

		bool SendHandshake(const std::unique_ptr<Socket::Socket>& socket, const std::string& infoHash, const std::string& peerId)
		{
			BinaryString handshake;

			handshake.PushUint8(19);
			handshake.PushString("BitTorrent protocol");
			handshake.PushInt32(0);
			handshake.PushInt32(0);
			handshake.PushString(infoHash);
			handshake.PushString(peerId);

			return socket->Send(handshake.Get());
		}

		void ReceiveHandshake(const std::unique_ptr<Socket::Socket>& socket, std::string& infoHash, std::string& peerId)
		{
			char msgLen;
			auto recvd = socket->Receive(msgLen);

			if (recvd <= 0)
			{
				throw SocketError();
			}

			std::string pstr;
			recvd = socket->Receive(pstr, msgLen);
			if (recvd != msgLen)
			{
				throw BadHandshake();
			}

			std::vector<char> reserved;
			recvd = socket->Receive(reserved, 8);
			if (recvd <= 0)
			{
				throw SocketError();
			}

			recvd = socket->Receive(infoHash, 20);
			if (recvd <= 0)
			{
				throw SocketError();
			}

			recvd = socket->Receive(peerId, 20);
			if (recvd <= 0)
			{
				throw SocketError();
			}
		}

		bool SendInterested(const std::unique_ptr<Socket::Socket>& socket)
		{
			BinaryString msg;
			msg.PushInt32(1, BigEndian);
			msg.PushUint8((char)PeerMessageID::Interested);
			return socket->Send(msg.Get());
		}

		PeerMessageID ReceiveMessageHeader(const std::unique_ptr<Socket::Socket>& socket, unsigned int& msgLength)
		{
			unsigned int len;
			auto recvd = socket->Receive(len);
			len = ByteSwap(len); // convert to little endian

			if (len > 262144)
			{
				throw BadMessageLength();
			}

			if (recvd <= 0)
			{
				throw SocketError();
			}

			if (len == 0)
			{
				msgLength = 0;
				return PeerMessageID::KeepAlive;
			}

			char id;
			recvd = socket->Receive(id);

			if (recvd <= 0)
			{
				throw SocketError();
			}

			if (id < 0 || id > 9)
			{
				throw BadMessageID();
			}

			msgLength = len;
			return (PeerMessageID)id;
		}

	}
	
}

#endif
