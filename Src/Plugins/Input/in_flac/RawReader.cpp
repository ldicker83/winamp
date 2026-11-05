#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "RawReader.h"
#include "main.h"
#include <limits.h>
#include <shlwapi.h>
#include "../nu/AutoWide.h"
#include <new>
#include <strsafe.h>
#include "foundation/error.h"

static bool IsMyExtension(const wchar_t *filename)
{
	const wchar_t *extension = PathFindExtensionW(filename);
	if (extension && *extension)
	{
		wchar_t exts[128] = {0};
		GetPrivateProfileStringW(L"in_flac", L"extensions", DEFAULT_EXTENSIONSW, exts, 128, winampINI);

		extension++;
		wchar_t *b = exts;
		wchar_t *c;
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
	return false;
}

int RawMediaReaderService::CreateRawMediaReader(const wchar_t *filename, ifc_raw_media_reader **out_reader)
{
	if (IsMyExtension(filename))
	{
		RawMediaReader *reader = new (std::nothrow) RawMediaReader();
		if (!reader)
		{
			return NErr_OutOfMemory;
		}

		int ret = reader->Initialize(filename);
		if (ret != NErr_Success)
		{
			delete reader;
			return ret;
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

static void OnError(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	//client_data=client_data; // dummy line so i can set a breakpoint
}

static void OnMetadata(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{

}

static FLAC__StreamDecoderWriteStatus OnAudio(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data)
{

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

RawMediaReader::RawMediaReader()
{
	decoder=0;
}

RawMediaReader::~RawMediaReader()
{
	if (decoder)
		FLAC__stream_decoder_delete(decoder);
}

int RawMediaReader::Initialize(const std::filesystem::path &filename)
{
	if (!state.Open(filename))
		return NErr_FileNotFound;

	decoder = FLAC__stream_decoder_new();
	if (!decoder)
		return NErr_FailedCreate;

	FLAC__stream_decoder_set_metadata_ignore(decoder, FLAC__METADATA_TYPE_VORBIS_COMMENT);
	state.SetObject(this);
	FLAC__stream_decoder_set_md5_checking(decoder, true);
	if(FLAC__stream_decoder_init_stream(
		decoder,
		FLAC_File_Read,
		FLAC_File_Seek,
		FLAC_File_Tell,
		FLAC_File_Length,
		FLAC_File_EOF,
		OnAudio,
		OnMetadata,
		OnError,
		&state 
		) != FLAC__STREAM_DECODER_INIT_STATUS_OK) 
	{
		FLAC__stream_decoder_delete(decoder);
		decoder=0;
		return NErr_Error;	
	}

	FLAC__stream_decoder_process_until_end_of_metadata(decoder);
	uint64_t position;
	FLAC__stream_decoder_get_decode_position(decoder, &position);
	FLAC__stream_decoder_delete(decoder);
	decoder=0;
	state.Seek(position);
	
	return NErr_Success;
}

int RawMediaReader::Read(void *buffer, size_t buffer_size, size_t *bytes_read)
{
	size_t read = 0;
	if (!state.Read(buffer, buffer_size, &read))
		return NErr_Error;

	if (bytes_read)
		*bytes_read = read;

	return read ? NErr_Success : NErr_EndOfFile;
}

size_t RawMediaReader::Release()
{

	delete this;
	return 0;
}

#define CBCLASS RawMediaReader
START_DISPATCH;
CB(RELEASE, Release);
CB(RAW_READ, Read);
END_DISPATCH;
#undef CBCLASS