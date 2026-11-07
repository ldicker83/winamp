#include "main.h"
#include "MP3Decoder.h"

// Decodes a file for Winamp's transcoding and other "extended read" users.

struct ExtendedRead
{
	MP3Decoder decoder;
};

static ExtendedRead *OpenExtendedRead(const wchar_t *fn, int *size, int *bps, int *nch, int *srate, bool useFloat)
{
	MP3Decoder::Settings settings;
	settings.useFloat = useFloat;
	settings.bits = (*bps == 24) ? 24 : 16;
	settings.mono = (*nch == 1);
	settings.gapless = !!config_gapless;
	settings.memoryLimitKB = config_max_bufsize_k;

	ExtendedRead *ext = new ExtendedRead;
	if (!ext->decoder.Open(fn, settings))
	{
		delete ext;
		return 0;
	}

	*nch = ext->decoder.Channels();
	*srate = ext->decoder.SampleRate();
	*bps = ext->decoder.Bits();

	const int64_t samples = ext->decoder.LengthSamples();
	*size = samples >= 0 ? (int)(samples * ext->decoder.FrameBytes()) : -1;
	return ext;
}

extern "C"
{
	//returns handle!=0 if successful, 0 if error
	//size will return the final nb of bytes written to the output, -1 if unknown
	__declspec(dllexport) intptr_t winampGetExtendedRead_openW(const wchar_t *fn, int *size, int *bps, int *nch, int *srate)
	{
		return reinterpret_cast<intptr_t>(OpenExtendedRead(fn, size, bps, nch, srate, false));
	}

	__declspec(dllexport) intptr_t winampGetExtendedRead_openW_float(const wchar_t *fn, int *size, int *bps, int *nch, int *srate)
	{
		return reinterpret_cast<intptr_t>(OpenExtendedRead(fn, size, bps, nch, srate, true));
	}

	//returns nb of bytes read. -1 if read error (like CD ejected). if (ret==0), EOF is assumed
	__declspec(dllexport) size_t winampGetExtendedRead_getData(intptr_t handle, char *dest, size_t len, int *killswitch)
	{
		ExtendedRead *ext = (ExtendedRead *)handle;
		if (!ext)
			return 0;

		len -= len % ext->decoder.FrameBytes(); // only whole sample frames
		size_t copied = 0;
		while (copied < len && !(killswitch && *killswitch))
		{
			size_t bytes = 0;
			const MP3Decoder::Result result = ext->decoder.Read(dest + copied, len - copied, &bytes);
			copied += bytes;
			if (result == MP3Decoder::EndOfFile)
				break;
			if (result == MP3Decoder::Failure)
				return copied ? copied : (size_t)-1;
		}
		return copied;
	}

	// return nonzero on success, zero on failure.
	__declspec(dllexport) int winampGetExtendedRead_setTime(intptr_t handle, int millisecs)
	{
		ExtendedRead *ext = (ExtendedRead *)handle;
		return ext && ext->decoder.Seek(millisecs);
	}

	__declspec(dllexport) void winampGetExtendedRead_close(intptr_t handle)
	{
		delete (ExtendedRead *)handle;
	}
}
