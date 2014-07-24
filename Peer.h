#ifndef __TORRENTSTREAM_PEER_H
#define __TORRENTSTREAM_PEER_H

namespace TorrentStream
{

	struct PeerMessage
	{
		int length;
		char id;
		std::vector<char> payload;
	};

	enum class PeerMessageType
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

	enum class PeerState
	{
		Disconnected = 0,
		Choked,
		Waiting,
		Idle,
		Downloading,
		Error
	};

	class Client;

	struct PieceRequest
	{
		size_t index;
		size_t offset;
	};

	class Peer
	{

		public:
		Peer(const std::string& ip, int port, const std::string& id, Client* client);

		~Peer()
		{
		}

		const std::string& GetID() const
		{
			return m_ID;
		}

		const BandwidthTracker& GetDownloadBandwidthTracker() const
		{
			return m_BandwidthDown;
		}

		PeerState GetState() const
		{
			return m_State;
		}

		void Connect();

		void WarmUp();

		size_t GetAverageBandwidth()
		{
			return m_AverageBandwidth;
		}

		private:
		void RequestPiece(size_t index);

		void HandleSendInterested();
		void HandleSendNotInterested();

		void HandleSendRequest(size_t index, size_t offset, size_t length);

		void HandleResolve(const boost::system::error_code& error, boost::asio::ip::tcp::resolver::iterator endpoint);
		void HandleConnect(const boost::system::error_code& error);
		void HandleHandshake(std::shared_ptr<std::array<char, 68>> buffer, const boost::system::error_code& error);

		void QueueReceiveMessage();
		void HandleReceiveMessageHeader(std::shared_ptr<std::array<char, 4>> buffer, const boost::system::error_code& error);
		void HandleReceiveMessageID(std::shared_ptr<char> id, size_t len, const boost::system::error_code& error);

		void HandleReceiveHaveHeader();
		void HandleReceiveHave(std::shared_ptr<int> index, const boost::system::error_code& error);

		void HandleReceiveBitfieldHeader(size_t len);
		void HandleReceiveBitfield(std::shared_ptr<std::vector<char>> bitfield, const boost::system::error_code& error);
		
		void HandleReceiveRequestHeader();
		void HandleReceiveRequest(const boost::system::error_code& error);

		void HandleReceivePieceHeader(size_t len);
		void HandleReceivePiece(std::shared_ptr<std::vector<char>> data, const boost::system::error_code& error);

		void HandleReceiveCancelHeader();
		void HandleReceiveCancel(const boost::system::error_code& error);

		void HandleReceivePortHeader();
		void HandleReceivePort(const boost::system::error_code& error);

		bool m_WarmUp = false;
		size_t m_AverageBandwidth = 0;

		size_t m_RequestSize = 16 * 1024;
		size_t m_CurrentPiece = 0;
		size_t m_CurrentPieceOffset = 0;
		std::deque<PieceRequest> m_PieceRequests;

		Client* m_Client;

		std::string m_IP;
		int m_Port;

		std::string m_ID;
		std::string m_SelfReportedID;

		bool m_Interested = false;

		size_t m_PieceLength;
		std::vector<bool> m_PieceAvailability;

		std::unique_ptr<boost::asio::ip::tcp::socket> m_Socket = nullptr;
		std::unique_ptr<boost::asio::ip::tcp::resolver> m_Resolver = nullptr;

		BandwidthTracker m_BandwidthDown;

		PeerState m_State = PeerState::Disconnected;

	};

}

#endif
