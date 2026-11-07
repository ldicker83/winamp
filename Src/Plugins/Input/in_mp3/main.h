#pragma once
#ifndef  WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "giofile.h"
#include "CVbriHeader.h"
#include "resource.h"
#include "in2.h"

#define PLUGIN_VERSION L"4.6"

extern In_Module mod;

extern char INI_FILE[MAX_PATH];
extern int g_length;
extern int lastfn_data_ready;
extern int id3Dlg(const wchar_t *fn, HWND hwnd);
extern int getlength();
extern void getfileinfo(const wchar_t *filename, wchar_t *title, int *length_in_ms);

extern int config_max_bufsize_k;
extern int config_gapless;
extern unsigned char config_miscopts;
extern unsigned char config_downsample;

extern wchar_t lastfn[8192];	// currently playing file (used for getting info on the current file)
extern int paused;				// are we paused?
extern void  config_read();
extern void about(HWND hwndParent);

extern void config(HWND hwndParent);
extern volatile int killDecodeThread;

extern unsigned int winampVersion;

#if !defined(__alpha) && !defined(_WIN64)
static __inline long float_to_long(double t)
{
	long r;
	__asm fld t
	__asm fistp r
	return r;
}
#else
#define float_to_long(x) ((long)( x ))
#endif

void w9x_lstrcpynW(wchar_t *dest, const wchar_t *src, int maxLen);

// maximum acceptable deviance between LAME header bytesize and real filesize (minus id3 tag)
// has to be large enough to accomodate unknown tags (APE, lyrics3) 
const int MAX_ACCEPTABLE_DEVIANCE = 16384; 
void get_extended_info(const wchar_t *fn, int *len);

void ConvertTryUTF8(const char *in, wchar_t *out, size_t outlen);

extern char config_extlist[129];

extern int m_force_seek;

int _r_i(char *name, int def);
void _w_i(char *name, int d);