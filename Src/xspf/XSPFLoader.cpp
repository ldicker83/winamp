#include "XSPFLoader.h"
#include "../xml/obj_xml.h"
#include "../xml/ifc_xmlreadercallback.h"
#include "api__xspf.h"
#include "../winamp/wa_ipc.h"
#include <api/service/waservicefactory.h>
#include <shlwapi.h>
#include <strsafe.h>

static bool HasTagzAPI()
{
	// TODO: should critical section this
	if (!AGAVE_API_TAGZ)
	{
		waServiceFactory *sf = WASABI_API_SVC->service_getServiceByGuid(tagzGUID);
		if (sf)
			AGAVE_API_TAGZ = (api_tagz *)sf->getInterface(); // retrieve a pointer to the API object
	}
	return !!AGAVE_API_TAGZ;
}

const wchar_t* ATFString()
{
	// TODO: should critical section this
	if (!WASABI_API_APP)
	{
		waServiceFactory *sf = WASABI_API_SVC->service_getServiceByGuid(applicationApiServiceGuid);
		if (sf)
			WASABI_API_APP = (api_application *)sf->getInterface(); // retrieve a pointer to the API object
	}
	if (WASABI_API_APP)
	{
		const wchar_t *atf = WASABI_API_APP->getATFString();
		if (atf && *atf) return atf;
	}
	return L"%artist% - %title%";
}

// go check out XSPFLoader::Load before decyphering this class
class XSPFLoaderCallback : public ifc_xmlreadercallback
{
public:
	XSPFLoaderCallback(ifc_playlistloadercallback *_playlist);
	~XSPFLoaderCallback();
	void xmlReaderOnStartElementCallback(const wchar_t *xmlpath, const wchar_t *xmltag, ifc_xmlreaderparams *params);
	void xmlReaderOnEndElementCallback(const wchar_t *xmlpath, const wchar_t *xmltag);
	void xmlReaderOnCharacterDataCallback(const wchar_t *xmlpath, const wchar_t *xmltag, const wchar_t *str);

protected:
	RECVS_DISPATCH;

private:
	enum
	{
		ELEMENT_NONE=-1,
		ELEMENT_LOCATION=0,
		ELEMENT_IDENTIFIER,
		ELEMENT_TITLE,
		ELEMENT_CREATOR,
		ELEMENT_ALBUM,
		ELEMENT_TRACKNUM,
		NUM_ELEMENTS,
	};
	wchar_t *elements[NUM_ELEMENTS];
	int element;
	ifc_playlistloadercallback *playlist;
};

XSPFLoaderCallback::XSPFLoaderCallback(ifc_playlistloadercallback *_playlist)
{
	element = ELEMENT_NONE;
	for (int i=0;i<NUM_ELEMENTS;i++)
		elements[i]=0;

	playlist = _playlist;
}

XSPFLoaderCallback::~XSPFLoaderCallback()
{
	for (int i=0;i<NUM_ELEMENTS;i++)
	{
		free(elements[i]);
		elements[i]=0;
	}
}

// this gets called for every opening tag
// xmlpath is the full XML path up to this point, delimited with \f (form feed)
// xmltag is the actual opening tag
// params is an object which lets you query for the attribtues in the tag
void XSPFLoaderCallback::xmlReaderOnStartElementCallback(const wchar_t *xmlpath, const wchar_t *xmltag, ifc_xmlreaderparams *params)
{
	if (!wcscmp(xmlpath, L"playlist\ftrackList\ftrack\flocation"))
		element=ELEMENT_LOCATION;
	else if (!wcscmp(xmlpath, L"playlist\ftrackList\ftrack\fidentifier"))
		element=ELEMENT_IDENTIFIER;
	else if (!wcscmp(xmlpath, L"playlist\ftrackList\ftrack\ftitle"))
		element=ELEMENT_TITLE;
	else if (!wcscmp(xmlpath, L"playlist\ftrackList\ftrack\fcreator"))
		element=ELEMENT_CREATOR;
	else if (!wcscmp(xmlpath, L"playlist\ftrackList\ftrack\falbum"))
		element=ELEMENT_ALBUM;
	else if (!wcscmp(xmlpath, L"playlist\ftrackList\ftrack\ftracknum"))
		element=ELEMENT_TRACKNUM;
	else
		element=ELEMENT_NONE;
}

// this gets called for every closing tag
void XSPFLoaderCallback::xmlReaderOnEndElementCallback(const wchar_t *xmlpath, const wchar_t *xmltag)
{
	if (!wcscmp(xmlpath, L"playlist\ftrackList\ftrack"))
	{
		// end of track info
		if (elements[ELEMENT_LOCATION] // if we have a location
		&& (PathIsURL(elements[ELEMENT_LOCATION]) // and it's a URL
			|| GetFileAttributes(elements[ELEMENT_LOCATION])  != INVALID_FILE_ATTRIBUTES)) // or a file that exists
		{
			// location field seemed OK so go ahead and add
			// TODO try using the length if available...
			playlist->OnFile(elements[ELEMENT_LOCATION], 0, -1, 0);
		}
		for (int i=0;i<NUM_ELEMENTS;i++)
		{
			free(elements[i]);
			elements[i]=0;
		}
	}
	else if (!wcscmp(xmlpath, L"playlist\ftrackList\ftrack\flocation"))
		element=ELEMENT_NONE;
	else if (!wcscmp(xmlpath, L"playlist\ftrackList\ftrack\fidentifier"))
		element=ELEMENT_NONE;
	else if (!wcscmp(xmlpath, L"playlist\ftrackList\ftrack\ftitle"))
		element=ELEMENT_NONE;
	else if (!wcscmp(xmlpath, L"playlist\ftrackList\ftrack\fcreator"))
		element=ELEMENT_NONE;
	else if (!wcscmp(xmlpath, L"playlist\ftrackList\ftrack\falbum"))
		element=ELEMENT_NONE;
	else if (!wcscmp(xmlpath, L"playlist\ftrackList\ftrack\ftracknum"))
		element=ELEMENT_NONE;
}

void XSPFLoaderCallback::xmlReaderOnCharacterDataCallback(const wchar_t *xmlpath, const wchar_t *xmltag, const wchar_t *str)
{
	if (element!=ELEMENT_NONE)
	{
		// this certainly isn't the most memory efficient way to do this but it'll work for now :)
		if (elements[element] == 0)
		{
			elements[element] = wcsdup(str);
		}
		else
		{
			size_t elementlen = wcslen(elements[element]);
			size_t incominglen = wcslen(str);
			
			wchar_t *&newstr = elements[element], *oldstr = newstr;
			newstr = (wchar_t *)realloc(newstr, sizeof(wchar_t)*(elementlen+incominglen+1));
			if (newstr != NULL)
			{
				memcpy(newstr+elementlen, str, incominglen*sizeof(wchar_t));
				newstr[elementlen+incominglen] = 0;
			}
		}
	}
}

#define CBCLASS XSPFLoaderCallback
START_DISPATCH;
VCB(ONSTARTELEMENT, xmlReaderOnStartElementCallback)
VCB(ONENDELEMENT, xmlReaderOnEndElementCallback)
VCB(ONCHARDATA, xmlReaderOnCharacterDataCallback)
END_DISPATCH;
#undef CBCLASS

/* ---------------------------- */
static int LoadFile(obj_xml *parser, const wchar_t *filename)
{
	HANDLE file = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);

	if (file == INVALID_HANDLE_VALUE)
		return IFC_PLAYLISTLOADER_FAILED;

	while (true)
	{
		char data[1024] = {0};
		DWORD bytesRead = 0;
		if (ReadFile(file, data, 1024, &bytesRead, NULL) && bytesRead)
		{
			if (parser->xmlreader_feed(data, bytesRead) != API_XML_SUCCESS)
			{
				CloseHandle(file);
				return IFC_PLAYLISTLOADER_FAILED;
			}
		}
		else
			break;
	}

	CloseHandle(file);
	if (parser->xmlreader_feed(0, 0) != API_XML_SUCCESS)
		return IFC_PLAYLISTLOADER_FAILED;

	return IFC_PLAYLISTLOADER_SUCCESS;
}

// this get called by Winamp
// you a method in the passed playlist loader callback object
// for each item in the playlist
int XSPFLoader::Load(const wchar_t *filename, ifc_playlistloadercallback *playlist)
{
	// first thing we'll need is an XML parser
	// we'll have to do the wasabi dance to get it

	// first, get the service factory for creating XML objects
	waServiceFactory *xmlFactory = WASABI_API_SVC->service_getServiceByGuid(obj_xmlGUID);
	if (xmlFactory)
	{
		obj_xml *parser = (obj_xml *)xmlFactory->getInterface(); // create an XML parser
		if (parser)
		{
			// ok now we can get down to biz'nes
			XSPFLoaderCallback xmlCallback(playlist);
			parser->xmlreader_registerCallback(L"*", &xmlCallback);
			parser->xmlreader_setCaseSensitive();
			parser->xmlreader_open();
			int ret = LoadFile(parser, filename);
			parser->xmlreader_unregisterCallback(&xmlCallback);
			parser->xmlreader_close();
			xmlFactory->releaseInterface(parser); // destroy the XML parser via the service factory

			return ret;
		}
	}

	return IFC_PLAYLISTLOADER_FAILED;
}

// Define the dispatch table
#define CBCLASS XSPFLoader
START_DISPATCH;
CB(IFC_PLAYLISTLOADER_LOAD, Load)
END_DISPATCH;
#undef CBCLASS