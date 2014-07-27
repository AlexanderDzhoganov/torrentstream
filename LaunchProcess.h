#ifndef __TORRENTSTREAM_LAUNCHPROCESS_H
#define __TORRENTSTREAM_LAUNCHPROCESS_H

namespace TorrentStream
{

	void LaunchProcess(const std::string& file, const std::string& commandline);

	std::string GetCurrentWorkingDirectory();

}

#endif
