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
#include "Options.h"
#include "main.hpp"
#include <vector>
#include "../nu/AutoWide.h"

extern std::vector<In_Module*> in_modules;

static bool pluginsLoaded;
INT_PTR CALLBACK InputProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam) 
{
	hi helpinfo[]={
		{IDC_INPUTS,IDS_P_IN_INPUTS},
		{IDC_CONF,IDS_P_IN_CONF},
		{IDC_ABOUT,IDS_P_IN_ABOUT},
		{IDC_OPENPLUGINDIR_IN,IDS_P_IN_OPENDIR},
	};

	DO_HELP();

	if (uMsg == WM_INITDIALOG)
	{
		pluginsLoaded = false;

		link_startsubclass(hwndDlg, IDC_PLUGINVERS);

		HWND listWindow = GetDlgItem(hwndDlg, IDC_INPUTS);
		if (IsWindow(listWindow))
		{
			size_t x;
			RECT r = {0};
			GetWindowRect(listWindow, &r);
			GetClientRect(listWindow, &r);
			MapWindowPoints(listWindow, hwndDlg, (LPPOINT)&r, 2);
			InflateRect(&r, 2, 2);
			DestroyWindow(listWindow);
			listWindow = CreateWindowExW(WS_EX_NOPARENTNOTIFY | WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
										 WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL |
										 LVS_SHOWSELALWAYS | LVS_SORTASCENDING | LVS_NOCOLUMNHEADER | LVS_NOSORTHEADER,
										 r.left, r.top, r.right - r.left, r.bottom - r.top,
										 hwndDlg, (HMENU)IDC_INPUTS, NULL, NULL);
			SetWindowPos(listWindow, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
			ListView_SetExtendedListViewStyleEx(listWindow, LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP, LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);
			SendMessageW(listWindow, WM_SETFONT, SendMessageW(hwndDlg, WM_GETFONT, 0, 0), FALSE);

			LVCOLUMNW lvc = {0};
			ListView_InsertColumnW(listWindow, 0, &lvc);
			ListView_InsertColumnW(listWindow, 1, &lvc);

			for (x = 0; x < in_modules.size(); x ++)
			{
				// only try to add if the plugin is still there
				// (as 5.5+ allows for dynamic unloads if supported)
				if(in_modules[x])
				{
					wchar_t fn[MAX_PATH] = {0}, buf[512] = {0};
					GetModuleFileNameW(in_modules[x]->hDllInstance, fn, MAX_PATH);
					FilePath::StripPath(fn);

					int ver = ((in_modules[x]->version & ~IN_UNICODE) & ~IN_INIT_RET);
					LVITEMW lvi = {LVIF_TEXT | LVIF_PARAM, (int)x, 0};
					lstrcpynW(buf, (ver == IN_VER ? (wchar_t*)in_modules[x]->description : AutoWide(in_modules[x]->description)), 512);
					lvi.pszText = buf;
					lvi.lParam = x;
					lvi.iItem = ListView_InsertItemW(listWindow, &lvi);

					lvi.mask = LVIF_TEXT;
					lvi.iSubItem = 1;
					lvi.pszText = fn;
					ListView_SetItemW(listWindow, &lvi);
				}
			}

			wchar_t dirstr[MAX_PATH] = {0};
			for (const auto &entry : FilePath::List(PLUGINDIR, L"IN_*.DLL"))
			{
				std::wstring name = entry.path().filename().native();
				FilePath::Combine(dirstr, PLUGINDIR, name.c_str());
				HMODULE b = LoadLibraryExW(dirstr, NULL, LOAD_LIBRARY_AS_DATAFILE);
				for (x = 0; b && (x != in_modules.size()); x ++)
				{
					if (in_modules[x]->hDllInstance == b)
					{
						break;
					}
				}

				if (x == in_modules.size() || !b)
				{
					LVITEMW lvi = {LVIF_TEXT | LVIF_PARAM, (int)x, 0};
					lvi.pszText = name.data();
					lvi.lParam = -2;
					lvi.iItem = ListView_InsertItemW(listWindow, &lvi);

					lvi.mask = LVIF_TEXT;
					lvi.iSubItem = 1;
					lvi.pszText = getStringW(IDS_NOT_LOADED, NULL, 0);
					ListView_SetItemW(listWindow, &lvi);
				}
				FreeLibrary(b);
			}

			GetClientRect(listWindow, &r);
			ListView_SetColumnWidth(listWindow, 1, LVSCW_AUTOSIZE);
			ListView_SetColumnWidth(listWindow, 0, (r.right - r.left) - ListView_GetColumnWidth(listWindow, 1));

			DirectMouseWheel_EnableConvertToMouseWheel(listWindow, TRUE);

			pluginsLoaded = true;
		}
	}
	else if (uMsg == WM_DESTROY)
	{
		HWND listWindow = GetDlgItem(hwndDlg, IDC_INPUTS);
		if (IsWindow(listWindow))
			DirectMouseWheel_EnableConvertToMouseWheel(listWindow, FALSE);
	}
	else if (uMsg == WM_NOTIFY)
	{
		LPNMHDR p = (LPNMHDR)lParam;
		if (p->idFrom == IDC_INPUTS)
		{
			if (p->code == LVN_ITEMCHANGED)
			{
				LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
				LVITEM lvi = {LVIF_PARAM, pnmv->iItem};
				if (ListView_GetItem(p->hwndFrom, &lvi) && (pnmv->uNewState & LVIS_SELECTED))
				{
					int loaded = (lvi.lParam != -2);
					EnableWindow(GetDlgItem(hwndDlg, IDC_CONF), loaded);
					EnableWindow(GetDlgItem(hwndDlg, IDC_ABOUT), loaded);
				}
				else
				{
					EnableWindow(GetDlgItem(hwndDlg, IDC_CONF), 0);
					EnableWindow(GetDlgItem(hwndDlg, IDC_ABOUT), 0);
				}
			}
			else if (p->code == NM_DBLCLK)
			{
				PostMessageW(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_CONF, 0), (LPARAM)GetDlgItem(hwndDlg, IDC_CONF));
			}
		}
		else if (p->code == HDN_ITEMCHANGINGW)
		{
			if (pluginsLoaded)
			{
#ifdef WIN64
				SetWindowLongPtrW(hwndDlg, DWLP_MSGRESULT, TRUE);
#else
				SetWindowLong(hwndDlg, DWL_MSGRESULT, TRUE);
#endif
				return TRUE;
			}
		}
	}
	else if (uMsg == WM_COMMAND)
	{
		switch (LOWORD(wParam))
		{
			case IDC_ABOUT:
				{
					if (IsWindowEnabled(GetDlgItem(hwndDlg, IDC_ABOUT)))
					{
						HWND listWindow = GetDlgItem(hwndDlg, IDC_INPUTS);
						LVITEM lvi = {LVIF_PARAM, ListView_GetSelectionMark(listWindow)};
						if (ListView_GetItem(listWindow, &lvi))
						{
							if (lvi.lParam >= 0 && lvi.lParam < (int)in_modules.size()) in_modules[lvi.lParam]->About(hwndDlg);
						}
					}
				}
				break;

			case IDC_CONF:
				{
					if (IsWindowEnabled(GetDlgItem(hwndDlg, IDC_CONF)))
					{
						HWND listWindow = GetDlgItem(hwndDlg, IDC_INPUTS);
						LVITEM lvi = {LVIF_PARAM, ListView_GetSelectionMark(listWindow)};
						if (ListView_GetItem(listWindow, &lvi))
						{
							if (lvi.lParam >= 0 && lvi.lParam < (int)in_modules.size()) in_modules[lvi.lParam]->Config(hwndDlg);
						}
					}
				}
				break;

			case IDC_OPENPLUGINDIR_IN:
				OpenFolder(hwndDlg, PLUGINDIR);
				break;

		}
	}
	
	link_handledraw(hwndDlg,uMsg,wParam,lParam);
	return FALSE;
} //input