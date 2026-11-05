#include "SimpleLoader.h"
#include <filesystem>
#include <fstream>
#include <string>
int SimpleLoader::Load(const wchar_t *filename, ifc_playlistloadercallback *playlist)
{
	std::wifstream simpleFile{std::filesystem::path(filename)};
	if (simpleFile)
	{
		std::wstring nextFile;
		while (std::getline(simpleFile, nextFile))
			playlist->OnFile(nextFile.c_str(), 0, -1, 0);
		return IFC_PLAYLISTLOADER_SUCCESS;
	}

	return IFC_PLAYLISTLOADER_FAILED;
	
}

// Define the dispatch table
#define CBCLASS SimpleLoader
START_DISPATCH;
CB(IFC_PLAYLISTLOADER_LOAD, Load)
END_DISPATCH;
#undef CBCLASS