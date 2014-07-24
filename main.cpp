#include <iostream>
#include <vector>
#include <unordered_map>
#include <deque>
#include <string>
#include <sstream>
#include <thread>
#include <mutex>
#include <memory>
#include <set>
#include <fstream>
#include <assert.h>

#include <boost/asio.hpp>
#include "ASIOThreadPool.h"

#include "Timer.h"
#include "BandwidthTracker.h"
#include "Filesystem.h"
#include "StringFacility.h"
#include "BinaryString.h"
#include "BencodeTokenizer.h"
#include "BencodeParser.h"
#include "MetadataFile.h"
#include "Socket.h"
#include "Wire.h"
#include "HTTP.h"
#include "Peer.h"
#include "Piece.h"
#include "File.h"
#include "Client.h"


using namespace TorrentStream;

int main()
{
	Timer::Initialize();

	std::ifstream f("test.torrent", std::ios::binary);
	if (!f.is_open())
	{
		throw std::exception();
	}

	std::vector<char> contents((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

	auto metadata = std::make_shared<MetadataFile>(contents);

	
	metadata->PrintInfo();

	Client client(metadata, "test\\");

	client.Start();
	client.Stop();
	
	return 0;
}