#ifndef NULLSOFT_WASABI_API_H
#define NULLSOFT_WASABI_API_H

#include "api/syscb/api_syscb.h"
#define WASABI_API_SYSCB sysCallbackApi

#include "api/config/api_config.h"
#define WASABI_API_CONFIG configApi

#include "api/service/api_service.h"

#include "api/application/api_application.h"
#define WASABI_API_APP applicationApi

#include "api/memmgr/api_memmgr.h"
extern api_memmgr *memmgrApi;
#define WASABI_API_MEMMGR memmgrApi

#include "api/filereader/api_filereader.h"
#define WASABI_API_FILE fileApi

#include "nu/threadpool/api_threadpool.h"
extern api_threadpool *threadPoolApi;
#define WASABI_API_THREADPOOL threadPoolApi

#endif