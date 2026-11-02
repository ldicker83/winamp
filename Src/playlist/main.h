#ifndef NULLSOFT_PLAYLIST_MAIN_H
#define NULLSOFT_PLAYLIST_MAIN_H

#include <windows.h>
#include <shlwapi.h>

extern int (*warand)(void);
HRESULT ResolveShortCut(HWND hwnd, LPCWSTR pszShortcutFile, LPWSTR pszPath);
bool IsUrl(const wchar_t *url);

#include "../Agave/Component/ifc_wa5component.h"

class PlaylistComponent : public ifc_wa5component
{
public:
	void RegisterServices(api_service *service);
	int RegisterServicesSafeModeOk();
	void DeregisterServices(api_service *service);
protected:
	RECVS_DISPATCH;
};
extern PlaylistComponent playlistComponent;
#endif