#include <shlobj.h>
#include <strsafe.h>

#include "main.h"
#include "api__playlist.h"
#include "../nu/ns_wc.h"

HRESULT ResolveShortCut(HWND hwnd, LPCWSTR pszShortcutFile, LPWSTR pszPath)
{
	IShellLinkW      *psl = 0;
	WIN32_FIND_DATAW  wfd = {0};

	*pszPath = 0;   // assume failure

	HRESULT hres = CoCreateInstance( CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void **)&psl );
	if ( SUCCEEDED( hres ) )
	{
		IPersistFile *ppf = 0;
		hres = psl->QueryInterface( &ppf ); // OLE 2!  Yay! --YO
		if ( SUCCEEDED( hres ) )
		{
			hres = ppf->Load( pszShortcutFile, STGM_READ );
			if ( SUCCEEDED( hres ) )
			{
				hres = psl->Resolve( hwnd, SLR_ANY_MATCH );
				if ( SUCCEEDED( hres ) )
				{
					wchar_t szGotPath[ MAX_PATH ] = { 0 };
					StringCchCopyW( szGotPath, MAX_PATH, pszShortcutFile );
					hres = psl->GetPath( szGotPath, MAX_PATH, &wfd, SLGP_SHORTPATH );
					StringCchCopyW( pszPath, MAX_PATH, szGotPath );
				}
			}

			ppf->Release();
		}

		psl->Release();
	}

	return SUCCEEDED(hres);
}

bool IsUrl(const wchar_t *url)
{
	return !!wcsstr(url, L"://");
}

