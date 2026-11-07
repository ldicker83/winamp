
/***************************************************************************\
 *
 *               (C) copyright Fraunhofer - IIS (1998)
 *                        All Rights Reserved
 *
 *   filename: giofile.h
 *   project : MPEG Decoder
 *   author  : Martin Sieler
 *   date    : 1998-02-11
 *   contents/description: HEADER - file I/O class for MPEG Decoder
 *
 *
\***************************************************************************/

#ifndef _GIOFILE_H
#define _GIOFILE_H

/* ------------------------ includes --------------------------------------*/
#include <windows.h>
#include <fstream>

#include "CVbriHeader.h"

#include "ID3v2.h"
#include "LAMEInfo.h"
#include "../nu/RingBuffer.h"
#include "../apev2/tag.h"

/*-------------------------- defines --------------------------------------*/

/*-------------------------------------------------------------------------*/

// Reads an MPEG audio file's tags and header frames. Decoding is done by
// libmpg123 (see MP3Decoder), which uses AudioStart() and AudioEnd() to skip
// the tags this class finds.
class CGioFile
{
public:
	CGioFile();
	~CGioFile();

	int  Open(const wchar_t *pszName, int maxbufsizek);
	int  Close();
	int  Read(void *pBuffer, int cbToRead, int *pcbRead);
	int  Peek(void *pBuffer, int cbToRead, int *pcbRead);

	// byte range of the MPEG audio in the file: from the first frame (including a
	// Xing/LAME/VBRI header frame) up to the first trailing tag
	uint64_t AudioStart() const { return audio_start; }
	uint64_t AudioEnd() const { return mpeg_position + mpeg_length; }

	int EndOf(void)
	{
		return IsEof();
	}

	unsigned int GetAvgVBRBitrate(void)
	{
		if (m_vbr_ms && m_vbr_frames)
		{
			if (m_vbr_bytes && encodingMethod != ENCODING_METHOD_CBR)
				return (unsigned int)(m_vbr_bytes * 8 / m_vbr_ms);
			else
				return (unsigned int)(mpeg_length * 8 / m_vbr_ms);
		}
		return 0;
	}
	bool IsEof() const;
	bool lengthVerified;

	int GetHeaderOffset();

	DWORD GetContentLength(void)   const;
	DWORD GetCurrentPosition(void) const;
	void SetCurrentPosition(long dwPos, int How);

	enum { GIO_FILE_BEGIN, GIO_FILE_CURRENT, GIO_FILE_END };

	uint64_t mpeg_length; /* length of valid audio data */
	uint64_t mpeg_position; /* starting position of first valid decodable non-header MPEG frame */
	uint64_t file_position; /* position within the MPEG data that we've read so far */

	uint64_t m_vbr_bytes;
	int m_vbr_frames, m_vbr_ms;
	int encodingMethod;
	uint64_t m_vbr_samples;
	int prepad, postpad;

	unsigned char id3v1_data[128];

	ID3v2 info;
	float GetGain();
	unsigned char m_vbr_toc[100];
	int m_vbr_frame_len;
	int m_vbr_flag;
	CVbriHeader *m_vbr_hdr;

	void *GetID3v1()
	{
		if (m_id3v1_len == 128)
			return id3v1_data;
		else
			return 0;
	}

	void *GetID3v2(uint32_t *len)
	{
		if (stream_id3v2_buf)
		{
			*len = stream_id3v2_read;
			return stream_id3v2_buf;
		}
		else
			return 0;
	}

	void *GetLyrics3(uint32_t *len)
	{
		if (lyrics3_data)
		{
			*len = lyrics3_size;
			return lyrics3_data;
		}
		else
			return 0;
	}

	void *GetAPEv2(uint32_t *len)
	{
		if (apev2_data)
		{
			*len = m_apev2_len;
			return apev2_data;
		}
		return 0;
	}

protected:
	void ReadiTunesGaps();
private:
	uint64_t audio_start;

	/* ID3v2 */
	int m_id3v1_len;

	/* ID3v2 */
	uint32_t stream_id3v2_read;
	char *stream_id3v2_buf;
	int m_id3v2_len;

	/* Lyrics3 */
	uint32_t lyrics3_size;
	char *lyrics3_data;

	/* APEv2 */
	uint32_t m_apev2_len;
	char *apev2_data;
	APEv2::Tag apev2;

	unsigned char *m_full_buffer;
	uint64_t m_full_buffer_pos/*,m_full_buffer_len*/;

	RingBuffer peekBuffer;
	//unsigned char m_peekbuf[8192];
	//int m_peekbuf_used;
	bool no_peek_hack;



	mutable std::ifstream m_file;
	bool   fEof;
};

/*-------------------------------------------------------------------------*/
#endif
