#pragma once
#if   defined(__linux__)
#include "linux/nxthread.h"
#elif defined(_WIN32)
#include "win/nxthread.h"
#endif
