#pragma once

#include "types.h"
# include "win32.h"

#include "../std_mkncc.h"


#define OSMODULEHANDLE	HINSTANCE
#define INVALIDOSMODULEHANDLE	((OSMODULEHANDLE)0)
#define OSWINDOWHANDLE	HWND
#define INVALIDOSWINDOWHANDLE	((OSWINDOWHANDLE)0)
#define OSICONHANDLE	HICON
#define INVALIDOSICONHANDLE	((OSICONHANDLE)0)
#define OSCURSORHANDLE	HICON
#define INVALIDOSCURSORHANDLE	((OSCURSORHANDLE)0)
#define OSTHREADHANDLE	HANDLE
#define INVALIDOSTHREADHANDLE	((OSTHREADHANDLE)0)
#define OSREGIONHANDLE	HRGN
#define INVALIDOSREGIONHANDLE	((OSREGIONHANDLE)0)
typedef HMENU OSMENUHANDLE;

#define RGBA(r,g,b,a)	((ARGB32)((uint8_t)(r) | ((uint8_t)(g) << 8) | ((uint8_t)(b) << 16) | ((uint8_t)(a) << 24)))

#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#ifdef WIN32
#define OSPIPE HANDLE
#define OSPROCESSID int
#endif

// Ode macro keyworkds
#define DISPATCH_   // makes a method dispatchable, automatically assigns a free ID (requires Interface)
#define DISPATCH(x) // makes a method dispatchable and specifies its ID (not duplicate check, requires Interface)
#define NODISPATCH  // prevents a method from being dispatched if the class is marked for dispatching by default
#define EVENT       // marks a method as being an event to which other classes can connect to receive notification (used by Script and DependentItem helpers)
#define SCRIPT      // exposes a method to script interfaces (requires Script helper)
#define IN          // Input parameter
#define OUT         // Output parameter
#define INOUT       // Input/Output parameter
