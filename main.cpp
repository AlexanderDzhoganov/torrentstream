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

#include "Log.h"

#include <boost/asio.hpp>
#include "ASIOThreadPool.h"
#include "ASIOPeerMessage.h"
#include "ASIOPeerComm.h"

#include "Timer.h"
#include "BandwidthTracker.h"
#include "Filesystem.h"
#include "StringFacility.h"
#include "BinaryString.h"
#include "BencodeTokenizer.h"
#include "BencodeParser.h"
#include "MetadataFile.h"

#include "HTTP.h"
#include "HTTPServe.h"
#include "Piece.h"
#include "Peer.h"
#include "File.h"
#include "Client.h"

using namespace TorrentStream;

int main(int argc, char** argv)
{
	if (argc == 1)
	{
		return 1;
	}

	Timer::Initialize();

	std::ifstream f(argv[1], std::ios::binary);
	if (!f.is_open())
	{
		throw std::exception();
	}

	std::vector<char> contents((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

	auto metadata = std::make_shared<MetadataFile>(contents);

	for (auto i = 0u; i < metadata->GetFilesCount(); i++)
	{
		std::cout << "[" << i << "] " << metadata->GetFileName(i) << std::endl;
	}
	
	std::cout << "Select file to play" << std::endl;

	int selected = 0;
	std::cin >> selected;

	Client client(metadata, "");

	client.Start(selected);
	client.Stop();
	
	return 0;
}
