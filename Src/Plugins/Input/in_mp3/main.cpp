//#define PLUGIN_NAME "Nullsoft MPEG Audio Decoder"
#include "main.h"
#include <time.h>
#include "DecodeThread.h"
#include "api__in_mp3.h"
#include "../Winamp/wa_ipc.h"
#include "../nu/ServiceBuilder.h"
#include "config.h"
#include "../nu/Singleton.h"
#include "RawMediaReader.h"

int lastfn_data_ready;

unsigned char config_miscopts=0;
unsigned int winampVersion=0x00005010; // default version # to use if winamp version is 5.1 or less (and therefore doesn't have a valid HWND during Init)
unsigned char config_downsample=0;

int config_max_bufsize_k=128;
int config_gapless=1;
char INI_FILE[MAX_PATH] = {0};

wchar_t lastfn[8192] = {0};	// currently playing file (used for getting info on the current file)

// Used for correcting DSP plug-in pitch changes
int paused = 0;				// are we paused?

volatile int killDecodeThread=0;			// the kill switch for the decode thread
HANDLE thread_handle=INVALID_HANDLE_VALUE;	// the handle to the decode thread

DWORD WINAPI DecodeThread(LPVOID b); // the decode thread procedure

extern char *getfileextensions();


#include "api/service/waservicefactory.h"

#include "FactoryHelper.h"

// wasabi based services for localisation support
HINSTANCE        WASABI_API_LNG_HINST  = 0;
HINSTANCE        WASABI_API_ORIG_HINST = 0;

api_language    *WASABI_API_LNG        = 0;
api_application *WASABI_API_APP        = 0;
api_config      *AGAVE_API_CONFIG      = 0;
api_memmgr      *WASABI_API_MEMMGR     = 0;

static RawMediaReaderService raw_media_reader_service;
static SingletonServiceFactory<svc_raw_media_reader, RawMediaReaderService> raw_factory;

int init()
{
	if (!IsWindow(mod.hMainWindow))
		return IN_INIT_FAILURE;

	winampVersion = (unsigned int)SendMessage(mod.hMainWindow, WM_WA_IPC, 0, IPC_GETVERSION);
	raw_factory.Register(mod.service, &raw_media_reader_service);

	ServiceBuild(AGAVE_API_CONFIG, AgaveConfigGUID);
	ServiceBuild(WASABI_API_LNG, languageApiGUID);
	ServiceBuild(WASABI_API_APP, applicationApiServiceGuid);
	ServiceBuild(WASABI_API_MEMMGR, memMgrApiServiceGuid);

	// need to have this initialised before we try to do anything with localisation features
	WASABI_API_START_LANG(mod.hDllInstance,InMp3LangGUID);

	static wchar_t szDescription[256];
	swprintf(szDescription, 256, WASABI_API_LNGSTRINGW(IDS_NULLSOFT_MPEG_AUDIO_DECODER), PLUGIN_VERSION);
	mod.description = (char*)szDescription;

	config_read();
	mod.FileExtensions=getfileextensions();
	return IN_INIT_SUCCESS;
}

void quit()
{
	ServiceRelease(mod.service, AGAVE_API_CONFIG, AgaveConfigGUID);
	ServiceRelease(mod.service, WASABI_API_APP, applicationApiServiceGuid);
	ServiceRelease(mod.service, WASABI_API_MEMMGR, memMgrApiServiceGuid);
	raw_factory.Deregister(mod.service);
}

int isourfile(const wchar_t *fn)
{
	return 0;
}


int m_force_seek=-1;

// called when winamp wants to play a file
int play(const in_char *fn)
{
	DWORD thread_id;
	paused=0;
	g_length=-1000;
	decode_pos_ms=0;
	seek_needed=m_force_seek;
	m_force_seek=-1;
	killDecodeThread=0;
	g_sndopened=0;
	lastfn_data_ready=0;
	g_closeaudio=0;
	lstrcpynW(lastfn,fn, 8192);
	mod.is_seekable = 0;
	mod.SetInfo(0,0,0,0);

	// launch decode thread
	thread_handle = (HANDLE)CreateThread(NULL,0,(LPTHREAD_START_ROUTINE) DecodeThread,NULL,0,&thread_id);
	SetThreadPriority(thread_handle, (int)AGAVE_API_CONFIG->GetInt(playbackConfigGroupGUID, L"priority", THREAD_PRIORITY_HIGHEST));

	return 0;
}

// standard pause implementation
void pause()
{
	paused=1;
	if (g_sndopened)
		mod.outMod->Pause(1);
}

void unpause()
{
	paused=0;
	if (g_sndopened)
		mod.outMod->Pause(0);
}

int ispaused()
{
	return paused;
}

// stop playing.
void stop()
{
	killDecodeThread=1;
	WaitForSingleObject(thread_handle,INFINITE);
	CloseHandle(thread_handle);
	thread_handle = INVALID_HANDLE_VALUE;
	g_length=-1000;
	lastfn[0]=0;
	if (g_closeaudio)
	{
		g_closeaudio=0;
		mod.outMod->Close();
		mod.SAVSADeInit();
	}
	g_sndopened=0;
	m_force_seek=-1;
}


// returns length of playing track
int getlength()
{
	return g_length;
}


// returns current output position, in ms.
// you could just use return mod.outMod->GetOutputTime(),
// but the dsp plug-ins that do tempo changing tend to make
// that wrong.
int getoutputtime()
{
	if (!lastfn_data_ready||!g_sndopened)
		return 0;

	if (seek_needed!=-1)
		return seek_needed;

	return decode_pos_ms +
	       (mod.outMod->GetOutputTime()-mod.outMod->GetWrittenTime());
}


// called when the user releases the seek scroll bar.
// usually we use it to set seek_needed to the seek
// point (seek_needed is -1 when no seek is needed)
// and the decode thread checks seek_needed.
void setoutputtime(int time_in_ms)
{
	seek_needed=time_in_ms;
	m_force_seek=-1;
}


// standard volume/pan functions
void setvolume(int volume)
{
	mod.outMod->SetVolume(volume);
}
void setpan(int pan)
{
	mod.outMod->SetPan(pan);
}


// Winamp applies its equalizer to the decoded audio (IN_MODULE_FLAG_EQ is never set)
void eq_set(int on, char data[10], int preamp)
{
}

// module definition.

extern In_Module mod =
{
	IN_VER_RET,	// defined in IN2.H
	"nullsoft(in_mp3.dll)",
	0,	// hMainWindow (filled in by winamp)
	0,  // hDllInstance (filled in by winamp)
	0,
	// this is a double-null limited list. "EXT\0Description\0EXT\0Description\0" etc.
	0,	// is_seekable
	1,	// uses output plug-in system
	config,
	about,
	init,
	quit,
	getfileinfo,
	id3Dlg,
	isourfile,
	play,
	pause,
	unpause,
	ispaused,
	stop,

	getlength,
	getoutputtime,
	setoutputtime,

	setvolume,
	setpan,

	0,0,0,0,0,0,0,0,0, // visualization calls filled in by winamp

	0,0, // dsp calls filled in by winamp

	eq_set,

	NULL,		// setinfo call filled in by winamp

	0, // out_mod filled in by winamp
};

// exported symbol. Returns output module.
extern "C"
{
	__declspec(dllexport) In_Module * winampGetInModule2()
	{
		return &mod;
	}
}