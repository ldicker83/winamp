#ifndef NULLSOFT_TATAKI_API_H
#define NULLSOFT_TATAKI_API_H

#include <api/service/api_service.h>
//extern api_service *serviceManager;
//#define WASABI_API_SVC serviceManager

#include <api/application/api_application.h>
#define WASABI_API_APP applicationApi

#include <api/syscb/api_syscb.h>
#define WASABI_API_SYSCB sysCallbackApi

#include <Agave/Random/api_random.h>
extern api_random *randomApi;
#define AGAVE_API_RANDOM randomApi

#include <api/memmgr/api_memmgr.h>
extern api_memmgr *memoryManager;
#define WASABI_API_MEMMGR memoryManager

#endif