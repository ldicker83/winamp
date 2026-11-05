#pragma once
#include <FLAC/all.h>
#include <cstdint>
#include <filesystem>
#include <fstream>


FLAC__StreamDecoderReadStatus FLAC_File_Read(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data);
FLAC__StreamDecoderSeekStatus FLAC_File_Seek(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data);
FLAC__StreamDecoderTellStatus FLAC_File_Tell(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data);
FLAC__StreamDecoderLengthStatus FLAC_File_Length(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data);
FLAC__bool FLAC_File_EOF(const FLAC__StreamDecoder *decoder, void *client_data);


class FLACClientData
{
public:
	bool Open(const std::filesystem::path &filename);
	// false only on a read error. reaching the end of the file gives true with fewer bytes
	bool Read(void *buffer, size_t bytes_requested, size_t *bytes_read);
	bool Seek(uint64_t position);
	bool Tell(uint64_t *position);
	bool EndOfFile();
	uint64_t GetLength() const { return length; }
	void SetObject(void *object) { this->object = object; }
	void *GetObject() { return object; }
private:
	std::ifstream file;
	uint64_t length = 0;
	void *object = nullptr;
};

template <typename _t>
static _t *FLAC_GetObject(void *client_data)
{
	return (_t *)((FLACClientData *)client_data)->GetObject();
}
