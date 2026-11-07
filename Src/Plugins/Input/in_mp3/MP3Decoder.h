#pragma once

#include <windows.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mpg123.h>

// MPEG audio (layer 1, 2 and 3) decoding with libmpg123.
//
// The file is read through libmpg123's reader callbacks, limited to the audio
// between the leading ID3v2 tag and any trailing APEv2, Lyrics3 or ID3v1 tag.
// libmpg123 reads the Xing/LAME header itself, so it handles gapless trimming,
// track length and sample accurate seeking.
class MP3Decoder
{
public:
	enum Result
	{
		Success,
		EndOfFile,
		Failure,
	};

	struct Settings
	{
		int bits = 16;              // 16 or 24; ignored when useFloat is set
		bool useFloat = false;      // 32-bit float output
		bool mono = false;          // mix down to one channel
		int downsample = 0;         // 0 = full rate, 1 = half rate, 2 = quarter rate
		bool gapless = true;        // trim encoder delay and padding
		bool replayGain = false;    // apply the ReplayGain settings from Winamp's playback preferences
		int memoryLimitKB = 0;      // read the whole file into memory if it isn't bigger than this
	};

	MP3Decoder();
	~MP3Decoder();
	MP3Decoder(const MP3Decoder &) = delete;
	MP3Decoder &operator=(const MP3Decoder &) = delete;

	bool Open(const wchar_t *filename, const Settings &settings);
	void Close();

	// Fills buffer with whole sample frames. bytesRead can be less than bytes
	// before the end of the file.
	Result Read(void *buffer, size_t bytes, size_t *bytesRead);
	bool Seek(int milliseconds);

	int SampleRate() const { return sampleRate; }
	int Channels() const { return channels; }
	int Bits() const { return bits; }
	int FrameBytes() const { return channels * bits / 8; }

	int64_t LengthSamples();    // -1 if unknown
	int LengthMilliseconds();   // -1 if unknown
	int64_t PositionSamples();

	int CurrentBitrate();       // kbps of the frame decoded last
	int AverageBitrate();       // kbps over the whole stream, 0 if unknown
	bool IsVBR();
	int Layer();

	struct Source; // the part of the file libmpg123 reads

private:
	mpg123_handle *handle = nullptr;
	std::unique_ptr<Source> source;
	int sampleRate = 0;
	int channels = 0;
	int bits = 0;
	int firstBitrate = 0;
	bool bitrateChanged = false;
};
