#include "main.h"
#include "../nu/FilePath.h"
#include "LAMEInfo.h"
#include "config.h"
#include "../winamp/wa_ipc.h"
#include <Richedit.h>
#include "api__in_mp3.h"
#include "FactoryHelper.h"
#include <shlwapi.h>
#include <strsafe.h>
#include <foundation/error.h>

static bool ScanForFrame(CGioFile *file, int *bytesRead)
{
	unsigned char buffer[512] = {0}; /* don't want to read too much, since most MP3's start at 0 */
	int buflen = 0;

	int checked=0;
	if (file->Peek(buffer, sizeof(buffer), &buflen) != NErr_Success)
		return false;
	unsigned char *b = buffer;
	while (buflen >= 4)
	{
		MPEGFrame frame1;
		frame1.ReadBuffer(b);
		if (frame1.IsSync())
		{
			if (checked)
				file->Read(buffer, checked, &buflen);
			*bytesRead=checked;
			return true;
		}

		checked++;
		buflen--;
		b++;
	}
	if (checked)
		file->Read(buffer, checked, &buflen);
	*bytesRead=checked;
	return false;
}

static bool mp3sync(CGioFile *file)
{
	unsigned char buffer[1448 + 4] = {0}; /* large enough for one max-size frame and the header of the second */
	int buflen = 0;

	static const unsigned long gdwHeaderSyncMask = 0xfffe0c00L;
	unsigned long ulHdr1=0;
	unsigned long ulHdr2=0;

	int bytesChecked=0;
	while (bytesChecked<32768)
	{
		int bytesRead=0;
		if (ScanForFrame(file, &bytesRead))
		{
			if (file->Peek(buffer, sizeof(buffer), &buflen) != NErr_Success)
				return false;

			if (buflen >= 4)
			{
				MPEGFrame frame1;
				frame1.ReadBuffer(buffer);
				if (frame1.IsSync())
				{
					ulHdr1 = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
					int framelength= frame1.FrameSize();
					if (buflen >= (framelength+4))
					{
						unsigned char *b = buffer + framelength;
						buflen -= frame1.FrameSize();
						MPEGFrame frame2;
						frame2.ReadBuffer(b);
						ulHdr2 = (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
						if (!((ulHdr1 ^ ulHdr2) & gdwHeaderSyncMask) && frame2.IsSync())
							return true;
						else
						{

						}
					}
				}
				file->Read(buffer, 1, &buflen);
				bytesChecked++;
			}
		}
		else if (file->EndOf())
				return 0;

		bytesChecked+=bytesRead;
	}
	return false;
}

static const wchar_t *GetMPEGVersionString(int mpegVersion)
{
	switch (mpegVersion)
	{
	case MPEGFrame::MPEG1:
		return L"MPEG-1";
	case MPEGFrame::MPEG2:
		return L"MPEG-2";
	case MPEGFrame::MPEG2_5:
		return L"MPEG-2.5";
	default:
		static wchar_t temp[64];
		return WASABI_API_LNGSTRINGW_BUF(IDS_ERROR,temp,64);
	}
}

static const wchar_t *GetEmphasisString(int emphasis)
{
	static wchar_t tempE[32];
	switch (emphasis)
	{
		case 0:
			return WASABI_API_LNGSTRINGW_BUF(IDS_NONE,tempE,32);
		case 1:
			return WASABI_API_LNGSTRINGW_BUF(IDS_50_15_MICROSEC,tempE,32);
		case 2:
			return WASABI_API_LNGSTRINGW_BUF(IDS_INVALID,tempE,32);
		case 3:
			return L"CITT j.17";
		default:
			return WASABI_API_LNGSTRINGW_BUF(IDS_ERROR,tempE,32);
	}
}

static const wchar_t *GetChannelModeString(int channelMode)
{
	static wchar_t tempM[32];
	switch (channelMode)
	{
		case 0:
			return WASABI_API_LNGSTRINGW_BUF(IDS_STEREO,tempM,32);
		case 1:
			return WASABI_API_LNGSTRINGW_BUF(IDS_JOINT_STEREO,tempM,32);
		case 2:
			return WASABI_API_LNGSTRINGW_BUF(IDS_2_CHANNEL,tempM,32);
		case 3:
			return WASABI_API_LNGSTRINGW_BUF(IDS_MONO,tempM,32);
		default:
			return WASABI_API_LNGSTRINGW_BUF(IDS_ERROR,tempM,32);
	}
}
#define INFO_READ_SIZE 32768
void GetFileDescription(const wchar_t *file, CGioFile &_file, wchar_t *data, size_t datalen)
{
	size_t size = datalen;
	wchar_t *mt = data;

	wchar_t langbuf[256] = {0};
	int flen = _file.GetContentLength();
	StringCchPrintfExW(mt, size, &mt, &size, 0, WASABI_API_LNGSTRINGW_BUF(IDS_PAYLOAD_SIZE, langbuf, 256), _file.GetContentLength());
	unsigned char mp3syncbuf[INFO_READ_SIZE] = {0};
	int inputRead = 0;

	DWORD start = _file.GetCurrentPosition();
	// find position of first sync
	if (!mp3sync(&_file))
		return;
	int syncposition = _file.GetCurrentPosition()-start;

	// advance to first sync
	_file.Peek(mp3syncbuf, INFO_READ_SIZE, &inputRead);

	unsigned int padding = 0;
	unsigned int encoderDelay = 0;

	MPEGFrame frame;
	frame.ReadBuffer(mp3syncbuf);


	int framelen = frame.FrameSize();

	//const CMp3StreamInfo *info = decoder.GetStreamInfo();
	//const CMpegHeader *header = decoder.m_Mbs.GetHdr();

	StringCchPrintfEx(mt, size, &mt, &size, 0, WASABI_API_LNGSTRINGW_BUF(IDS_HEADER_FOUND_AT_X_BYTES, langbuf, 256), _file.GetHeaderOffset() + syncposition);
	if (!padding) padding = _file.postpad;
	if (!encoderDelay) encoderDelay = _file.prepad;

	if (padding || encoderDelay)
		StringCchPrintfEx(mt, size, &mt, &size, 0, WASABI_API_LNGSTRINGW_BUF(IDS_ENC_DELAY_ZERO_PADDING, langbuf, 256), encoderDelay, padding);

	int is_vbr_lens = _file.m_vbr_ms;
	int iLen = (is_vbr_lens ? is_vbr_lens/1000 : ((flen * 8) / frame.GetBitrate()));
	if(iLen > 0)
		StringCchPrintfEx(mt, size, &mt, &size, 0, WASABI_API_LNGSTRINGW_BUF(IDS_LENGTH_X_SECONDS, langbuf, 256), iLen);
	else
	{
		float fLen = (is_vbr_lens ? is_vbr_lens/1000.0f : ((flen * 8.0f) / frame.GetBitrate()));
		StringCchPrintfEx(mt, size, &mt, &size, 0, WASABI_API_LNGSTRINGW_BUF(IDS_LENGTH_X_PART_SECONDS, langbuf, 256), fLen);
	}

	StringCchPrintfEx(mt, size, &mt, &size, 0, WASABI_API_LNGSTRINGW_BUF(IDS_S_LAYER_X, langbuf, 256), GetMPEGVersionString(frame.mpegVersion), frame.GetLayer());

	int frames = _file.m_vbr_frames;

	int is_vbr = _file.m_vbr_flag || _file.m_vbr_hdr;
	if (!is_vbr || _file.encodingMethod == ENCODING_METHOD_CBR)
	{
		if (frames)
		{
			StringCchPrintfEx(mt, size, &mt, &size, 0, WASABI_API_LNGSTRINGW_BUF(IDS_X_KBIT, langbuf, 256), frame.GetBitrate() / 1000, frames);
		}
		else
		{
			StringCchPrintfEx(mt, size, &mt, &size, 0, WASABI_API_LNGSTRINGW_BUF(IDS_X_KBIT_APPROX, langbuf, 256), frame.GetBitrate() / 1000, MulDiv(flen, 8, framelen));
		}
	}
	else if (is_vbr && _file.encodingMethod == ENCODING_METHOD_ABR)
		StringCchPrintfEx(mt, size, &mt, &size, 0, WASABI_API_LNGSTRINGW_BUF(IDS_X_KBIT_ABR, langbuf, 256), _file.GetAvgVBRBitrate(), frames);
	else
	{
		StringCchPrintfEx(mt, size, &mt, &size, 0, WASABI_API_LNGSTRINGW_BUF(IDS_X_KBIT_VBR, langbuf, 256), _file.GetAvgVBRBitrate(), _file.m_vbr_hdr?L"I":L"", frames);
	}

	StringCchPrintfEx(mt, size, &mt, &size, 0, WASABI_API_LNGSTRINGW_BUF(IDS_X_HZ_S, langbuf, 256), frame.GetSampleRate(), GetChannelModeString(frame.channelMode));
	StringCchPrintfEx(mt, size, &mt, &size, 0, L"\r\nCRC: %s", WASABI_API_LNGSTRINGW_BUF((frame.CRC ? IDS_YES : IDS_NO), langbuf, 256));
	wchar_t tmp[16] = {0};
	StringCchPrintfEx(mt, size, &mt, &size, 0, WASABI_API_LNGSTRINGW_BUF(IDS_COPYRIGHTED, langbuf, 256), WASABI_API_LNGSTRINGW_BUF((frame.copyright ? IDS_YES : IDS_NO),tmp,16));
	StringCchPrintfEx(mt, size, &mt, &size, 0, WASABI_API_LNGSTRINGW_BUF(IDS_ORIGINAL, langbuf, 256), WASABI_API_LNGSTRINGW_BUF((frame.original ? IDS_YES : IDS_NO),tmp,16));
	StringCchPrintfEx(mt, size, &mt, &size, 0, WASABI_API_LNGSTRINGW_BUF(IDS_EMPHASIS, langbuf, 256), GetEmphasisString(frame.emphasis));

	if (_file.m_vbr_frame_len && !_file.lengthVerified)
		StringCchPrintfEx(mt, size, &mt, &size, 0, WASABI_API_LNGSTRINGW_BUF(IDS_MP3_HAS_BEEN_MODIFIED_NOT_ALL_MAY_BE_CORRECT, langbuf, 256));
}

void GetAudioInfo(const wchar_t *filename, CGioFile *file, int *len, int *channels, int *bitrate, int *vbr, int *sr)
{
	*bitrate=0;
	*len=0;
	*vbr=0;
	*channels=0;
	if (file)
	{
		if (*bitrate = file->GetAvgVBRBitrate()*1000)
		{
			*len = file->m_vbr_ms;
			*vbr = file->m_vbr_flag || file->m_vbr_hdr;
		}

		if (!mp3sync(file))
			return;

		unsigned char t[4] = {0};
		int n = 0;

		if (file->Peek(t, sizeof(t), &n) != NErr_Success)
			return;

		MPEGFrame frame;
		frame.ReadBuffer(t);
		if (frame.IsSync())
		{
			if (!*bitrate)
			{
				*bitrate = frame.GetBitrate();
				*len = MulDiv(file->GetContentLength(), 1000*8, *bitrate);
			}
			*channels = frame.GetNumChannels();
			*sr = frame.GetSampleRate();
		}
	}
}
