#include "MP3Decoder.h"
#include "giofile.h"
#include <foundation/error.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

// The part of the file libmpg123 sees. Offsets passed to the callbacks are
// relative to the start of the audio, so the tags outside it don't exist for
// libmpg123.
struct MP3Decoder::Source
{
	~Source()
	{
		if (file != INVALID_HANDLE_VALUE)
			CloseHandle(file);
	}

	HANDLE file = INVALID_HANDLE_VALUE;
	std::vector<unsigned char> memory; // the whole range, when it is small enough
	uint64_t start = 0;
	uint64_t size = 0;
	uint64_t position = 0;
};

static int SourceRead(void *handle, void *buffer, size_t count, size_t *bytesRead)
{
	auto *source = static_cast<MP3Decoder::Source *>(handle);
	*bytesRead = 0;

	const uint64_t remaining = source->size - (std::min)(source->position, source->size);
	if (count > remaining)
		count = static_cast<size_t>(remaining);
	if (!count)
		return 0;

	if (!source->memory.empty())
	{
		memcpy(buffer, source->memory.data() + source->position, count);
	}
	else
	{
		LARGE_INTEGER offset;
		offset.QuadPart = static_cast<LONGLONG>(source->start + source->position);
		if (!SetFilePointerEx(source->file, offset, nullptr, FILE_BEGIN))
			return -1;

		size_t total = 0;
		while (total < count)
		{
			const DWORD chunk = static_cast<DWORD>((std::min<size_t>)(count - total, 1 << 20));
			DWORD read = 0;
			if (!ReadFile(source->file, static_cast<unsigned char *>(buffer) + total, chunk, &read, nullptr))
				return -1;
			if (!read)
				break;
			total += read;
		}
		count = total;
	}

	source->position += count;
	*bytesRead = count;
	return 0;
}

static int64_t SourceSeek(void *handle, int64_t offset, int whence)
{
	auto *source = static_cast<MP3Decoder::Source *>(handle);
	int64_t base;
	switch (whence)
	{
	case SEEK_SET: base = 0; break;
	case SEEK_CUR: base = static_cast<int64_t>(source->position); break;
	case SEEK_END: base = static_cast<int64_t>(source->size); break;
	default: return -1;
	}

	const int64_t position = base + offset;
	if (position < 0)
		return -1;
	source->position = static_cast<uint64_t>(position);
	return position;
}

MP3Decoder::MP3Decoder() = default;

MP3Decoder::~MP3Decoder()
{
	Close();
}

void MP3Decoder::Close()
{
	if (handle)
	{
		mpg123_close(handle);
		mpg123_delete(handle);
		handle = nullptr;
	}
	source.reset();
	sampleRate = channels = bits = 0;
	firstBitrate = 0;
	bitrateChanged = false;
}

bool MP3Decoder::Open(const wchar_t *filename, const Settings &settings)
{
	Close();

	if (!_wcsnicmp(filename, L"file://", 7))
		filename += 7;

	// find where the audio is, and the ReplayGain values in the tags
	float gain = 1.0f;
	auto range = std::make_unique<Source>();
	{
		CGioFile tags;
		if (tags.Open(filename, 0) != NErr_Success)
			return false;
		range->start = tags.AudioStart();
		range->size = tags.AudioEnd() > range->start ? tags.AudioEnd() - range->start : 0;
		if (settings.replayGain)
			gain = tags.GetGain();
		tags.Close();
	}

	range->file = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
	if (range->file == INVALID_HANDLE_VALUE)
		return false;

	if (settings.memoryLimitKB > 0 && range->size <= static_cast<uint64_t>(settings.memoryLimitKB) * 1024)
	{
		// read from the file while memory is still empty, then switch to the copy
		std::vector<unsigned char> copy(static_cast<size_t>(range->size));
		size_t read = 0;
		if (!copy.empty() && (SourceRead(range.get(), copy.data(), copy.size(), &read) || read != copy.size()))
			return false;
		range->memory.swap(copy);
		range->position = 0;
		CloseHandle(range->file);
		range->file = INVALID_HANDLE_VALUE;
	}

	int error = MPG123_OK;
	handle = mpg123_new(nullptr, &error);
	if (!handle)
		return false;

	long flags = MPG123_QUIET;
	if (settings.gapless)
		flags |= MPG123_GAPLESS;
	if (settings.mono)
		flags |= MPG123_MONO_MIX;
	mpg123_param2(handle, MPG123_FLAGS, flags, 0.0);
	mpg123_param2(handle, MPG123_RVA, MPG123_RVA_OFF, 0.0);
	if (settings.downsample > 0)
		mpg123_param2(handle, MPG123_DOWN_SAMPLE, (std::min)(settings.downsample, 2), 0.0);

	int encoding = MPG123_ENC_SIGNED_16;
	if (settings.useFloat)
		encoding = MPG123_ENC_FLOAT_32;
	else if (settings.bits == 24 && mpg123_format_support(handle, 44100, MPG123_ENC_SIGNED_24))
		encoding = MPG123_ENC_SIGNED_24;

	mpg123_format_none(handle);
	if (mpg123_format2(handle, 0, settings.mono ? MPG123_MONO : MPG123_MONO | MPG123_STEREO, encoding) != MPG123_OK)
	{
		Close();
		return false;
	}

	source = std::move(range);
	if (mpg123_reader64(handle, SourceRead, SourceSeek, nullptr) != MPG123_OK
	    || mpg123_open_handle64(handle, source.get()) != MPG123_OK)
	{
		Close();
		return false;
	}

	long rate = 0;
	int outputChannels = 0, outputEncoding = 0;
	if (mpg123_getformat2(handle, &rate, &outputChannels, &outputEncoding, 1) != MPG123_OK || rate <= 0 || outputChannels <= 0)
	{
		Close();
		return false;
	}

	sampleRate = static_cast<int>(rate);
	channels = outputChannels;
	bits = mpg123_encsize(outputEncoding) * 8;

	if (gain != 1.0f)
		mpg123_volume(handle, gain);

	firstBitrate = CurrentBitrate();
	return true;
}

MP3Decoder::Result MP3Decoder::Read(void *buffer, size_t bytes, size_t *bytesRead)
{
	*bytesRead = 0;
	if (!handle)
		return Failure;

	const size_t frameBytes = static_cast<size_t>(FrameBytes());
	bytes -= bytes % frameBytes;

	unsigned char *out = static_cast<unsigned char *>(buffer);
	while (*bytesRead < bytes)
	{
		size_t done = 0;
		const int result = mpg123_read(handle, out + *bytesRead, bytes - *bytesRead, &done);
		*bytesRead += done;

		switch (result)
		{
		case MPG123_OK:
			break;
		case MPG123_NEW_FORMAT:
			// the output format is fixed when the file is opened, so a stream that
			// changes format part way through keeps going in the first format
			break;
		case MPG123_DONE:
			return *bytesRead ? Success : EndOfFile;
		default:
			return *bytesRead ? Success : Failure;
		}

		if (!done && result == MPG123_OK)
			break;
	}

	const int bitrate = CurrentBitrate();
	if (bitrate && firstBitrate && bitrate != firstBitrate)
		bitrateChanged = true;
	return Success;
}

bool MP3Decoder::Seek(int milliseconds)
{
	if (!handle || !sampleRate)
		return false;
	const int64_t sample = static_cast<int64_t>((std::max)(milliseconds, 0)) * sampleRate / 1000;
	return mpg123_seek64(handle, sample, SEEK_SET) >= 0;
}

int64_t MP3Decoder::LengthSamples()
{
	if (!handle)
		return -1;
	const int64_t length = mpg123_length64(handle);
	return length >= 0 ? length : -1;
}

int MP3Decoder::LengthMilliseconds()
{
	const int64_t length = LengthSamples();
	if (length < 0 || !sampleRate)
		return -1;
	return static_cast<int>(length * 1000 / sampleRate);
}

int64_t MP3Decoder::PositionSamples()
{
	if (!handle)
		return 0;
	const int64_t position = mpg123_tell64(handle);
	return position >= 0 ? position : 0;
}

int MP3Decoder::CurrentBitrate()
{
	mpg123_frameinfo2 info;
	if (handle && mpg123_info2(handle, &info) == MPG123_OK)
		return info.bitrate;
	return 0;
}

int MP3Decoder::AverageBitrate()
{
	const int length = LengthMilliseconds();
	if (!source || length <= 0)
		return 0;
	// bits per millisecond is kilobits per second
	return static_cast<int>(source->size * 8 / static_cast<uint64_t>(length));
}

bool MP3Decoder::IsVBR()
{
	mpg123_frameinfo2 info;
	if (handle && mpg123_info2(handle, &info) == MPG123_OK && info.vbr != MPG123_CBR)
		return true;
	return bitrateChanged;
}

int MP3Decoder::Layer()
{
	mpg123_frameinfo2 info;
	if (handle && mpg123_info2(handle, &info) == MPG123_OK)
		return info.layer;
	return 0;
}
