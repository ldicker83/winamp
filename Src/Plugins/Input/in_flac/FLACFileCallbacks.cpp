#include "FLACFileCallbacks.h"
#include <system_error>


bool FLACClientData::Open(const std::filesystem::path &filename)
{
	file.open(filename, std::ios::binary);
	if (!file.is_open())
		return false;

	std::error_code error;
	length = std::filesystem::file_size(filename, error);
	if (error)
	{
		file.close();
		return false;
	}
	return true;
}

bool FLACClientData::Read(void *buffer, size_t bytes_requested, size_t *bytes_read)
{
	file.read(static_cast<char *>(buffer), static_cast<std::streamsize>(bytes_requested));
	*bytes_read = static_cast<size_t>(file.gcount());

	if (file.bad())
		return false;

	// a short read sets eofbit and failbit, which would make the next tellg fail
	file.clear();
	return true;
}

bool FLACClientData::Seek(uint64_t position)
{
	file.clear();
	file.seekg(static_cast<std::streamoff>(position));
	return !file.fail();
}

bool FLACClientData::Tell(uint64_t *position)
{
	const std::streamoff offset = file.tellg();
	if (offset < 0)
		return false;

	*position = static_cast<uint64_t>(offset);
	return true;
}

bool FLACClientData::EndOfFile()
{
	uint64_t position;
	return !Tell(&position) || position >= length;
}

FLAC__StreamDecoderReadStatus FLAC_File_Read(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)
{
	size_t bytes_read = 0;
	const bool success = ((FLACClientData *)client_data)->Read(buffer, *bytes, &bytes_read);
	*bytes = bytes_read;

	if (!success)
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;

	if (bytes_read == 0)
		return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;

	return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

FLAC__StreamDecoderSeekStatus FLAC_File_Seek(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
	if (((FLACClientData *)client_data)->Seek(absolute_byte_offset))
		return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
	else
		return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
}

FLAC__StreamDecoderTellStatus FLAC_File_Tell(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
	if (((FLACClientData *)client_data)->Tell(absolute_byte_offset))
		return FLAC__STREAM_DECODER_TELL_STATUS_OK;
	else
		return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
}

FLAC__StreamDecoderLengthStatus FLAC_File_Length(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)
{
	*stream_length = ((FLACClientData *)client_data)->GetLength();
	return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

FLAC__bool FLAC_File_EOF(const FLAC__StreamDecoder *decoder, void *client_data)
{
	return ((FLACClientData *)client_data)->EndOfFile();
}
