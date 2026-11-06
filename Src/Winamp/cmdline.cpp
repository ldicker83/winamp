/** (c) Nullsoft, Inc.         C O N F I D E N T I A L
** Filename: 
** Project:
** Description:
** Author:
** Created:
**/

#include "main.h"
#include "../nu/FilePath.h"
#include "FileOperations.h"
#include "resource.h"
#include "strutil.h"
#include "../nu/AutoChar.h"
#include "../nu/AutoWide.h"
#include "../nu/ns_wc.h"
#include "api.h"
#include "main.hpp"

static HWND find_otherwinamp_fast()
{
	wchar_t buf[MAX_PATH] = {0};
	StringCchPrintfW(buf, MAX_PATH, L"%s_%x_CLASS", szAppName, APP_VERSION_NUM);
	HANDLE waitEvent = OpenEventW(EVENT_ALL_ACCESS, FALSE, buf);
	if (waitEvent)
	{
		HWND lhwnd = 0;
		CloseHandle(waitEvent);
		while (NULL != (lhwnd = FindWindowExW(NULL, lhwnd, szAppName, NULL)))
		{
			if (lhwnd != hMainWindow)
				return lhwnd;
		}
	}
	return NULL;
}

wchar_t *EatSpaces(wchar_t *cmdLine)
{
	while (cmdLine && *cmdLine && *cmdLine == L' ')
		cmdLine = CharNextW(cmdLine);

	return cmdLine;
}

static const wchar_t *EatSpaces(const wchar_t *cmdLine)
{
	while (cmdLine && *cmdLine && *cmdLine == L' ')
		cmdLine = CharNextW(cmdLine);

	return cmdLine;
}

wchar_t *FindNextCommand(wchar_t *cmdLine)
{
	int inQuotes = 0;
	while (cmdLine && *cmdLine)
	{
		if (*cmdLine == L' ' && !inQuotes) // if we see a space (and we're not in quotes) then we're done
		{
			// we purposefully don't eat any extra space characters here
			// that way we can null terminate the results of this function and get a clean string
			break;
		}
		else if (*cmdLine == L'\"') // check for quotes
		{
			inQuotes = !inQuotes; // toggles quotes mode
		}
		cmdLine = CharNextW(cmdLine); // iterate the string
	}
	return cmdLine;
}

void GetParameter(const wchar_t *commandLine, wchar_t *yourBuffer, size_t yourBufferSize)
{
	int inQuotes = 0;

	commandLine = EatSpaces(commandLine);

	for(;;)
	{
		if (yourBufferSize == 1  // buffer is out
			|| *commandLine == 0 // out of stuff to copy
			|| (*commandLine == L' ' && !inQuotes)) // or we found a space
		{
			*yourBuffer = 0;
			break;
		}
		else if (*commandLine == L'\"') // don't copy quotes
		{
			inQuotes = !inQuotes; // but do toggle the quote flag (so we can ignore spaces)
		}
		else // safe character to copy
		{
			*yourBuffer++ = *commandLine;
			yourBufferSize--;
		}
		commandLine++;
	}
}

bool IsCommand(const wchar_t *cmdline, const wchar_t *str, size_t size)
{
	if (!_wcsnicmp(cmdline, str, size) && (!cmdline[size] || cmdline[size] == L' '))
		return true;
	else
		return false;
}

void* LoadResource(HINSTANCE hinst, HINSTANCE owner, LPCTSTR lpType, LPCTSTR lpName, DWORD* size)
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

static LRESULT WINAPI cmdLineProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			HICON hIcon = LoadIconW(hMainInstance, MAKEINTRESOURCE(ICON_XP));
			SetClassLongPtrW(hwndDlg, GCLP_HICON, (LONG_PTR)hIcon);

			char *b = NULL, *p = 0, *op = 0;
			DWORD size = 0;
			HGLOBAL hResource = LoadResource(hMainInstance, hMainInstance, TEXT("TEXT"), TEXT("CMDLINE"), &size);
			p = (char*)hResource;
			if (p && (op = strstr(p, "!!End")))  // if there's "!!End" in the resource, than copy everything before it
			{
				b = (char*)GlobalAlloc(GPTR, op-p+1);
				memcpy(b, p, op-p);
				b[op-p] = 0;
			} else {
				b = (char*)GlobalAlloc(GPTR, size+1);
				if (b && p)
				{
					memcpy(b, p, size);
					b[size] = 0;
				}
			}

			SetDlgItemTextA(hwndDlg, IDC_CMDLINE, (b ? b : p)); // send it to the text control to display
			if (b) GlobalFree(b);
			SetFocus(GetDlgItem(hwndDlg, IDC_CMDLINE));
		}
		return FALSE;

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDCANCEL:
				case IDOK:
					DestroyWindow(hwndDlg);
				return FALSE;
			}
		}
		break;
	}

	return 0;
}

// Launchers that quote every argument hand us "/ADD" rather than /ADD, which the checks below
// would treat as a filename. Rewrite a quoted switch in place so they see it: "/ADD" becomes
// /ADD, and "/CONFIG=C:\a b.ini" becomes /CONFIG="C:\a b.ini", which GetParameter already reads.
// The text only ever gets shorter or stays the same length.
static void UnquoteSwitch(wchar_t *p)
{
	if (!p || p[0] != L'\"' || p[1] != L'/')
		return;

	wchar_t *close = wcschr(p + 1, L'\"');
	if (!close)
		return;

	wchar_t *equals = wcschr(p + 1, L'=');
	if (equals && equals < close)
	{
		// slide "/NAME=" left over the opening quote and reopen the quote after the '='
		wmemmove(p, p + 1, equals - p);
		*equals = L'\"';
	}
	else
	{
		// drop both quotes
		const size_t inside = close - (p + 1);
		wmemmove(p, p + 1, inside);
		wmemmove(p + inside, close + 1, wcslen(close + 1) + 1);
	}
}

wchar_t *ParseParameters(wchar_t *lpszCmdParam, int *bAdd, int *bBookmark, int *bHandle, int *nCmdShow, int *bCommand, int *bCmdParam, int *bAllowCompat)
{
	for (;;)
	{
		lpszCmdParam = EatSpaces(lpszCmdParam);
		UnquoteSwitch(lpszCmdParam);
		if (IsCommand(lpszCmdParam, L"-embedding", 10))
		{
			lpszCmdParam = SkipXW(lpszCmdParam, 10);
		}
		else if (IsCommand(lpszCmdParam, L"/ALLOW_COMPAT_MODE", 18))
		{
			lpszCmdParam = SkipXW(lpszCmdParam, 18);
			*bAllowCompat = 1;
		}
		else if (!_wcsnicmp(lpszCmdParam, L"/SAFE=", 6))
		{
			wchar_t p[1024] = {0};
			lpszCmdParam = SkipXW(lpszCmdParam, 6);
			GetParameter(lpszCmdParam, p, 1024);
			int mode = _wtoi(p);

			g_safeMode = (1 + (mode == 2));
		}
		else if (!_wcsnicmp(lpszCmdParam, L"/SAFEALWAYS", 11))
		{
			lpszCmdParam = SkipXW(lpszCmdParam, 11);
			g_safeMode = 3;
		}
		else if (IsCommand(lpszCmdParam, L"/?", 2))
		{
			DialogBoxW(hMainInstance, MAKEINTRESOURCEW(IDD_CMDLINE), 0, (DLGPROC)cmdLineProc);
			ExitProcess(0);
		}
		else if (IsCommand(lpszCmdParam, L"/NEW", 4))
		{
			lpszCmdParam = SkipXW(lpszCmdParam, 4);
			bNoHwndOther = 1;
		}
		else if (IsCommand(lpszCmdParam, L"/HANDLE", 7))
		{
			lpszCmdParam = SkipXW(lpszCmdParam, 7);
			*bHandle = 1;
		}
		else if (!_wcsnicmp(lpszCmdParam, L"/ADD", 4) && (!lpszCmdParam[4] || lpszCmdParam[4] == L' '))
		{
			lpszCmdParam = SkipXW(lpszCmdParam, 4);
			*bAdd = 1;
		}
		else if (!_wcsnicmp(lpszCmdParam, L"/BOOKMARK", 9) && (!lpszCmdParam[9] || lpszCmdParam[9] == L' '))
		{
			lpszCmdParam = SkipXW(lpszCmdParam, 9);
			*bBookmark = 1;
		}
		else if (!_wcsnicmp(lpszCmdParam, L"/CONFIG=", 8))
		{
			wchar_t p[1024] = {0};
			lpszCmdParam = SkipXW(lpszCmdParam, 8);
			GetParameter(lpszCmdParam, p, 1024);
			config_setinifile(p);
		}
		else if (!_wcsnicmp(lpszCmdParam, L"/INIDIR=", 8))
		{
			wchar_t p[1024] = {0};
			lpszCmdParam = SkipXW(lpszCmdParam, 8);
			GetParameter(lpszCmdParam, p, 1024);
			config_setinidir(p);
		}
		else if (!_wcsnicmp(lpszCmdParam, L"/M3UDIR=", 8))
		{
			wchar_t p[1024] = {0};
			lpszCmdParam = SkipXW(lpszCmdParam, 8);
			GetParameter(lpszCmdParam, p, 1024);
			config_setm3udir(p);
		}
		else if (!_wcsnicmp(lpszCmdParam, L"/CLASS=", 7))
		{
			wchar_t p[1024] = {0};
			lpszCmdParam = SkipXW(lpszCmdParam, 7);
			GetParameter(lpszCmdParam, p, 1024);
			StringCchCopyW(szAppName, 64, p);
		}
		else if (IsCommand(lpszCmdParam, L"/DELM3U", 7))
		{
			setup_config();
			FilePath::Remove(M3U_FILE);
			FilePath::Remove(OLD_M3U_FILE);
			ExitProcess(0);
		}
		else if (IsCommand(lpszCmdParam, L"/QUIT", 5) ||
			     IsCommand(lpszCmdParam, L"/EXIT", 5) ||
				 IsCommand(lpszCmdParam, L"/CLOSE", 6))
		{
			lpszCmdParam = SkipXW(lpszCmdParam, 5 + IsCommand(lpszCmdParam, L"/CLOSE", 6));
			HWND hwnd_other_winamp = find_otherwinamp_fast();
			if (IsWindow(hwnd_other_winamp))
				PostMessageW(hwnd_other_winamp, WM_CLOSE, 0, 0);
			ExitProcess(0);
		}
		else if (IsCommand(lpszCmdParam, L"/KILL", 5))
		{
			HWND hwnd_other_winamp;
			DWORD other_winamp_procId = 0;
			lpszCmdParam = SkipXW(lpszCmdParam, 5);
			hwnd_other_winamp = find_otherwinamp_fast();
			if (hwnd_other_winamp)
			{
				PostMessageW(hwnd_other_winamp, WM_CLOSE, 0, 0);

				GetWindowThreadProcessId(hwnd_other_winamp, &other_winamp_procId); // get the process ID
				if (other_winamp_procId) // if we didn't get one, it probably already handled the WM_CLOSE message ...
				{
					HANDLE other_winamp_process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE | SYNCHRONIZE, FALSE, other_winamp_procId);
					if (other_winamp_process) // if we got a process handle (it might have quit already in the meantime ...)
					{
						if (WaitForSingleObject(other_winamp_process, 3000) == WAIT_TIMEOUT) // wait 5 seconds for it to close
						{
							DWORD exitCode = 0;
							TerminateProcess(other_winamp_process, exitCode); // terminate if we timed out
							WaitForSingleObject(other_winamp_process, 1000); // wait some more because TerminateProcess() returns immediately
						}
						CloseHandle(other_winamp_process); // release our reference to the handle
					}
				}
			}
			ExitProcess(0);
		}
		/*else if (!_wcsnicmp(lpszCmdParam, L"/RETURN=", 8))
		{
			wchar_t p[40] = {0};
			lpszCmdParam = SkipXW(lpszCmdParam, 8);
			GetParameter(lpszCmdParam, p, 40);
			ExitProcess(_wtoi(p));
		}*/
		/*else if (!_wcsnicmp(lpszCmdParam, L"/WATCHER", 8))
		{
			wchar_t p[2048] = {0};
			lpszCmdParam = SkipXW(lpszCmdParam, 9);
			GetParameter(lpszCmdParam, p, 2048);
			// eat parameter for now...
			ExitProcess(0); // and do not do anything...
		}*/
		else if (IsCommand(lpszCmdParam, L"/STARTMIN", 9))
		{
			lpszCmdParam = SkipXW(lpszCmdParam, 10);
			*nCmdShow = SW_MINIMIZE;
		}
		else if (IsCommand(lpszCmdParam, L"/STARTMAX", 9))
		{
			lpszCmdParam = SkipXW(lpszCmdParam, 10);
			*nCmdShow = SW_RESTORE;
		}
		else if (IsCommand(lpszCmdParam, L"/PREV", 5))
		{
			lpszCmdParam = SkipXW(lpszCmdParam, 6);
			*bCommand = MAKEWPARAM(WINAMP_BUTTON1, 0);
		}
		else if (IsCommand(lpszCmdParam, L"/PLAY", 5))
		{
			lpszCmdParam = SkipXW(lpszCmdParam, 6);
			*bCommand = MAKEWPARAM(WINAMP_BUTTON2, 0);
		}
		else if (IsCommand(lpszCmdParam, L"/PAUSE", 6))
		{
			lpszCmdParam = SkipXW(lpszCmdParam, 7);
			*bCommand = MAKEWPARAM(WINAMP_BUTTON3, 0);
		}
		else if (IsCommand(lpszCmdParam, L"/STOP", 5))
		{
			lpszCmdParam = SkipXW(lpszCmdParam, 6);
			*bCommand = MAKEWPARAM(WINAMP_BUTTON4, 0);
		}
		else if (IsCommand(lpszCmdParam, L"/STOPFADE", 9))
		{
			lpszCmdParam = SkipXW(lpszCmdParam, 10);
			*bCommand = MAKEWPARAM(WINAMP_BUTTON4_SHIFT, 0);
		}
		else if (IsCommand(lpszCmdParam, L"/STOPAFTER", 10))
		{
			lpszCmdParam = SkipXW(lpszCmdParam, 11);
			*bCommand = MAKEWPARAM(WINAMP_BUTTON4_CTRL, 0);
		}
		else if (IsCommand(lpszCmdParam, L"/NEXT", 5))
		{
			lpszCmdParam = SkipXW(lpszCmdParam, 6);
			*bCommand = MAKEWPARAM(WINAMP_BUTTON5, 0);
		}
		else if (IsCommand(lpszCmdParam, L"/PLAYPAUSE", 10))
		{
			lpszCmdParam = SkipXW(lpszCmdParam, 10);
			*bCommand = MAKEWPARAM(IPC_ISPLAYING, 2);
		}
		else if (IsCommand(lpszCmdParam, L"/FWD", 4))
		{
			lpszCmdParam = SkipXW(lpszCmdParam, 4);
			*bCommand = MAKEWPARAM(WINAMP_BUTTON5_SHIFT, 0);
		}
		else if (IsCommand(lpszCmdParam, L"/REV", 4))
		{
			lpszCmdParam = SkipXW(lpszCmdParam, 4);
			*bCommand = MAKEWPARAM(WINAMP_BUTTON1_SHIFT, 0);
		}
		else if (!_wcsnicmp(lpszCmdParam, L"/RANDOM=", 8))
		{
			wchar_t p[2] = {0};
			lpszCmdParam = SkipXW(lpszCmdParam, 8);
			GetParameter(lpszCmdParam, p, 2);
			if (p[0] == '0') *bCmdParam = 0;
			else if (p[0] == '1') *bCmdParam = 1;
			*bCommand = MAKEWPARAM(IPC_SET_SHUFFLE, 1);
		}
		else if (IsCommand(lpszCmdParam, L"/RANDOM", 7))
		{
			lpszCmdParam = SkipXW(lpszCmdParam, 7);
			*bCommand = MAKEWPARAM(WINAMP_FILE_SHUFFLE, 0);
		}
		else if (!_wcsnicmp(lpszCmdParam, L"/SHUFFLE=", 9))
		{
			wchar_t p[2] = {0};
			lpszCmdParam = SkipXW(lpszCmdParam, 9);
			GetParameter(lpszCmdParam, p, 2);
			if (p[0] == '0') *bCmdParam = 0;
			else if (p[0] == '1') *bCmdParam = 1;
			else break;
			*bCommand = MAKEWPARAM(IPC_SET_SHUFFLE, 1);
		}
		else if (IsCommand(lpszCmdParam, L"/SHUFFLE", 8))
		{
			lpszCmdParam = SkipXW(lpszCmdParam, 9);
			*bCommand = MAKEWPARAM(WINAMP_FILE_SHUFFLE, 0);
		}
		else if (!_wcsnicmp(lpszCmdParam, L"/REPEAT=", 8))
		{
			wchar_t p[2] = {0};
			lpszCmdParam = SkipXW(lpszCmdParam, 8);
			GetParameter(lpszCmdParam, p, 2);
			if (p[0] == '0') *bCmdParam = 0;
			else if (p[0] == '1') *bCmdParam = 1;
			else break;
			*bCommand = MAKEWPARAM(IPC_SET_REPEAT, 1);
		}
		else if (IsCommand(lpszCmdParam, L"/REPEAT", 7))
		{
			lpszCmdParam = SkipXW(lpszCmdParam, 7);
			*bCommand = MAKEWPARAM(WINAMP_FILE_REPEAT, 0);
		}
		else if (IsCommand(lpszCmdParam, L"/PANLEFT", 8))
		{
			lpszCmdParam = SkipXW(lpszCmdParam, 8);
			*bCommand = MAKEWPARAM(EQ_PANLEFT, 2);
		}
		else if (IsCommand(lpszCmdParam, L"/PANRIGHT", 9))
		{
			lpszCmdParam = SkipXW(lpszCmdParam, 9);
			*bCommand = MAKEWPARAM(EQ_PANRIGHT, 2);
		}
		else if (!_wcsnicmp(lpszCmdParam, L"/PAN=", 5))
		{
			wchar_t p[5] = {0};
			lpszCmdParam = SkipXW(lpszCmdParam, 5);
			GetParameter(lpszCmdParam, p, 5);
			if (p[0])
			{
				*bCmdParam = _wtoi(p);
				*bCommand = MAKEWPARAM(IPC_SETPANNING, 2);
			}
			else
				ExitProcess(0);
		}
		else if (IsCommand(lpszCmdParam, L"/VOLUP", 6))
		{
			lpszCmdParam = SkipXW(lpszCmdParam, 6);
			*bCommand = MAKEWPARAM(WINAMP_VOLUMEUP, 0);
		}
		else if (IsCommand(lpszCmdParam, L"/VOLDOWN", 8))
		{
			lpszCmdParam = SkipXW(lpszCmdParam, 8);
			*bCommand = MAKEWPARAM(WINAMP_VOLUMEDOWN, 0);
		}
		else if (!_wcsnicmp(lpszCmdParam, L"/VOL=", 5))
		{
			wchar_t p[4] = {0};
			lpszCmdParam = SkipXW(lpszCmdParam, 5);
			GetParameter(lpszCmdParam, p, 4);
			if (p[0])
			{
				int vol = _wtoi(p);
				if (vol < 0) vol = 0;
				if (vol > 100) vol = 100;
				*bCmdParam = ceil(vol * 2.55);
				*bCommand = MAKEWPARAM(IPC_SETVOLUME, 1);
			}
			else
				ExitProcess(0);
		}
		else if (IsCommand(lpszCmdParam, L"/JUMPTO", 7))
		{
			lpszCmdParam = SkipXW(lpszCmdParam, 8);
			*bCommand = MAKEWPARAM(WINAMP_JUMPFILE, 0);
		}
		else if (IsCommand(lpszCmdParam, L"/CLEAR", 6))
		{
			lpszCmdParam = SkipXW(lpszCmdParam, 7);
			*bCommand = MAKEWPARAM(IPC_DELETE, 1);
		}
		else if (!_wcsnicmp(lpszCmdParam, L"/COMMAND=", 9))
		{
			wchar_t p[16] = {0};
			lpszCmdParam = SkipXW(lpszCmdParam, 9);
			GetParameter(lpszCmdParam, p, 16);
			int id = _wtoi(p);
			if (id > 0 && id < 65536)
			{
				*bCommand = MAKEWPARAM(id, 0);
			}
			else
				ExitProcess(0);
		}
		else if (!_wcsnicmp(lpszCmdParam, L"/WA_IPC", 7) && (!lpszCmdParam[7] || lpszCmdParam[7] == L' '))
		{
			wchar_t p[16] = {0};
			lpszCmdParam = SkipXW(lpszCmdParam, 7);
			GetParameter(lpszCmdParam, p, 16);

			int id = _wtoi(p);
			if (id > 0 && id <= 65536)
			{
				*bCommand = MAKEWPARAM(id, 1);

				if (lpszCmdParam && *lpszCmdParam)
					lpszCmdParam = CharNextW(lpszCmdParam);
				while (lpszCmdParam && *lpszCmdParam)
				{
					if (*lpszCmdParam == L' ') break;
					lpszCmdParam = CharNextW(lpszCmdParam);
				}

				wchar_t p2[16] = {0};
				GetParameter(lpszCmdParam, p2, 16);
				id = _wtoi(p2);
				if (id >= 0 && id <= 65536)
				{
					*bCmdParam = id;

					if (lpszCmdParam && *lpszCmdParam)
						lpszCmdParam = CharNextW(lpszCmdParam);
					while (lpszCmdParam && *lpszCmdParam)
					{
						if (*lpszCmdParam == L' ') break;
						lpszCmdParam = CharNextW(lpszCmdParam);
					}
				}
				else
					ExitProcess(0);
			}
			else
				ExitProcess(0);
		}
		else if (*lpszCmdParam == L'/') // ignore /options :)
		{
			lpszCmdParam = SkipXW(lpszCmdParam, 1);
		}
		else
			break;

		lpszCmdParam = FindNextCommand(lpszCmdParam);
	}

	return lpszCmdParam;
}

void parseCmdLine(wchar_t *cmdline, HWND hwnd)
{
	wchar_t buf[MAX_PATH*4] = {0};
	wchar_t tmp[MAX_PATH] = {0};
	wchar_t *p;
	if (wcsstr(cmdline, L"/BOOKMARK") == cmdline)
	{
		wchar_t bookmark[1024] = {0};
		cmdline = SkipXW(cmdline, 9);
		GetParameter(cmdline, bookmark, 1024);

		COPYDATASTRUCT cds;
		cds.dwData = IPC_ADDBOOKMARKW;
		cds.lpData = (void *) bookmark;
		cds.cbData = sizeof(wchar_t)*(lstrlenW((wchar_t*) cds.lpData) + 1);
		SendMessageW(hwnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
		return ;
	}
	else if (wcsstr(cmdline, L"/HANDLE") == cmdline)
	{
		wchar_t uri[1024] = {0};
		cmdline = SkipXW(cmdline, 7);
		GetParameter(cmdline, uri, 1024);
		COPYDATASTRUCT cds;
		cds.dwData = IPC_HANDLE_URI;
		cds.lpData = (void *) uri;
		cds.cbData = sizeof(wchar_t)*(lstrlenW((wchar_t*) cds.lpData) + 1);
		SendMessageW(hwnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
		return ;
	}
	lstrcpynW(buf, cmdline, MAX_PATH*4);
	p = buf;

	wchar_t param[1024] = {0};
	while (p && *p)
	{
		p = EatSpaces(p);

		GetParameter(p, param, 1024);
		if (!hwnd)
		{
			PlayList_appendthing(param, 0, 0);
		}
		else
		{
			COPYDATASTRUCT cds;
			std::error_code error;
			if (!FilePath::IsUrl(param))
				StringCchCopyW(tmp, MAX_PATH, std::filesystem::absolute(param, error).c_str());
			if (!FilePath::IsUrl(param) && !error && tmp[0])
			{
				cds.dwData = IPC_PLAYFILEW;
				cds.lpData = (void *) tmp;
				cds.cbData = sizeof(wchar_t)*(lstrlenW((wchar_t *) cds.lpData) + 1);
				SendMessageW(hwnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
			}
			else
			{
				cds.dwData = IPC_PLAYFILEW;
				cds.lpData = (void *) param;
				cds.cbData = sizeof(wchar_t) * (lstrlenW((wchar_t *) cds.lpData) + 1);
				SendMessageW(hwnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
			}
		}
		p = FindNextCommand(p);
	}
} // parseCmdLine()

wchar_t *CheckSkin(wchar_t *lpszCmdParam, HWND hwnd_other, int *exit)
{ 
	wchar_t buf[32] = {0};
	*exit=0;
	lstrcpynW(buf, extensionW(lpszCmdParam), 32);
	if (wcsstr(buf, L"\"")) wcsstr(buf, L"\"")[0] = 0;

	// process .wsz/.wal file
	if (!_wcsicmp(buf, L"wsz") || !_wcsicmp(buf, L"wal"))
	{
		wchar_t *p = lpszCmdParam, buf[MAX_PATH] = {0}, buf2[MAX_PATH] = {0},
					 outname[MAX_PATH] = {0}, current[MAX_PATH] = {0};

		while (p &&*p == L' ') p++;
		if (p && *p == L'\"') { p++; if (wcsstr(p, L"\"")) wcsstr(p, L"\"")[0] = 0; }

		// this is roughly equivalent to PathUndecorate, which we can't use because it requires IE 5.0+
		StringCchCopyW(outname, MAX_PATH, FilePath::FindFileName(p));
		StringCchCopyW(buf, MAX_PATH, SKINDIR);
		FilePath::Combine(buf2, buf, outname);

		_r_sW("skin", current, MAX_PATH);

		bool name_match = !_wcsicmp(outname, current);
		bool file_match = !_wcsicmp(p, buf2);
		//if (_wcsicmp(outname, current))
		if (!name_match || name_match && !file_match)
		{
			// prompt if the user is ok to install (subject to user preferences)
			int ret = IDYES;
			if (config_skin_prompt)
			{
				ret = LPMessageBox(NULL, IDS_SKINS_INSTALL_PROMPT, IDS_SKINS_INSTALL_HEADER, MB_YESNO | MB_ICONQUESTION);
			}

			if (ret != IDYES)
			{
				*exit = 1;
				return lpszCmdParam;
			}
			else
			{
				if(*exit == -2)
				{
					*exit = -1;
				}
			}

			{
				wchar_t *tmp;
				tmp = outname + lstrlenW(outname);
				size_t tmpsize = MAX_PATH - (tmp - outname);
				while (tmp >= outname && *tmp != L'[') tmp--;
					if (tmp >= outname && tmp[1] && !_wcsicmp(tmp + 2, L"].wsz")) 
						StringCchCopyW(tmp, tmpsize, L".wsz");
					if (tmp >= outname && tmp[1] && !_wcsicmp(tmp + 2, L"].wal")) 
						StringCchCopyW(tmp, tmpsize, L".wal");
			}

			if (!InstallItem(p, buf, outname))
			{
				wchar_t buffer[MAX_PATH*3] = {0};
				StringCchPrintfW(buffer,sizeof(buffer),getStringW(IDS_SKINS_INSTALL_ERROR,NULL,0),outname,p,buf);
				MessageBoxW(NULL, buffer, getStringW(IDS_SKINS_INSTALL_HEADER,NULL,0), MB_OK | MB_ICONEXCLAMATION);
			}
		}

		if (hwnd_other)
		{
				_w_sW("skin", outname);
				COPYDATASTRUCT cds;
				cds.dwData = IPC_SETSKINW;
				cds.lpData = (void *) outname;
				cds.cbData = sizeof(wchar_t)*(lstrlenW(outname) + 1);
				SendMessageW(hwnd_other, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
				ShowWindow(hwnd_other, SW_RESTORE);
				SetForegroundWindow(hwnd_other);
			*exit=1;
		}
		else
		{
				g_skinloadedmanually = 1;
				_w_sW("skin", outname);
		}
		lpszCmdParam = L"";
	}

	return lpszCmdParam;
}