/** (c) Nullsoft, Inc.         C O N F I D E N T I A L
** Filename: 
** Project:
** Description: Utility functions for handling language support
** Author:
** Created:
**/

#include <locale.h>
#include "main.h"
#include "language.h"
#ifdef _DEBUG
static void CheckLangThread()
{
	if (mainThreadId != GetCurrentThreadId())
	{
//		DebugBreak();
		/**
		** If you hit this breakpoint, it's because you tried to use WASABI_API_LANG->GetString on another thread,
		** without supplying your own buffer.
		** You really don't want to be doing that.
		** Hit alt+7, check what function is calling GetString/GetStringW and go fix it.
		** Now.
		**/
	}
}

#else
#define CheckLangThread()
#endif

char *getString(UINT uID, char *str, size_t maxlen)
{
	return langManager->GetString(hMainInstance, hMainInstance, uID, str, maxlen);
}

int LPMessageBox(HWND parent, UINT idMessage, UINT idTitle, UINT type)
{
	wchar_t message[32768] = {0};
	wchar_t title[256] = {0};

	// TODO consider exposing something in the winamp.lng file so we can follow this where possible as it should allow for a localised messagebox as long as the OS supports the language
	// return MessageBoxExW(parent,getStringW(idMessage,message,32768),getStringW(idTitle,title,256),type,MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH));
	return MessageBoxW(parent,getStringW(idMessage,message,32768),getStringW(idTitle,title,256),type);
}

char* Language::GetString(HINSTANCE hinst, HINSTANCE owner, UINT uID, char *str, size_t maxlen)
{
	__declspec(thread) static char *buf;
	if (!str)
	{
		CheckLangThread();
		if (!buf)
			buf = (char *)calloc(LANG_STATIC_BUFFER_SIZE, sizeof(buf[0]));
		str = buf; 
		maxlen = LANG_STATIC_BUFFER_SIZE; 
	}

	// sometimes we need to ignore things i.e. accessing on closing, etc
	if (((unsigned long)str >= 65536) && !LoadStringA(hinst, uID, str, (int)maxlen))
	{
		if (hinst == owner || !LoadStringA(owner, uID, str, (int)maxlen))
		{
			lstrcpynA(str, "Error loading string", (int)maxlen);
		}
	}
	return str;
}

wchar_t *getStringW(UINT uID, wchar_t *str, size_t maxlen)
{
	return langManager->GetStringW(hMainInstance, hMainInstance, uID, str, maxlen);
}

wchar_t* Language::GetStringW(HINSTANCE hinst, HINSTANCE owner, UINT uID, wchar_t *str, size_t maxlen)
{
	__declspec(thread) static wchar_t *buf;
	if (!str) 
	{
		CheckLangThread();
		if (!buf) 	
			buf = (wchar_t *)calloc(LANG_STATIC_BUFFER_SIZE, sizeof(buf[0]));
		str = buf; 
		maxlen = LANG_STATIC_BUFFER_SIZE; 
	}

	// sometimes we need to ignore things i.e. accessing on closing, etc
	if (((unsigned long)str >= 65536) && !LoadStringW(hinst, uID, str, (int)maxlen))
	{
		if (hinst == owner || !LoadStringW(owner, uID, str, (int)maxlen))
		{
			lstrcpynW(str, L"Error loading string", (int)maxlen);
		}
	}
	return str;
}

char* Language::GetStringFromGUID(const GUID guid, HINSTANCE owner, UINT uID, char *str, size_t maxlen)
{
	__declspec(thread) static char *buf;
	if (!str) 
	{
		CheckLangThread();
		if (!buf) 	
			buf = (char *)calloc(LANG_STATIC_BUFFER_SIZE, sizeof(buf[0]));
		str = buf; 
		maxlen = LANG_STATIC_BUFFER_SIZE; 
	}

	// sometimes we need to ignore things i.e. accessing on closing, etc
	if (((unsigned long)str >= 65536) && !LoadStringA(owner, uID, str, (int)maxlen))
	{
		lstrcpynA(str, "Error loading string", (int)maxlen);
	}
	return str;
}

wchar_t* Language::GetStringFromGUIDW(const GUID guid, HINSTANCE owner, UINT uID, wchar_t *str, size_t maxlen)
{
	__declspec(thread) static wchar_t *buf;
	if (!str) 
	{
		CheckLangThread();
		if (!buf) 	
			buf = (wchar_t *)calloc(LANG_STATIC_BUFFER_SIZE, sizeof(buf[0]));
		str = buf; 
		maxlen = LANG_STATIC_BUFFER_SIZE; 
	}

	// sometimes we need to ignore things i.e. accessing on closing, etc
	if (((unsigned long)str >= 65536) && !LoadStringW(owner, uID, str, (int)maxlen))
	{
		lstrcpynW(str, L"Error loading string", (int) maxlen);
	}
	return str;
}

const wchar_t *Language::GetLanguageFolder()
{
	return L"";
}

void* Language::LoadResourceFromFileW(HINSTANCE hinst, HINSTANCE owner, LPCWSTR lpType, LPCWSTR lpName, DWORD* size)
{
	HINSTANCE hmod = hinst;
	HRSRC rsrc = FindResourceW(hmod, lpName, lpType);
	if(!rsrc)
	{
		hmod = owner;
		rsrc = FindResourceW(hmod, lpName, lpType);
	}
	if(rsrc)
	{
		HGLOBAL resourceHandle = LoadResource(hmod, rsrc);
		if(size){*size = SizeofResource(hmod, rsrc);}
		return LockResource(resourceHandle);
	}
	return 0;
}

void* Language::LoadResourceFromFile(HINSTANCE hinst, HINSTANCE owner, LPCTSTR lpType, LPCTSTR lpName, DWORD* size)
{
	HINSTANCE hmod = hinst;
	HRSRC rsrc = FindResource(hmod, lpName, lpType);
	if(!rsrc)
	{
		hmod = owner;
		rsrc = FindResource(hmod, lpName, lpType);
	}
	if(rsrc)
	{
		HGLOBAL resourceHandle = LoadResource(hmod, rsrc);
		if(size){*size = SizeofResource(hmod, rsrc);}
		return LockResource(resourceHandle);
	}
	return 0;
}

const wchar_t *Language::GetLanguageIdentifier( int mode )
{
	static wchar_t id_str[ 9 ] = { 0 };
	id_str[ 0 ] = 0;
	GetStringW( hMainInstance, hMainInstance, LANG_PACK_LANG_ID, id_str, 9 );

	if ( !_wcsicmp( id_str, L"Error l" ) )
	{
		id_str[ 0 ] = 0;
	}

	if ( mode && id_str[ 0 ] )
	{
		wchar_t *iStr = id_str;

		while ( iStr && *iStr && *iStr != L'-' )
		{
			iStr = CharNextW( iStr );
		}

		if ( iStr && *iStr == '-' )
		{
			iStr = CharNextW( iStr );
			*CharPrevW( id_str, iStr ) = 0;
		}

		if ( mode == LANG_LANG_CODE )
			return id_str;
		else if ( mode == LANG_COUNTRY_CODE )
			return iStr;
	}

	return ( id_str[ 0 ] ? id_str : 0 );
}

HWND Language::CreateLDialogParam( HINSTANCE localised, HINSTANCE original, UINT id, HWND parent, DLGPROC proc, LPARAM param )
{
	HWND hwnd = (HWND)CreateDialogParamA( localised, MAKEINTRESOURCEA( id ), parent, proc, param );
	if ( !hwnd && localised != original )
		hwnd = (HWND)CreateDialogParamA( original, MAKEINTRESOURCEA( id ), parent, proc, param );
	return hwnd;
}

HWND Language::CreateLDialogParamW( HINSTANCE localised, HINSTANCE original, UINT id, HWND parent, DLGPROC proc, LPARAM param )
{
	HWND hwnd = (HWND)CreateDialogParamW( localised, MAKEINTRESOURCEW( id ), parent, proc, param );
	if ( !hwnd && localised != original )
		hwnd = (HWND)CreateDialogParamW( original, MAKEINTRESOURCEW( id ), parent, proc, param );
	return hwnd;
}

INT_PTR Language::LDialogBoxParam( HINSTANCE localised, HINSTANCE original, UINT id, HWND parent, DLGPROC proc, LPARAM param )
{
	INT_PTR ret = DialogBoxParamA( localised, MAKEINTRESOURCEA( id ), parent, proc, param );
	if ( ( ret == -1 && GetLastError() != ERROR_SUCCESS ) && localised != original )
		ret = DialogBoxParamA( original, MAKEINTRESOURCEA( id ), parent, proc, param );
	return ret;
}

INT_PTR Language::LDialogBoxParamW( HINSTANCE localised, HINSTANCE original, UINT id, HWND parent, DLGPROC proc, LPARAM param )
{
	INT_PTR ret = DialogBoxParamW( localised, MAKEINTRESOURCEW( id ), parent, proc, param );
	if ( ( ret == -1 && GetLastError() != ERROR_SUCCESS ) && localised != original )
		ret = DialogBoxParamW( original, MAKEINTRESOURCEW( id ), parent, proc, param );
	return ret;
}

HWND LPCreateDialogParam( int id, HWND parent, DLGPROC proc, LPARAM param )
{
	return langManager->CreateLDialogParam(hMainInstance, hMainInstance, id, parent, proc, param );
}

HWND LPCreateDialogParamW( int id, HWND parent, DLGPROC proc, LPARAM param )
{
	return langManager->CreateLDialogParamW(hMainInstance, hMainInstance, id, parent, proc, param );
}

INT_PTR LPDialogBoxParam( int id, HWND parent, DLGPROC proc, LPARAM param )
{
	return langManager->LDialogBoxParam(hMainInstance, hMainInstance, id, parent, proc, param );
}

INT_PTR LPDialogBoxParamW( int id, HWND parent, DLGPROC proc, LPARAM param )
{
	return langManager->LDialogBoxParamW(hMainInstance, hMainInstance, id, parent, proc, param );
}

HMENU Language::LoadLMenu( HINSTANCE localised, HINSTANCE original, UINT id )
{
	HMENU menu = LoadMenuA( localised, MAKEINTRESOURCEA( id ) );
	if ( !menu && localised != original )
		menu = LoadMenuA( original, MAKEINTRESOURCEA( id ) );
	
	return menu;
}

HMENU Language::LoadLMenuW(HINSTANCE localised, HINSTANCE original, UINT id)
{
	HMENU menu = LoadMenuW(localised, MAKEINTRESOURCEW(id));
	if (!menu && localised != original)
		menu = LoadMenuW(original, MAKEINTRESOURCEW(id));
	return menu;
}

HACCEL Language::LoadAcceleratorsA(HINSTANCE hinst, HINSTANCE owner, LPCSTR lpTableName)
{
	HACCEL hAccel = ::LoadAcceleratorsA(hinst, lpTableName);
	if (!hAccel && hinst != owner)
		hAccel = ::LoadAcceleratorsA(owner, lpTableName);
	return hAccel;
}

HACCEL Language::LoadAcceleratorsW(HINSTANCE hinst, HINSTANCE owner, LPCWSTR lpTableName)
{
	HACCEL hAccel = ::LoadAcceleratorsW(hinst, lpTableName);
	if (!hAccel && hinst != owner)
		hAccel = ::LoadAcceleratorsW(owner, lpTableName);
	return hAccel;
}

// Implemented in 5.58+
// when we're loading a language pack we really need to specify if we're
// going to require correct use of the user's locale setting so that the
// output of certain text ie '%+6.1f' uses the correct decimal separator
// ref: http://msdn.microsoft.com/en-us/library/aa246453%28VS.60%29.aspx
BOOL Language::UseUserNumericLocale(void)
{
	wchar_t tmp[4] = {0}, lang[4] = {0}, ctry[4] = {0};
	GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, lang, 4);
	GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SISO3166CTRYNAME, ctry, 4);

	// do this check to ensure that the we only change the locale
	// if the language pack and the user locale identifiers match
	if(!_wcsicmp(lang, GetLanguageIdentifier(LANG_LANG_CODE)) &&
	   !_wcsicmp(ctry, GetLanguageIdentifier(LANG_COUNTRY_CODE)) &&
	   GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SABBREVLANGNAME, tmp, 4))
	{
		_configthreadlocale(_ENABLE_PER_THREAD_LOCALE);
		// now we set the functions to use the user's numeric locale
		return !!_wsetlocale(LC_NUMERIC,tmp);
	}
	return FALSE;
}

_locale_t Language::Get_C_NumericLocale(void)
{
	__declspec(thread) static _locale_t C_locale;
	if(!C_locale) C_locale = _create_locale(LC_NUMERIC, "C");
	return C_locale;
}

// Implemented in 5.64+
wchar_t* Language::FormattedSizeString(wchar_t *pszDest, int cchDest, __int64 size)
{
	static const int geno = _r_i("geno", 1);
	if (!pszDest) return 0;
	size_t remaining = cchDest;
	DWORD part = 0;
	pszDest[0] = 0x00;

	if (size < 1024)
	{
		StringCchPrintfExW(pszDest, cchDest, NULL, &remaining, STRSAFE_IGNORE_NULLS,
						   L"%u %s", (DWORD)(size >> 10) + ((((DWORD)(size))&1023) ? 1: 0),
						   getStringW(IDS_BYTES, NULL, 0));
	}
	else if (size < 1048576)
	{
		part = ((((DWORD)(size))&1023)*100) >> 10;
		if (part > 0)
		{
			StringCchPrintfExW(pszDest, cchDest, NULL, &remaining, STRSAFE_IGNORE_NULLS, L"%u.%02u %s",
							   (DWORD)(size >> 10), part, getStringW(geno ? IDS_KB : IDS_KIB, NULL, 0));
		}
		else
		{
			StringCchPrintfExW(pszDest, cchDest, NULL, &remaining, STRSAFE_IGNORE_NULLS, L"%u %s",
							   (DWORD)(size >> 10), getStringW(geno ? IDS_KB : IDS_KIB, NULL, 0));
		}
	}
	else if (size < 1073741824)
	{
		part = ((((DWORD)(size >> 10))&1023)*100) >> 10;
		if (part > 0)
		{
			StringCchPrintfExW(pszDest, cchDest, NULL, &remaining, STRSAFE_IGNORE_NULLS, L"%u.%02u %s",
							   (DWORD)(size >> 20), part, getStringW(geno ? IDS_MB : IDS_MIB, NULL, 0));
		}
		else
		{
			StringCchPrintfExW(pszDest, cchDest, NULL, &remaining, STRSAFE_IGNORE_NULLS, L"%u %s",
							   (DWORD)(size >> 20), getStringW(geno ? IDS_MB : IDS_MIB, NULL, 0));
		}
	}
	else if (size < 1099511627776)
	{
		part = ((((DWORD)(size >> 20))&1023)*100) >> 10;
		if (part > 0)
		{
			StringCchPrintfExW(pszDest, cchDest, NULL, &remaining, STRSAFE_IGNORE_NULLS, L"%u.%02u %s",
							   (DWORD)(size >> 30), part, getStringW(geno ? IDS_GB : IDS_GIB, NULL, 0));
		}
		else
		{
			StringCchPrintfExW(pszDest, cchDest, NULL, &remaining, STRSAFE_IGNORE_NULLS, L"%u %s",
							   (DWORD)(size >> 30), getStringW(geno ? IDS_GB : IDS_GIB, NULL, 0));
		}
	}
	else
	{
		part = ((((DWORD)(size >> 30))&1023)*100) >> 10;
		if (part > 0)
		{
			StringCchPrintfExW(pszDest, cchDest, NULL, &remaining, STRSAFE_IGNORE_NULLS, L"%u.%02u %s",
							   (DWORD)(size >> 40), part, getStringW(geno ? IDS_TB : IDS_TIB, NULL, 0));
		}
		else
		{
			StringCchPrintfExW(pszDest, cchDest, NULL, &remaining, STRSAFE_IGNORE_NULLS, L"%u %s",
							   (DWORD)(size >> 40), getStringW(geno ? IDS_TB : IDS_TIB, NULL, 0));
		}
	}


	return pszDest;
}

HMENU LPLoadMenu(UINT id)
{
	return langManager->LoadLMenu(hMainInstance, hMainInstance, id);
}


HINSTANCE Language::FindDllHandleByGUID(const GUID guid)
{
	return NULL;
}


HINSTANCE Language::FindDllHandleByString(const char* _str)
{
	return NULL;
}


HINSTANCE Language::FindDllHandleByStringW(const wchar_t* _str)
{
	return NULL;
}


// plug-ins use their own resources now that there are no language packs to override them
HINSTANCE Language::StartLanguageSupport(HINSTANCE hinstance, const GUID guid)
{
	return hinstance;
}


#ifdef CBCLASS
#undef CBCLASS
#endif

#define CBCLASS Language
START_DISPATCH;
CB( API_LANGUAGE_GETSTRING,             GetString )
CB( API_LANGUAGE_GETSTRINGW,            GetStringW )
CB( API_LANGUAGE_GETSTRINGFROMGUID,     GetStringFromGUID )
CB( API_LANGUAGE_GETSTRINGFROMGUIDW,    GetStringFromGUIDW )
CB( API_LANGUAGE_GETHINSTANCEBYGUID,    FindDllHandleByGUID )
CB( API_LANGUAGE_GETHINSTANCEBYNAME,    FindDllHandleByString )
CB( API_LANGUAGE_GETHINSTANCEBYNAMEW,   FindDllHandleByStringW )
CB( API_LANGUAGE_STARTUP,               StartLanguageSupport )
CB( API_LANGUAGE_GETLANGUAGEFOLDER,     GetLanguageFolder )
CB( API_LANGUAGE_CREATELDIALOGPARAM,    CreateLDialogParam )
CB( API_LANGUAGE_LDIALOGBOXPARAM,       LDialogBoxParam )
CB( API_LANGUAGE_LOADLMENU,             LoadLMenu )
CB( API_LANGUAGE_CREATELDIALOGPARAMW,   CreateLDialogParamW )
CB( API_LANGUAGE_LDIALOGBOXPARAMW,      LDialogBoxParamW )
CB( API_LANGUAGE_LOADLMENUW,            LoadLMenuW )
CB( API_LANGUAGE_GETLANGUAGEIDENTIFIER, GetLanguageIdentifier )
CB( API_LANGUAGE_LOADRESOURCEFROMFILEA, LoadResourceFromFile )
CB( API_LANGUAGE_LOADRESOURCEFROMFILEW, LoadResourceFromFileW )
CB( API_LANGUAGE_LOADACCELERATORSA,     LoadAcceleratorsA )
CB( API_LANGUAGE_LOADACCELERATORSW,     LoadAcceleratorsW )
CB( API_LANGUAGE_USEUSERNUMERICLOCALE,  UseUserNumericLocale )
CB( API_LANGUAGE_GET_C_NUMERICLOCALE,   Get_C_NumericLocale )
CB( API_LANGUAGE_FORMATTEDSIZESTRING,   FormattedSizeString )
END_DISPATCH