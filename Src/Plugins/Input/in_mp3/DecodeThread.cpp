#include "DecodeThread.h"
#include "main.h"
#include "config.h"
#include "api__in_mp3.h"
#include "MP3Decoder.h"
#include "../Winamp/wa_ipc.h"

// post this to the main window at end of file (after playback as stopped)
#define WM_WA_MPEG_EOF WM_USER+2

int decode_pos_ms; // decoding position, in milliseconds
volatile int seek_needed; // if != -1, the position in milliseconds the decode thread should seek to
int g_sndopened;
int g_length = -1000;
volatile int g_closeaudio = 0;

namespace
{
	// samples written to the output plug-in at a time (one layer 3 frame)
	constexpr int kChunkSamples = 1152;
	// samples per block given to the visualization
	constexpr int kVisSamples = 576;

	class DecodeLoop
	{
	public:
		DWORD Run();

	private:
		bool OpenOutput();
		void Seek(int milliseconds);
		void SendToVis(const unsigned char *samples, int sampleCount, int timestamp);
		void UpdateBitrate();
		int DisplayBitrate();

		MP3Decoder decoder;
		int frameBytes = 0;
		int shownBitrate = 0;

		// room for DSP plug-ins, which can return up to twice the samples they were given
		unsigned char buffer[kChunkSamples * 2 * 4 * 2];
	};
}

bool DecodeLoop::OpenOutput()
{
	// maxLatency is the time between an outMod->Write() call and when those samples are heard
	const int maxLatency = mod.outMod->Open(decoder.SampleRate(), decoder.Channels(), decoder.Bits(), -1, -1);
	if (maxLatency < 0) // error opening the device
	{
		PostMessage(mod.hMainWindow, WM_COMMAND, 40047, 0);
		return false;
	}
	g_sndopened = 1;
	if (paused)
		mod.outMod->Pause(1);

	// -666 means the current volume
	mod.outMod->SetVolume(-666);

	mod.SAVSAInit(maxLatency, decoder.SampleRate());
	mod.VSASetInfo(decoder.SampleRate(), decoder.Channels());
	return true;
}

void DecodeLoop::Seek(int milliseconds)
{
	decoder.Seek(milliseconds);
	decode_pos_ms = (int)(decoder.PositionSamples() * 1000 / decoder.SampleRate());
	mod.outMod->Flush(decode_pos_ms);
	seek_needed = -1;
}

void DecodeLoop::SendToVis(const unsigned char *samples, int sampleCount, int timestamp)
{
	const int bits = decoder.Bits();
	const int channels = decoder.Channels();
	for (int offset = 0; offset < sampleCount; offset += kVisSamples)
	{
		const char *block = (const char *)samples + offset * frameBytes;
		const int blockTime = timestamp + MulDiv(offset, 1000, decoder.SampleRate());
		int specChannels = 0, waveChannels = 0;
		if (mod.SAGetMode())
			mod.SAAddPCMData((char *)block, channels, bits, blockTime);
		if (mod.VSAGetMode(&specChannels, &waveChannels))
			mod.VSAAddPCMData((char *)block, channels, bits, blockTime);
	}
}

int DecodeLoop::DisplayBitrate()
{
	// "Show average bitrate on VBR files"
	if ((config_miscopts & 2) && decoder.IsVBR())
	{
		if (const int average = decoder.AverageBitrate())
			return average;
	}
	return decoder.CurrentBitrate();
}

void DecodeLoop::UpdateBitrate()
{
	const int bitrate = DisplayBitrate();
	if (bitrate && bitrate != shownBitrate)
	{
		shownBitrate = bitrate;
		mod.SetInfo(bitrate, -1, -1, 1);
	}
}

DWORD DecodeLoop::Run()
{
	MP3Decoder::Settings settings;
	settings.bits = (int)AGAVE_API_CONFIG->GetUnsigned(playbackConfigGroupGUID, L"bits", 16);
	settings.mono = AGAVE_API_CONFIG->GetBool(playbackConfigGroupGUID, L"mono", false);
	settings.downsample = config_downsample;
	settings.gapless = !!config_gapless;
	settings.replayGain = true;
	settings.memoryLimitKB = config_max_bufsize_k;

	if (!decoder.Open(lastfn, settings))
	{
		if (!killDecodeThread) Sleep(200);
		if (!killDecodeThread) PostMessage(mod.hMainWindow, WM_WA_MPEG_EOF, 0, 0);
		return 0;
	}

	// libmpg123 applies the ReplayGain (and the no-ReplayGain adjustment) through
	// MP3Decoder, and leaves the equalizer to Winamp
	mod.UsesOutputPlug |= IN_MODULE_FLAG_REPLAYGAIN;
	mod.UsesOutputPlug &= ~IN_MODULE_FLAG_EQ;

	mod.is_seekable = 1;
	frameBytes = decoder.FrameBytes();
	const int length = decoder.LengthMilliseconds();
	g_length = length >= 0 ? length : -1000;
	lastfn_data_ready = 1;

	if (!OpenOutput())
		return 0;

	shownBitrate = DisplayBitrate();
	mod.SetInfo(shownBitrate, decoder.SampleRate() / 1000, decoder.Channels(), 1);

	bool decoderDone = false;
	while (!killDecodeThread)
	{
		if (seek_needed != -1)
		{
			Seek(seek_needed);
			decoderDone = false;
		}

		if (decoderDone)
		{
			mod.outMod->CanWrite(); // some output plug-ins need CanWrite() called regularly
			if (!mod.outMod->IsPlaying())
			{
				// everything has been heard, so tell Winamp and end the thread
				if (!killDecodeThread) PostMessage(mod.hMainWindow, WM_WA_MPEG_EOF, 0, 0);
				break;
			}
			Sleep(10);
			continue;
		}

		// DSP plug-ins can double the amount of data (for tempo changes)
		const int chunkBytes = kChunkSamples * frameBytes;
		if (mod.outMod->CanWrite() < chunkBytes * (mod.dsp_isactive() == 1 ? 2 : 1))
		{
			Sleep(10);
			continue;
		}

		size_t bytes = 0;
		const MP3Decoder::Result result = decoder.Read(buffer, chunkBytes, &bytes);
		if (seek_needed != -1)
			continue; // a seek arrived while decoding; this data is from the old position

		if (bytes)
		{
			const int start = decode_pos_ms;
			decode_pos_ms = (int)(decoder.PositionSamples() * 1000 / decoder.SampleRate());

			int samples = (int)(bytes / frameBytes);
			SendToVis(buffer, samples, start);
			if (mod.dsp_isactive())
				samples = mod.dsp_dosamples((short *)buffer, samples, decoder.Bits(), decoder.Channels(), decoder.SampleRate());
			mod.outMod->Write((char *)buffer, samples * frameBytes);
			UpdateBitrate();
		}

		if (result != MP3Decoder::Success)
		{
			// end of the file, or an error libmpg123 couldn't get past
			mod.outMod->Write(0, 0);
			decoderDone = true;
		}
	}

	g_closeaudio = 1;
	return 0;
}

DWORD WINAPI DecodeThread(LPVOID)
{
	DecodeLoop loop;
	return loop.Run();
}
