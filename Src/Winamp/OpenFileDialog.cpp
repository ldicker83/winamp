/** (c) Nullsoft, Inc.         C O N F I D E N T I A L
 ** Filename:
 ** Project:
 ** Description:
 ** Author:
 ** Created:
 **/

#include "main.h"
#include "../nu/FilePath.h"
#include "resource.h"
#include "../nu/FilePath.h"
#include "strutil.h"
#include "../nu/FilePath.h"
#include "../nu/AutoChar.h"
#include "../nu/FilePath.h"
#include "api.h"
#include "../nu/FilePath.h"
#include <shlwapi.h>
#include "../nu/FilePath.h"
#include <shobjidl.h>
#include "../nu/FilePath.h"

/*static wchar_t *inc(wchar_t *p, size_t &size, int extra=0)
{
	int len = lstrlenW(p) + extra;
	size-=len;
	p+=len;
	return p;
}*/

#if 0
extern std::vector<In_Module*> in_modules;
void getNewFileVistaPlus(int clearlist, HWND hwnd, const wchar_t *path)
{
    IFileOpenDialog *pFileOpen;
	static int qq;
	if (qq) return;
	qq = 1;
    HRESULT hr = CoCreateInstance(__uuidof(FileOpenDialog), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileOpen));
    if (SUCCEEDED(hr))
    {
		wchar_t titleStr[128] = {0};

		pFileOpen->SetTitle(getStringW((clearlist ? IDS_OFD_OPEN_FILES : IDS_OFD_ADD_FILES_TO_PLAYLIST),titleStr,128));
		pFileOpen->SetDefaultExtension(L"");
		pFileOpen->SetOptions(FOS_FILEMUSTEXIST | FOS_ALLOWMULTISELECT | FOS_NOREADONLYRETURN | FOS_PATHMUSTEXIST);
		if(!clearlist) pFileOpen->SetOkButtonLabel(L"Add");

		IShellItem* shellItem;
		hr = SHCreateItemFromParsingName((!path ? WASABI_API_APP->path_getWorkingPath() : path), 0, IID_IShellItem, reinterpret_cast<void**>(&shellItem));
		if (SUCCEEDED(hr))
		{
			pFileOpen->SetFolder(shellItem);
		}

		wchar_t *fsb = in_getfltstrW(TRUE);
		int filterNum = 0;
		// allocate enough for the number of input plug-ins plus 'all supported' and 'all files'
		COMDLG_FILTERSPEC *fileTypes = new COMDLG_FILTERSPEC[in_modules.size() + 2];
		while(fsb && *fsb && *fsb+1)
		{
			int len = lstrlenW(fsb) + 1;
			wchar_t *fsb2 = fsb + len;
            fileTypes[filterNum].pszName = fsb;
            fileTypes[filterNum].pszSpec = fsb2;
			CharUpperBuffW(fsb2, lstrlenW(fsb2));
			filterNum++;
			fsb += len + lstrlenW(fsb2) + 1;
		}

		pFileOpen->SetFileTypes(filterNum, fileTypes);

		if (hwnd == hMainWindow)
			UninitDragDrops();

        hr = pFileOpen->Show(hwnd);
        if (SUCCEEDED(hr))
        {
			IShellItemArray *pItems;
			hr = pFileOpen->GetResults(&pItems);
            if (SUCCEEDED(hr))
            {
				IEnumShellItems *sItems;
				hr = pItems->EnumItems(&sItems);
				if (SUCCEEDED(hr))
				{
					DWORD numItems = 0;
					pItems->GetCount(&numItems);

					int sp;
					if (clearlist)
					{
						sp = 0; PlayList_delete();
					}
					else sp = PlayList_getlength();

					if(numItems <= 1)
					{
						IShellItem *pItem;
						if(sItems->Next(1, &pItem, NULL) == S_OK)
						{
							LPWSTR folderpath = NULL;
							LPWSTR filepath = NULL;
							hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &filepath);
							if (SUCCEEDED(hr))
							{
								IShellItem *ppItem;
								hr = pItem->GetParent(&ppItem);
								if (SUCCEEDED(hr))
								{
									hr = ppItem->GetDisplayName(SIGDN_FILESYSPATH, &folderpath);
									if (SUCCEEDED(hr))
									{
										WASABI_API_APP->path_setWorkingPath(folderpath);
									}
								}

								if (LoadPlaylist(filepath, 1, 0) == -1) // if not a playlist file
								{
									PlayList_append(filepath, 0);
								}

								CoTaskMemFree(filepath);
								CoTaskMemFree(folderpath);
							}
						}
						pItem->Release();
					}
					else
					{
						size_t size = ((numItems + 1) * MAX_PATH);
						wchar_t *temp = (wchar_t *)GlobalAlloc(GPTR, size * sizeof(wchar_t)), *p = temp;
						IShellItem *pItem;
						while(sItems->Next(1, &pItem, NULL) == S_OK)
						{
							LPWSTR folderpath = NULL;
							LPWSTR filepath = NULL;
							hr = pItem->GetDisplayName(SIGDN_NORMALDISPLAY, &filepath);
							if (SUCCEEDED(hr))
							{
								// replicate how lpstrFile is filled under a multiple select
								// -> selected folder followed by each filename \0 separated
								if(p == temp)
								{
									IShellItem *ppItem;
									hr = pItem->GetParent(&ppItem);
									if (SUCCEEDED(hr))
									{
										hr = ppItem->GetDisplayName(SIGDN_FILESYSPATH, &folderpath);
										if (SUCCEEDED(hr))
										{
											WASABI_API_APP->path_setWorkingPath(folderpath);

											if(StringCchCatW(p, size, folderpath) == S_OK)
											{
												int len = lstrlenW(folderpath) + 1;
												p += len;
												size -= len;
											}
										}
									}
								}

								if(StringCchCatW(p, size, filepath) == S_OK)
								{
									int len = lstrlenW(filepath) + 1;
									p += len;
									size -= len;
								}

								CoTaskMemFree(filepath);
								CoTaskMemFree(folderpath);
							}
							pItem->Release();
						}

						PlayList_addfromdlg(temp);
						if (config_rofiob&1) PlayList_sort(2, sp);

						GlobalFree(temp);
					}

					if (clearlist)
						BeginPlayback();

					sItems->Release();
				}
				pItems->Release();
            }
        }
        pFileOpen->Release();
		GlobalFree(fsb);

		if (hwnd == hMainWindow)
			InitDragDrops();
		for(;;)
		{
			MSG msg;
			if (!PeekMessage(&msg, hMainWindow, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE))
				break;
		}
    }
	qq = 0;
}
#endif

void getNewFile(int clearlist, HWND hwnd, const wchar_t *path) // if clearlist is 1, playlist is cleared
{
	// TODO - Egg was having stability issues with this so keeping disabled until resolved
	// on Vista and up use the IFileOpenDialog interface as it
	// allows us to get around the MAX_PATH file filter issues
	/*if(IsVista())
	{
		getNewFileVistaPlus(clearlist, hwnd, path);
		return;
	}*/

	// otherwise revert to using the older pre-Vista methods
	const int len = 256 * 1024 - 128;

	wchar_t titleStr[128] = {0};

	std::error_code error;
	const std::filesystem::path oldCurPath = std::filesystem::current_path(error);
	wchar_t *temp, *fsb;
	OPENFILENAMEW l = {sizeof(OPENFILENAMEW),0};
	static int q;
	if (q) return;
	q = 1;
	temp = (wchar_t *)GlobalAlloc(GPTR, len * sizeof(wchar_t));
	l.lStructSize = sizeof(l);
	l.hwndOwner = hwnd;
	l.lpstrFilter = fsb = in_getfltstrW(FALSE);

	l.lpstrFile = temp;
	l.nMaxFile = len - 1;
	l.lpstrTitle = getStringW((clearlist ? IDS_OFD_OPEN_FILES : IDS_OFD_ADD_FILES_TO_PLAYLIST),titleStr,128);
	l.lpstrDefExt = L"";
	l.lpstrInitialDir = path;
	if (!l.lpstrInitialDir) l.lpstrInitialDir = WASABI_API_APP->path_getWorkingPath();

	l.Flags = OFN_HIDEREADONLY | OFN_EXPLORER | OFN_ALLOWMULTISELECT
	          | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
	if (hwnd == hMainWindow)
		UninitDragDrops();
	if (GetOpenFileNameW(&l))
	{
		WASABI_API_APP->path_setWorkingPath(std::filesystem::current_path(error).c_str());
		int sp;
		if (clearlist)
		{
			sp = 0; PlayList_delete();
		}
		else sp = PlayList_getlength();
		if (temp[lstrlenW(temp)+1])
		{
			PlayList_addfromdlg(temp);
			if (config_rofiob&1) PlayList_sort(2, sp);
		}
		else
		{
			if (LoadPlaylist(temp, 1, 0) == -1) // if not a playlist file
			{
				PlayList_append(temp, 0);
			}
		}
		if (clearlist)
			BeginPlayback();
	}
	std::filesystem::current_path(oldCurPath, error);

	if (hwnd == hMainWindow)
		InitDragDrops();
	for(;;)
	{
		MSG msg;
		if (!PeekMessage(&msg, hMainWindow, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE))
			break;
	}
	GlobalFree(fsb);
	GlobalFree(temp);
	q = 0;
}

static wchar_t loc[FILENAME_SIZE];
int ResizeComboBoxDropDown(HWND hwndDlg, UINT id, const char* str, int width){
	SIZE size = {0};
	HWND control = GetDlgItem(hwndDlg, id);
	HDC hdc = GetDC(control);
	// get and select parent dialog's font so that it'll calculate things correctly
	HFONT font = (HFONT)SendMessageW(hwndDlg,WM_GETFONT,0,0), oldfont = (HFONT)SelectObject(hdc,font);
	GetTextExtentPoint32A(hdc, str, lstrlenA(str)+1, &size);

	int ret = width;
	if(size.cx > width)
	{
		SendDlgItemMessage(hwndDlg, id, CB_SETDROPPEDWIDTH, size.cx, 0);
		ret = size.cx;
	}

	SelectObject(hdc, oldfont);
	ReleaseDC(control, hdc);
	return ret;
}

int ResizeComboBoxDropDownW(HWND hwndDlg, UINT id, const wchar_t *str, int width){
	SIZE size = {0};
	HWND control = GetDlgItem(hwndDlg, id);
	HDC hdc = GetDC(control);
	// get and select parent dialog's font so that it'll calculate things correctly
	HFONT font = (HFONT)SendMessageW(hwndDlg,WM_GETFONT,0,0), oldfont = (HFONT)SelectObject(hdc,font);
	GetTextExtentPoint32W(hdc, str, (int) wcslen(str)+1, &size);

	int ret = width;
	if(size.cx > width)
	{
		SendDlgItemMessage(hwndDlg, id, CB_SETDROPPEDWIDTH, size.cx, 0);
		ret = size.cx;
	}

	SelectObject(hdc, oldfont);
	ReleaseDC(control, hdc);
	return ret;
}

