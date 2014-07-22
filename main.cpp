#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <sstream>
#include <thread>
#include <mutex>
#include <memory>
#include <fstream>
#include <assert.h>

#include "StringFacility.h"
#include "BencodeTokenizer.h"
#include "BencodeParser.h"
#include "MetadataFile.h"
#include "Socket.h"
#include "HTTP.h"
#include "Peer.h"
#include "Piece.h"
#include "File.h"
#include "Tracker.h"

using namespace TorrentStream;

int main()
{
	std::ifstream f("../test.torrent", std::ios::binary);
	if (!f.is_open())
	{
		throw std::exception();
	}

	std::vector<char> contents((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

	auto metadata = std::make_shared<MetadataFile>(contents);

	metadata->PrintInfo();

	Client client(metadata);

	client.Start();
	client.Stop();

	return 0;
}