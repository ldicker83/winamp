#ifndef __WASABI_API_H
#define __WASABI_API_H

#include "wasabicfg.h"
#include "ServiceManager.h"
#include "SysCallbacks.h"
#include "MemoryManager.h"

#define WASABI_API_SVC serviceManager
#define WASABI_API_SYSCB sysCallbacks
#define WASABI_API_MEMMGR memoryManager

#include "../tagz/api_tagz.h"
extern api_tagz *tagz;
#define WINAMP5_API_TAGZ tagz


#include "application.h"
#define WASABI_API_APP application

#include "Metadata.h"
#define WASABI_API_METADATA metadata

#include "../nu/threadpool/ThreadPool.h"
extern ThreadPool *threadPool;
#define WASABI_API_THREADPOOL threadPool

#include "stats.h"
#define AGAVE_API_STATS (&stats)

/* Services we need from W5S services */
#include "../Agave/AlbumArt/api_albumart.h"
extern api_albumart *albumArtApi;
#define AGAVE_API_ALBUMART albumArtApi
#include "../playlist/api_playlistmanager.h"
extern api_playlistmanager *playlistManager;
#define AGAVE_API_PLAYLISTMANAGER playlistManager

#include "../playlist/api_playlists.h"
extern api_playlists *playlistsManager;
#define AGAVE_API_PLAYLISTS playlistsManager



#include "./winampApi.h"
extern WinampApi *winampApi;
#define WASABI_API_WINAMP winampApi



#endif