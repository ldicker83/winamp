/** (c) Nullsoft, Inc.         C O N F I D E N T I A L
 ** Filename: 
 ** Project:
 ** Description:
 ** Author:
 ** Created:
 **/

#include "main.h"
#include "api.h"
#include "../nu/AutoChar.h"


void DoInstall(int is_install)
{
	//2=audiotype
	//4=videotype
	//8=cd
	//16=set needreg=1
	//32=dircontextmenus
	//64=playlisttype


	config_setup_filetypes(0);
	config_registermediaplayer(1);
	
	if (is_install&32) config_adddircontext(0);
	else config_removedircontext(0);

	if (is_install&16) WritePrivateProfileStringW(L"WinampReg", L"NeedReg", L"1", INI_FILE);

	if (is_install&8)
	{
		if (!config_iscdplayer()) config_regcdplayer(1, 0);
	}
	else
	{
		if (config_iscdplayer()) config_regcdplayer(0, 0);
	}

	if (is_install&(4 | 2 | 64)) // audio or video or both
	{
		wchar_t ext_list[16384] = {0};

		if (is_install&(4 | 2))
		{
			wchar_t *ext = in_getextlistW();
			wchar_t *a = ext;

			if (ext)
			{
				while (a && *a)
				{
					wchar_t buf[1024] = {0}, buf2[64] = {0};
					StringCchPrintfW(buf, 1024, L"test.%s", a);

					buf2[0] = 0;
					in_get_extended_fileinfoW(buf, L"type", buf2, 32);
					int type = _wtoi(buf2);

					if (lstrlenW(ext_list) + lstrlenW(buf) > 16128) break;
					{
						int doreg = (!type && (is_install & 2)) || (type && (is_install & 4));

						if (doreg) config_register(a, doreg);
						if (doreg)
						{
							if (*ext_list) StringCchCatW(ext_list, 16384, L":");
							StringCchCatW(ext_list, 16384, a);
						}
					}

					a += lstrlenW(a) + 1;
				}
				GlobalFree((HGLOBAL)ext);
				SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST | SHCNF_FLUSHNOWAIT, NULL, NULL);
			}
		}

		if (is_install&64)
		{
			const wchar_t *p;
			size_t i=0;
			while (NULL != (p = playlistManager->EnumExtension(i++)))
			{
				int doreg = !!(is_install & 64);
				if (doreg)
				{
					config_register((wchar_t*)p, doreg);
					if (*ext_list) StringCchCatW(ext_list, 16384, L":");
					StringCchCatW(ext_list, 8192, p);
				}
				p += lstrlenW(p) + 1;
			}
		}
		_w_sW("config_extlist", ext_list);
	}

	//CreateEQPresets();

	config_register(L"wsz", 1);
	config_register(L"wal", 1);
	config_write(0);
	out_deinit();
	in_deinit();
	w5s_deinit();
	Wasabi_Unload();
	RemoveRegistrar();
	ExitProcess(0);
}