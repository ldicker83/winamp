/***************************************************************************\
*
*               (C) copyright Fraunhofer - IIS (1998)
*                        All Rights Reserved
*
*   filename: giofile.cpp
*   project : MPEG Decoder
*   author  : Martin Sieler
*   date    : 1998-02-11
*   contents/description: file I/O class for MPEG Decoder
*
*
\***************************************************************************/

/* ------------------------ includes --------------------------------------*/

#include "main.h"
#include "api__in_mp3.h"
#include <time.h>
#include <locale.h>
#include "../Winamp/wa_ipc.h"

#include "LAMEinfo.h"
#include "OFL.h"
#include "../nu/AutoChar.h"
#include "../nu/AutoWide.h"

#include <api/service/waservicefactory.h>
#include <shlwapi.h>
#include "MP3Info.h"
#include "config.h"
#include <math.h>
#include "id3.h"
#include "../apev2/header.h"
#include <foundation/error.h>
#include <strsafe.h>


#define SAFE_MALLOC_MATH(orig, x) (((orig)<x)?x:orig)
// seems like a reasonable limit, but we should check with tag first

static int64_t Seek64(HANDLE hf, int64_t distance, DWORD MoveMethod)
{
	LARGE_INTEGER li;
	li.QuadPart = distance;
	li.LowPart = SetFilePointer (hf, li.LowPart, &li.HighPart, MoveMethod);
	if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
	{
		li.QuadPart = -1;
	}
	return li.QuadPart;
}

HWND GetDialogBoxParent()
{
	HWND parent = (HWND)SendMessage(mod.hMainWindow, WM_WA_IPC, 0, IPC_GETDIALOGBOXPARENT);
	if (!parent || parent == (HWND)1)
		return mod.hMainWindow;
	return parent;
}

/*-------------------------- defines --------------------------------------*/

/*-------------------------------------------------------------------------*/

//-------------------------------------------------------------------------*
//
//                   CGioFile
//
//-------------------------------------------------------------------------*

//-------------------------------------------------------------------------*
//   constructor
//-------------------------------------------------------------------------*


extern int lastfn_status_err;

CGioFile::CGioFile()
{
	mpeg_length=0;
	file_position=0;
	mpeg_position=0;
	no_peek_hack=false;
	encodingMethod=0;
	ZeroMemory(&last_write_time, sizeof(last_write_time));
	ZeroMemory(id3v1_data, sizeof(id3v1_data));
	lengthVerified=false;
	m_vbr_bytes = 0;
	prepad = 0;
	postpad = 0;
	m_vbr_ms = 0;
	m_vbr_hdr = 0;
	stream_id3v2_buf = 0;
	lyrics3_size=0;
	lyrics3_data=0;
	apev2_data=0;
	m_full_buffer = NULL;
	fEof = false;
	hFile = INVALID_HANDLE_VALUE;
	m_vbr_frames = 0;
	ZeroMemory(&m_vbr_toc, sizeof(m_vbr_toc));
	m_vbr_frame_len = 0;
	m_vbr_flag = 0;
	m_id3v2_len = 0;
	m_id3v1_len = 0;

}

//-------------------------------------------------------------------------*
//   destructor
//-------------------------------------------------------------------------*

CGioFile::~CGioFile()
{
	free(stream_id3v2_buf);
	free(lyrics3_data);
	free(apev2_data);

	Close();
	delete m_vbr_hdr;
	m_vbr_hdr = 0;
}

static bool GetRG(const char *key, ID3v2 &info, APEv2::Tag &apev2, wchar_t *val, int len)
{
	val[0]=0;
	if (info.GetString(key, val, len) == 1 && val[0])
		return true;
	if (apev2.GetString(key, val, len) == APEv2::APEV2_SUCCESS && val[0])
		return true;
	return false;
}

float CGioFile::GetGain()
{
	if (AGAVE_API_CONFIG->GetBool(playbackConfigGroupGUID, L"replaygain", false))
	{
		//		if (info.HasData())
		{
			float dB = 0, peak = 1.0f;
			wchar_t gain[128] = L"", peakVal[128] = L"";
			_locale_t C_locale = WASABI_API_LNG->Get_C_NumericLocale();

			switch (AGAVE_API_CONFIG->GetUnsigned(playbackConfigGroupGUID, L"replaygain_source", 0))
			{
			case 0:  // track
				if ((GetRG("replaygain_track_gain", info, apev2, gain, 128) == false || !gain[0])
					&& !AGAVE_API_CONFIG->GetBool(playbackConfigGroupGUID, L"replaygain_preferred_only", false))
					GetRG("replaygain_album_gain", info, apev2, gain, 128);

				if ((GetRG("replaygain_track_peak", info, apev2, peakVal, 128) == false || !peakVal[0])
					&& !AGAVE_API_CONFIG->GetBool(playbackConfigGroupGUID, L"replaygain_preferred_only", false))
					GetRG("replaygain_album_peak", info, apev2, peakVal, 128);
				break;
			case 1:
				if ((GetRG("replaygain_album_gain", info, apev2, gain, 128) == false || !gain[0])
					&& !AGAVE_API_CONFIG->GetBool(playbackConfigGroupGUID, L"replaygain_preferred_only", false))
					GetRG("replaygain_track_gain", info, apev2, gain, 128);

				if ((GetRG("replaygain_album_peak", info, apev2, peakVal, 128) == false || !peakVal[0])
					&& !AGAVE_API_CONFIG->GetBool(playbackConfigGroupGUID, L"replaygain_preferred_only", false))
					GetRG("replaygain_track_peak", info, apev2, peakVal, 128);
				break;
			}

			if (gain[0])
			{
				if (gain[0] == L'+')
					dB = (float)_wtof_l(&gain[1], C_locale);
				else
					dB = (float)_wtof_l(gain, C_locale);
			}
			else
			{
				dB = AGAVE_API_CONFIG->GetFloat(playbackConfigGroupGUID, L"non_replaygain", -6.0);
				return (float)pow(10.0f, dB / 20.0f);
			}

			if (peakVal[0])
			{
				peak = (float)_wtof_l(peakVal, C_locale);
			}

			switch (AGAVE_API_CONFIG->GetUnsigned(playbackConfigGroupGUID, L"replaygain_mode", 1))
			{
			case 0:  // apply gain
				return (float)pow(10.0f, dB / 20.0f);
			case 1:  // apply gain, but don't clip
				return min((float)pow(10.0f, dB / 20.0f), 1.0f / peak);
			case 2:  // normalize
				return 1.0f / peak;
			case 3:  // prevent clipping
				if (peak > 1.0f)
					return 1.0f / peak;
				else
					return 1.0f;
			}
		}
		/*	else
		{
		float dB = AGAVE_API_CONFIG->GetFloat(playbackConfigGroupGUID, L"non_replaygain", -6.0);
		return pow(10.0f, dB / 20.0f);
		}*/
	}

	return 1.0f; // no gain
}
//-------------------------------------------------------------------------*
//   Open
//-------------------------------------------------------------------------*

int CGioFile::Open(const wchar_t *pszName, int maxbufsizek)
{
	peekBuffer.reserve(16384);

	m_vbr_flag      = 0;
	m_vbr_frames    = 0;
	m_vbr_samples   = 0;
	m_vbr_ms        = 0;
	m_vbr_frame_len = 0;
	
	m_id3v2_len     = 0;
	m_id3v1_len     = 0;
	m_apev2_len     = 0;
	
	lyrics3_size    = 0;
	
	
	mpeg_length     = 0;
	mpeg_position   = 0;
	file_position   = 0;
	
	if ( !_wcsnicmp( pszName, L"file://", 7 ) )
		pszName += 7;

	if ( PathIsURL( pszName ) )
		return NErr_ConnectionFailed;
	else
	{
		hFile = CreateFile(pszName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);

		if (hFile != INVALID_HANDLE_VALUE)
		{
			GetFileTime( hFile, 0, 0, &last_write_time );
			mpeg_length = Seek64( hFile, 0, FILE_END );

			uint64_t startOffset = 0;
			unsigned char buf[ 1448 ] = { 0 };

			DWORD len = 0;
			while ( 1 ) // read all tags (sometimes programs get stupid and make multiple tags)
			{
				len = 0;
				Seek64( hFile, startOffset, FILE_BEGIN );
				ReadFile( hFile, buf, 10, &len, NULL );

				if ( len >= 10 &&
					 buf[ 0 ] == 'I' &&
					 buf[ 1 ] == 'D' &&
					 buf[ 2 ] == '3' &&
					 buf[ 3 ] != 255 &&
					 buf[ 4 ] != 255 &&
					 buf[ 6 ] < 0x80 &&
					 buf[ 7 ] < 0x80 &&
					 buf[ 8 ] < 0x80 &&
					 buf[ 9 ] < 0x80 )
				{
					DWORD thisLen = 10;
					if ( buf[ 5 ] & 0x10 ) // check for footer flag
						thisLen += 10;

					thisLen += ( (int)buf[ 6 ] ) << 21;
					thisLen += ( (int)buf[ 7 ] ) << 14;
					thisLen += ( (int)buf[ 8 ] ) << 7;
					thisLen += ( (int)buf[ 9 ] );

					SetFilePointer( hFile, -10, NULL, FILE_CURRENT );

					if ( stream_id3v2_buf ) // already read a tag?
					{
						startOffset   += thisLen;
						m_id3v2_len   += thisLen;
						mpeg_length   -= thisLen;
						mpeg_position += thisLen;
						
						SetFilePointer( hFile, thisLen, NULL, FILE_CURRENT );

						continue;
					}

					stream_id3v2_buf = (char *)malloc( thisLen );
					if ( stream_id3v2_buf )
					{
						memcpy( stream_id3v2_buf, buf, 10 );

						DWORD dummy = 0;
						if ( !ReadFile( hFile, stream_id3v2_buf, thisLen, &dummy, 0 ) || dummy < thisLen )
						{
							free( stream_id3v2_buf );
							stream_id3v2_buf = NULL;
							
							thisLen = 0;
							SetFilePointer( hFile, 0, NULL, FILE_END );

							break;
						}
						else
						{
							mpeg_position     += thisLen;
							stream_id3v2_read  = thisLen;
							mpeg_length       -= thisLen;
							startOffset       += thisLen;

							info.Decode( stream_id3v2_buf, thisLen );
						}
					}
					else
					{
						/* memory allocation failed, let's assume the ID3v2 tag was valid ... */
						mpeg_position     += thisLen;
						stream_id3v2_read  = thisLen;
						mpeg_length       -= thisLen;
						startOffset       += thisLen;
					}

					m_id3v2_len += thisLen;
				}
				else
				{
					// benski> unnecessary because we call SetFilePointer immediately after the loop:
					// CUT: SetFilePointer(hFile, -10, NULL, FILE_CURRENT);
					break;
				}
			}

			/* Read ID3v1 Tag */
			if ( mpeg_length >= 128 )
			{
				SetFilePointer( hFile, -128, NULL, FILE_END );

				len = 0;
				if ( ReadFile( hFile, id3v1_data, 128, &len, NULL ) && len == 128 )
				{
					if ( id3v1_data[ 0 ] == 'T' && id3v1_data[ 1 ] == 'A' && id3v1_data[ 2 ] == 'G' )
					{
						m_id3v1_len  = 128;
						mpeg_length -= m_id3v1_len;
					}
				}
			}

			/* read appended ID3v2.4 tag */
			if ( mpeg_length >= 10 && Seek64( hFile, mpeg_position + mpeg_length - 10, FILE_BEGIN ) != -1 )
			{
				len = 0;
				ReadFile( hFile, buf, 10, &len, NULL );
				if ( len >= 10 &&
					 buf[ 0 ] == '3' &&
					 buf[ 1 ] == 'D' &&
					 buf[ 2 ] == 'I' &&
					 buf[ 3 ] != 255 &&
					 buf[ 4 ] != 255 &&
					 buf[ 6 ] < 0x80 &&
					 buf[ 7 ] < 0x80 &&
					 buf[ 8 ] < 0x80 &&
					 buf[ 9 ] < 0x80 )
				{
					DWORD thisLen = 10;
					if ( buf[ 5 ] & 0x10 ) // check for header flag
						thisLen += 10;

					thisLen += ( (int)buf[ 6 ] ) << 21;
					thisLen += ( (int)buf[ 7 ] ) << 14;
					thisLen += ( (int)buf[ 8 ] ) << 7;
					thisLen += ( (int)buf[ 9 ] );

					mpeg_length -= thisLen;
				}
			}

			/* Read Lyrics3 Tag */
			if ( mpeg_length >= 15 )
			{
				free( lyrics3_data );
				lyrics3_data = NULL;

				lyrics3_size = 0;
				char lyrics3_end_signature[ 15 ] = { 0 };

				Seek64( hFile, ( mpeg_position + mpeg_length - 15 ), FILE_BEGIN );

				len = 0;
				if ( ReadFile( hFile, lyrics3_end_signature, 15, &len, NULL ) && len == 15 )
				{
					if ( !memcmp( lyrics3_end_signature + 6, "LYRICS200", 9 ) )
					{
						lyrics3_size = strtoul( lyrics3_end_signature, 0, 10 );
						if ( lyrics3_size )
						{
							lyrics3_data = (char *)malloc( lyrics3_size );
							if ( lyrics3_data )
							{
								SetFilePointer( hFile, -(LONG)( 15 + lyrics3_size ), NULL, FILE_CURRENT );

								len = 0;
								ReadFile( hFile, lyrics3_data, lyrics3_size, &len, 0 );
								if ( len != lyrics3_size || memcmp( lyrics3_data, "LYRICSBEGIN", 11 ) )
								{
									free( lyrics3_data );
									lyrics3_data = NULL;

									lyrics3_size = 0;
								}
								else
								{
									mpeg_length -= lyrics3_size + 15;
								}
							}
						}
					}
				}
			}

			if ( mpeg_length >= 32 )
			{
				/* Read APEv2 Tag */
				free( apev2_data );
				apev2_data = NULL;
				
				char ape[ 32 ] = { 0 };
				Seek64( hFile, ( mpeg_position + mpeg_length - 32 ), FILE_BEGIN );

				len = 0;
				if ( ReadFile( hFile, ape, 32, &len, NULL ) && len == 32 )
				{
					APEv2::Header footer( ape );
					if ( footer.Valid() )
					{
						m_apev2_len = footer.TagSize();
						if ( mpeg_length >= m_apev2_len )
						{
							Seek64( hFile, -(int64_t)( m_apev2_len ), FILE_CURRENT );

							apev2_data = (char *)malloc( m_apev2_len );
							if ( apev2_data )
							{
								len = 0;
								ReadFile( hFile, apev2_data, m_apev2_len, &len, NULL );
								if ( len != m_apev2_len || apev2.Parse( apev2_data, m_apev2_len ) != APEv2::APEV2_SUCCESS )
								{
									free( apev2_data );
									apev2_data = NULL;

									m_apev2_len = 0;
								}
							}

							mpeg_length -= m_apev2_len;
						}
					}
				}
			}

			{
				Seek64( hFile, mpeg_position, FILE_BEGIN );

				len = 0;
				ReadFile( hFile, buf, sizeof( buf ), &len, NULL );
				
				delete m_vbr_hdr;
				m_vbr_hdr = NULL;
				
				LAMEinfo lame;
				lame.toc = m_vbr_toc;

				m_vbr_frame_len = ReadLAMEinfo( buf, &lame );
				if ( m_vbr_frame_len )
				{
					lengthVerified = false;
					prepad         = lame.encoderDelay;
					postpad        = lame.padding;

					encodingMethod = lame.encodingMethod;
					if ( !encodingMethod && lame.cbr )
						encodingMethod = ENCODING_METHOD_CBR;

					if ( lame.flags & TOC_FLAG )
					{
						int x;
						for ( x = 0; x < 100; x++ )
							if ( m_vbr_toc[ x ] ) break;

						if ( x != 100 )
							m_vbr_flag = 1;
					}

					if ( lame.flags & BYTES_FLAG )
					{
						// some programs are stupid and count the id3v2 length in the lame header
						if ( mpeg_length + m_id3v2_len == lame.bytes || mpeg_length + m_id3v2_len + m_id3v1_len == lame.bytes )
						{
							m_vbr_bytes    = mpeg_length;
							lengthVerified = true;
						}
						else if ( abs( (int)mpeg_length - lame.bytes ) < MAX_ACCEPTABLE_DEVIANCE )
						{
							m_vbr_bytes    = lame.bytes;
							lengthVerified = true;
						}
					}

					if ( lame.flags & FRAMES_FLAG
						 && m_vbr_bytes && lengthVerified ) // only use the length if we're sure it's unmodified
					{
						m_vbr_frames   = lame.frames;
						m_vbr_samples  = Int32x32To64( lame.frames, lame.h_id ? 1152 : 576 );
						m_vbr_samples -= ( prepad + postpad );
						m_vbr_ms       = MulDiv( (int)m_vbr_samples, 1000, lame.samprate );
					}

					if ( !m_vbr_frames || encodingMethod == ENCODING_METHOD_CBR )
						m_vbr_flag = 0;

					mpeg_length   -= m_vbr_frame_len;
					mpeg_position += m_vbr_frame_len;
				}
				else
				{
					m_vbr_hdr    = new CVbriHeader;

					m_vbr_frame_len = m_vbr_hdr->readVbriHeader(buf);
					if (m_vbr_frame_len)
					{
						m_vbr_bytes     = m_vbr_hdr->getBytes();
						m_vbr_frames    = m_vbr_hdr->getNumFrames();
						m_vbr_ms        = m_vbr_hdr->getNumMS();
						
						lengthVerified  = true;

						mpeg_length    -= m_vbr_frame_len;
						mpeg_position  += m_vbr_frame_len;
					}
					else
					{
						delete m_vbr_hdr;
						m_vbr_hdr = NULL;
					}
				}
			}

			// read OFL
			{
				Seek64( hFile, mpeg_position, FILE_BEGIN );
				len = 0;
				ReadFile( hFile, buf, sizeof( buf ), &len, NULL );
				MPEGFrame frame;
				frame.ReadBuffer( buf );
				OFL ofl;
				if ( ofl.Read( frame, &buf[ 4 ], len - 4 ) == NErr_Success )
				{
					m_vbr_ms      = (int)ofl.GetLengthSeconds() * 1000;
					m_vbr_samples = ofl.GetSamples();

					m_vbr_frames = ofl.GetFrames();
					size_t pre, post;
					if ( ofl.GetGaps( &pre, &post ) == NErr_Success )
					{
						prepad  = (int)pre - 529;
						postpad = (int)post + 529;
					}
				}
			}

			ReadiTunesGaps();

			Seek64( hFile, mpeg_position, FILE_BEGIN );

			if ( maxbufsizek * 1024 >= mpeg_length )
			{
				DWORD m_full_buffer_len = 0;
				m_full_buffer = new unsigned char[ (unsigned int)mpeg_length ];
				if ( !ReadFile( hFile, m_full_buffer, (DWORD)mpeg_length, &m_full_buffer_len, NULL ) )
				{
					CloseHandle( hFile );
					hFile = INVALID_HANDLE_VALUE;
					delete[] m_full_buffer;
					m_full_buffer = NULL;
					return NErr_Error;
				}
				CloseHandle( hFile );
				hFile = INVALID_HANDLE_VALUE;
				m_full_buffer_pos = 0;
			}
			else
			{
				m_full_buffer = NULL;
			}

			fEof = false;
			return NErr_Success;
		}
		else
		{
			//DWORD dwError = GetLastError();
			return NErr_Error;
		}
	}

	return NErr_Success;
}

//-------------------------------------------------------------------------*
//   Close
//-------------------------------------------------------------------------*

int CGioFile::Close()
{
	int dwReturn = NErr_Success;

	delete [] m_full_buffer;
	m_full_buffer = NULL;

	if (hFile != INVALID_HANDLE_VALUE)
	{
		dwReturn = CloseHandle(hFile) ? NErr_Success : NErr_Error;
		hFile = INVALID_HANDLE_VALUE;
	}

	return dwReturn;
}

class GioFileFiller : public Filler
{
public:
	GioFileFiller(CGioFile *_file)
	{
		ret=NErr_Success;
		file=_file;
	}

	size_t Read(void *dest, size_t len)
	{
		int bytesRead=0;
		ret = file->Read(dest, (int)len, &bytesRead);
		return bytesRead;
	}

	CGioFile *file;
	int ret;
};

int CGioFile::Peek(void *pBuffer, int cbToRead, int *pcbRead)
{
	GioFileFiller filler(this);

	// do we need to fill up?
	if (cbToRead > (int)peekBuffer.size())
	{
		no_peek_hack = true;// benski> HACK ALERT!  we have to set this or else Read() will try to use the peek buffer
		peekBuffer.fill(&filler, cbToRead - peekBuffer.size());
		no_peek_hack=false;
	}

	*pcbRead = (int)peekBuffer.peek(pBuffer, cbToRead);
	return filler.ret;
}

//-------------------------------------------------------------------------*
//   Read
//-------------------------------------------------------------------------*

int CGioFile::Read(void *pBuffer, int cbToRead, int *pcbRead)
{
	if (!no_peek_hack && peekBuffer.size())
	{
		*pcbRead = (int)peekBuffer.read(pBuffer, cbToRead);
		return NErr_Success;
	}

	BOOL bSuccess;
	if (m_full_buffer)
	{
		int len = cbToRead;
		if ((uint64_t)len > (mpeg_length - m_full_buffer_pos))
		{
			len = (int)(mpeg_length - m_full_buffer_pos);
		}
		if (pcbRead) *pcbRead = len;
		if (len)
		{
			memcpy(pBuffer, m_full_buffer + m_full_buffer_pos, len);
			m_full_buffer_pos += len;
		}
		else
		{
			fEof = true;
			return NErr_EndOfFile;
		}
		return NErr_Success;
	}
	else
	{
		if (hFile != INVALID_HANDLE_VALUE)
		{
			if ((uint64_t)cbToRead >= (mpeg_length - file_position))
			{
				cbToRead = (int)(mpeg_length - file_position);
				fEof = true;
			}

			if (cbToRead == 0)
			{
				if (pcbRead)
					*pcbRead = 0;
				return NErr_Success;
			}

			DWORD dwRead = 0;
			bSuccess = ReadFile(hFile, pBuffer, cbToRead, &dwRead, NULL);

			if (bSuccess)
			{
				file_position += dwRead;
				// update pcbRead
				if (pcbRead)
					*pcbRead = dwRead;

				// check for EOF
				if (dwRead == 0)
					fEof = true;

				return NErr_Success;
			}
			else
			{
				// error reading from file
				return NErr_Error;
			}
		}
		else
		{
			// no valid file handle
			return NErr_Error;
		}
	}
}

//-------------------------------------------------------------------------*
//   IsEof
//-------------------------------------------------------------------------*

bool CGioFile::IsEof() const
{
	return fEof;
}

//-------------------------------------------------------------------------*
//   GetContentLength
//-------------------------------------------------------------------------*

DWORD CGioFile::GetContentLength(void) const
{
	DWORD dwSize = 0 ;

	dwSize = (DWORD)mpeg_length;

	return dwSize ;
}

//-------------------------------------------------------------------------*
//   GetCurrentPosition
//-------------------------------------------------------------------------*

DWORD CGioFile::GetCurrentPosition(void) const
{
	DWORD dwPos = 0;

	if (m_full_buffer)
	{
		dwPos = (DWORD)m_full_buffer_pos;
	}
	else if (hFile != INVALID_HANDLE_VALUE)
	{
		dwPos = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
		dwPos -= (DWORD)(mpeg_position + peekBuffer.size());
	}

	return dwPos ;
}

//-------------------------------------------------------------------------*
//   SetCurrentPosition
//-------------------------------------------------------------------------*

void CGioFile::SetCurrentPosition(long dwPos, int How) // GIO_FILE_BEGIN only
{
	fEof = false;
	if (m_full_buffer)
	{
		m_full_buffer_pos = dwPos;
		if (m_full_buffer_pos < 0) m_full_buffer_pos = 0;
		if (m_full_buffer_pos > mpeg_length) m_full_buffer_pos = mpeg_length;
	}
	else if (hFile != INVALID_HANDLE_VALUE)
	{
		Seek64(hFile, mpeg_position + dwPos, FILE_BEGIN);
		file_position = dwPos;
		peekBuffer.clear();
	}
}

int CGioFile::GetHeaderOffset()
{
	return (int)mpeg_position;
}
/*-------------------------------------------------------------------------*/

void CGioFile::Seek(int posms, int br)
{
	int offs = 0;
	if (m_vbr_hdr)
	{
		offs = m_vbr_hdr->seekPointByTime((float)posms);
	}
	else if (!m_vbr_flag)
		offs = MulDiv(posms, br, 8);
	else
	{
		int ms = 0;
		int fl = GetContentLength();
		if (!m_vbr_frames)
		{
			ms = MulDiv(fl, 8 * 1000, br);
		}
		else ms = m_vbr_ms;
		offs = SeekPoint(m_vbr_toc, fl, (float)posms / ((float)ms / 100.0f));
	}
	SetCurrentPosition(offs, GIO_FILE_BEGIN);
}

bool CGioFile::IsSeekable()
{
	return true;
}

void CGioFile::GetStreamInfo(wchar_t *, size_t)
{
}

static inline const wchar_t *IncSafe(const wchar_t *val, int x)
{
	while (x--)
	{
		if (val && *val)
			val++;
	}
	return val;
}

void CGioFile::ReadiTunesGaps()
{
	if (info.HasData() && !prepad && !postpad)
	{
		wchar_t str[128] = {0};
		if (info.GetString("pregap", str, 128) == 1)
			prepad = _wtoi(str);

		str[0]=0;
		if (info.GetString("postgap", str, 128) == 1)
			postpad = _wtoi(str);
	}
}

#define CBCLASS CGioFile
START_DISPATCH;
CB( MPEGSTREAM_PEEK, Peek )
CB( MPEGSTREAM_READ, Read )
CB( MPEGSTREAM_EOF,  EndOf )
CB( MPEGSTREAM_GAIN, GetGain )
END_DISPATCH;
#undef CBCLASS