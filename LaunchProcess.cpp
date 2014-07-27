#include <Windows.h>
#include <string>

#include "LaunchProcess.h"

namespace TorrentStream
{

	void LaunchProcess(const std::string& file, const std::string& commandline)
	{
		STARTUPINFO info = { sizeof(info) };
		PROCESS_INFORMATION processInfo;

		auto cmd = "\"" + file + "\" " + commandline;
	
		CreateProcess(file.c_str(), (char*)cmd.c_str(), NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo);
	}

	std::string GetCurrentWorkingDirectory()
	{
		std::string result;
		result.resize(512);
		result.resize(GetCurrentDirectory(512, (LPSTR)result.c_str()));
		return result;
	}

}