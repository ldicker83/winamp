#include "main.h"
#include "B4SLoader.h"
#include "../nu/AutoChar.h"
#include <strsafe.h>

B4SLoader::B4SLoader() : parser(0), parserFactory(0)
{
	parserFactory = WASABI_API_SVC->service_getServiceByGuid(obj_xmlGUID);
	if (parserFactory)
		parser = (obj_xml *)parserFactory->getInterface();

	if (parser)
	{
		parser->xmlreader_registerCallback(L"WasabiXML\fplaylist\fentry\fname", &b4sXml.title);
		parser->xmlreader_registerCallback(L"WinampXML\fplaylist\fentry\fname", &b4sXml.title);

		parser->xmlreader_registerCallback(L"WasabiXML\fplaylist\fentry\flength", &b4sXml.length);
		parser->xmlreader_registerCallback(L"WinampXML\fplaylist\fentry\flength", &b4sXml.length);

		//parser->xmlreader_registerCallback(L"WasabiXML\fplaylist", this);
		//parser->xmlreader_registerCallback(L"WinampXML\fplaylist", this);
		parser->xmlreader_registerCallback(L"WasabiXML\fplaylist\fentry", &b4sXml);
		parser->xmlreader_registerCallback(L"WinampXML\fplaylist\fentry", &b4sXml);
		parser->xmlreader_open();
		//parser->xmlreader_setEncoding(L"UTF-8"); 
	}
}

B4SXML::B4SXML()
{
	filename[0]=0;
	playlist = 0;
}

B4SLoader::~B4SLoader()
{
	if (parser)
	{
		parser->xmlreader_unregisterCallback(&b4sXml);
		parser->xmlreader_unregisterCallback(&b4sXml.length);
		parser->xmlreader_unregisterCallback(&b4sXml.title);
		parser->xmlreader_close();
	}

	if (parserFactory && parser)
		parserFactory->releaseInterface(parser);

	parserFactory = 0;
	parser = 0;
}

void B4SXML::StartTag(const wchar_t *xmlpath, const wchar_t *xmltag, ifc_xmlreaderparams *params)
{
	if (!_wcsicmp(xmltag, L"entry")) 
	{
		const wchar_t *track = params->getItemValue(L"playstring");
		if (!_wcsnicmp(track, L"file://", 7))
			track+=7;
		else if (!_wcsnicmp(track, L"file:", 5))
			track+=5;
		StringCchCopyW(filename, FILENAME_SIZE, track);
	}
}

void B4SXML::EndTag(const wchar_t *xmlpath, const wchar_t *xmltag)
{
	if (!_wcsicmp(xmltag, L"entry") && filename[0]) 
	{
		int lengthSeconds=-1; 
		const wchar_t *trackLength = length.GetString();
		if (trackLength && *trackLength)
			lengthSeconds = _wtoi(trackLength) / 1000;

		const wchar_t *trackTitle = title.GetString();
		// TODO: deal with relative pathnames
		if (trackTitle && *trackTitle)
			playlist->OnFile(filename, trackTitle, lengthSeconds, 0); // TODO: extended info
		else
			playlist->OnFile(filename, 0, lengthSeconds, 0);// TODO: extended info

		filename[0]=0;
		length.Reset();
		title.Reset();
	}
}
#if 0 // TOOD: reimplement
void B4SLoader::LoadFile(const char *filename)
{
	if (!parser)
		return ; // no sense in continuing if there's no parser available

	HANDLE file = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);

	if (file == INVALID_HANDLE_VALUE)
		return ;

	char data[1024] = {0};

	while (true)
	{
		DWORD bytesRead = 0;
		if (ReadFile(file, data, 1024, &bytesRead, NULL) && bytesRead)
		{
			parser->xmlreader_feed(data, bytesRead);
		}
		else
			break;
	}

	CloseHandle(file);
	parser->xmlreader_feed(0, 0);
}
#endif
int B4SLoader::Load(const wchar_t *filename, ifc_playlistloadercallback *playlist)
{
	if (!parser)
		return IFC_PLAYLISTLOADER_FAILED; // no sense in continuing if there's no parser available

	b4sXml.playlist=playlist;

	HANDLE file = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);

	if (file == INVALID_HANDLE_VALUE)
		return IFC_PLAYLISTLOADER_FAILED;

	while (true)
	{
		char data[1024] = {0};
		DWORD bytesRead = 0;
		if (ReadFile(file, data, 1024, &bytesRead, NULL) && bytesRead)
		{
			parser->xmlreader_feed(data, bytesRead);
		}
		else
			break;
	}

	CloseHandle(file);
	parser->xmlreader_feed(0, 0);
	return IFC_PLAYLISTLOADER_SUCCESS;
}

#ifdef CBCLASS
#undef CBCLASS
#endif

#define CBCLASS B4SXML
START_DISPATCH;
VCB(ONSTARTELEMENT, StartTag)
VCB(ONENDELEMENT, EndTag)
END_DISPATCH;

#ifdef CBCLASS
#undef CBCLASS
#endif

#define CBCLASS B4SLoader
START_DISPATCH;
CB(IFC_PLAYLISTLOADER_LOAD, Load)

END_DISPATCH;