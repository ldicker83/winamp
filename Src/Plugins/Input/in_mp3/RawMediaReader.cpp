#include "RawMediaReader.h"
#include "main.h"
#include "../nu/AutoWide.h"
#include "../nu/FilePath.h"
#include <limits.h>
#include <strsafe.h>

static bool IsMyExtension(const wchar_t *filename)
{
	if (FilePath::IsUrl(filename))
		return false;
	{
		const wchar_t *extension = FilePath::FindExtension(filename);
		if (extension && *extension)
		{
			AutoWide wideList(config_extlist); // TODO: build a copy of this at config load time so we don't have to run this every time
			extension++;
			wchar_t *b = wideList;

			wchar_t *c = 0;
			do
			{
				wchar_t d[20] = {0};
				StringCchCopyW(d, 15, b);
				if ((c = wcschr(b, L';')))
				{
					if ((c-b)<15)
						d[c - b] = 0;
				}

				if (!lstrcmpiW(extension, d))
					return true;

				b = c + 1;
			}
			while (c);
		}
	}
	return false;
}

int RawMediaReaderService::CreateRawMediaReader(const wchar_t *filename, ifc_raw_media_reader **out_reader)
{
	if (IsMyExtension(filename))
	{
		CGioFile *file = new CGioFile();
		if (!file)
			return NErr_OutOfMemory;

		if (file->Open(filename, 0) != NErr_Success)
		{
			delete file;
			return NErr_FileNotFound;
		}

		RawMediaReader *reader = new RawMediaReader(file);
		if (!reader)
		{
			file->Close();
			delete file;
			return NErr_OutOfMemory;
		}

		*out_reader = reader;
		return NErr_Success;
	}
	else
	{
		return NErr_False;
	}
}

#define CBCLASS RawMediaReaderService
START_DISPATCH;
CB(CREATERAWMEDIAREADER, CreateRawMediaReader);
END_DISPATCH;
#undef CBCLASS

RawMediaReader::RawMediaReader(CGioFile *file) : file(file)
{}

int RawMediaReader::Read( void *buffer, size_t buffer_size, size_t *bytes_read )
{
	if ( buffer_size > INT_MAX )
		return NErr_BadParameter;

	int file_bytes_read = 0;
	int ret = file->Read( buffer, (int)buffer_size, &file_bytes_read );

	if ( ret == NErr_Success )
	{
		*bytes_read = (size_t)file_bytes_read;
		if ( !file_bytes_read && file->IsEof() )
			return NErr_EndOfFile;

		return NErr_Success;
	}
	else
		return NErr_Error;
}

size_t RawMediaReader::Release()
{
	file->Close();

	delete file;
	file = NULL;

	delete this;

	return 0;
}


#define CBCLASS RawMediaReader
START_DISPATCH;
CB( RELEASE,  Release );
CB( RAW_READ, Read );
END_DISPATCH;
#undef CBCLASS
