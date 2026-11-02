#pragma once

#ifndef _WA_LOGGER_HEADER
#define _WA_LOGGER_HEADER

#include <string>
#include <windows.h>

#include "WAT.h"

inline void wa_log( const std::string &text )
{
	OutputDebugStringA( ( text + "\n" ).c_str() );
}

#ifdef _DEBUG
#define LOG_DEBUG(text)     wa_log( wa::strings::convert::to_string( text ) );
#define LOG_INFO(text)      wa_log( wa::strings::convert::to_string( text ) );
#define LOG_WARN(text)      wa_log( wa::strings::convert::to_string( text ) );
#else
#define LOG_DEBUG(text)     /* nop */
#define LOG_INFO(text)      /* nop */
#define LOG_WARN(text)      /* nop */
#endif // _DEBUG
#define LOG_ERROR(text)     wa_log( wa::strings::convert::to_string( text ) );
#define LOG_CRITICAL(text)  wa_log( wa::strings::convert::to_string( text ) );


inline wchar_t     _log_message_w[ DEFAULT_STR_BUFFER_SIZE ];
inline char        _log_message_a[ DEFAULT_STR_BUFFER_SIZE ];
inline std::string _log_message;

#endif // !_WA_LOGGER_HEADER
