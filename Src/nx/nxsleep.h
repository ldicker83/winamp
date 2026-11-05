#pragma once
#if   defined(__linux__)
#include "linux/nxsleep.h"
#elif defined(_WIN32)
#include "win/nxsleep.h"
#endif
