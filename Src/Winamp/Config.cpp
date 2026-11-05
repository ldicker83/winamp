/** (c) Nullsoft, Inc.         C O N F I D E N T I A L
** Filename:
** Project:
** Description:
** Author:
** Created:
**/

#ifndef CONFIG_IMPL
#define CONFIG_IMPL
#endif
#include "Main.h"
#include "../nu/FilePath.h"
#include "config.h"
#include "WinampAttributes.h"
#include "../nu/AutoWide.h"
#include "../nu/AutoChar.h"
#include "../nu/ns_wc.h"
#include "../nu/AutoLock.h"
#include "../nu/AutoCharFn.h"
#include "main.hpp"

int _r_i(const std::string& name, int def)
{
	return GetPrivateProfileIntA(app_name, name.c_str(), def, INI_FILEA);
}

#define RI(x) (( config_##x ) = _r_i(#x,( config_##x )))
#define RI_NP(x) (( x ) = _r_i(#x,( x )))
#define RB(x) (( config_##x ) = !!_r_i(#x,( config_##x )))
void _w_i(const std::string& name, intptr_t d)
{
	char str[120] = {0};
	StringCchPrintfA(str, 120, "%d", d);
	WritePrivateProfileStringA(app_name, name.c_str(), str, INI_FILEA);
}
#define WI(x) _w_i(#x,( config_##x ))
#define WI_NP(x) _w_i(#x,( x ))

void _r_s(const std::string& name, char *data, int mlen)
{
	char utf8_data[2048] = {0};
	wchar_t utf16_data[2048] = {0};
	char buf[2048] = {0};
	StringCchCopyA(buf, 2048, data);
	GetPrivateProfileStringA(app_name, name.c_str(), buf, utf8_data, 2048, INI_FILEA);
	MultiByteToWideCharSZ(CP_UTF8, 0, utf8_data, -1, utf16_data, 2048);
	WideCharToMultiByteSZ(CP_ACP, 0, utf16_data, -1, data, mlen, 0, 0);
}
#define RS(x) (_r_s(#x,config_##x,sizeof(config_##x)))
#define RS_NP(x) (_r_s(#x,x,sizeof(x)))

void _r_sW(const std::string& name, wchar_t *data, int mlen)
{
	char utf8_data[2048] = {0};
	char default_data[2048] = {0};
	WideCharToMultiByteSZ(config_utf8?CP_UTF8:CP_ACP, 0, data, -1, default_data, 2048,0,0);
	GetPrivateProfileStringA(app_name, name.c_str(), default_data, utf8_data, 2048, INI_FILEA);
	MultiByteToWideCharSZ(config_utf8?CP_UTF8:CP_ACP, 0, utf8_data, -1, data, mlen);
}
#define RSW(x) (_r_sW(#x,config_##x,sizeof(config_##x)/sizeof(wchar_t)))
#define RSW_NP(x) (_r_sW(#x,x,sizeof(x)/sizeof(wchar_t)))

void _w_s(const std::string& name, const char *data)
{
	WritePrivateProfileStringA(app_name, name.c_str(), AutoChar(AutoWide(data), CP_UTF8), INI_FILEA);
}
#define WS(x) (_w_s(#x,config_##x))
#define WS_NP(x) (_w_s(#x,x))

void _w_sW(const std::string& name, const wchar_t *data)
{
	WritePrivateProfileStringA(app_name, name.c_str(), AutoChar(data, CP_UTF8), INI_FILEA); // TODO: don't want autowide here
}
#define WSW(x) (_w_sW(#x,config_##x))
#define WSW_NP(x) (_w_sW(#x,x))


void config_write(int i)
{
	config_pilp = PlayList_getPosition();
	//GetCurrentDirectoryW(MAX_PATH, config_cwd);
	if (i && plneedsave)
	{
		// changed 5.64 to only save if there was a change as this can save a
		// decent amount of time on closing with a large unmodified playlist.
		savem3ufn(M3U_FILE, 0, 1);
		savem3ufn(OLD_M3U_FILE, 0, 1);
	}

	config_utf8=1;
	WI(utf8);

	if (i != 1) // write mostly unused stuff, like proxy, plugin names, etc
	{
		WS(defext);
		WSW(titlefmt);
		WSW(visplugin_name);
		WSW(dspplugin_name);
		WI(visplugin_num);
		WI(pe_fontsize);
		WI(pe_direction);
		WI(visplugin_priority);
		WI(visplugin_autoexec);
		WI(dspplugin_num);
		WI(sticon);
		WI(splash);
		WI(taskbar);
		WI(dropaotfs);
		WI(nomwheel);
		WI(ascb_new);
		WI(ttips);
		WI(riol);
		WI(minst);
		WI(addtolist);
		WI(snap);
		WI(snaplen);
		WI(parent);
		WI(hilite);
		WI(disvis);
		WI(rofiob);
		WI(shownumsinpl);
		WI(keeponscreen);
		WI(eqdsize);
		WI(usecursors);
		WI(fixtitles);
		WI(priority);
		WI(shuffle_morph_rate);
		WI(useexttitles);
		WI(bifont);
		WI(bifont_alt);
		WI(dotitlenum);
		WI(dotasknum);
		WI(plscrollsize);
		WI(plmw2xscroll);
		WI(inet_mode);
		WI(ospb);
		WI(embedwnd_freesize);
		WI(no_visseh);
	}
	WI(newverchk);
	WI(newverchk2);
	WI(newverchk3);
	WI(newverchk_rc);
	WI(user_consent_join_channels);
	// write everything else
	{
		int config_prefs_last_page = prefs_last_page | (about_lastpage << 8);
		WI(prefs_last_page);
		prefs_last_page &= 255;

		_w_i("prefs_wx", prefs_rect.left);
		_w_i("prefs_wy", prefs_rect.top);

		_w_i("alt3_wx", alt3_rect.left);
		_w_i("alt3_wy", alt3_rect.top);

		_w_i("editinfo_wx", editinfo_rect.left);
		_w_i("editinfo_wy", editinfo_rect.top);

		_w_i("ctrle_wx", ctrle_rect.left);
		_w_i("ctrle_wy", ctrle_rect.top);

		_w_i("about_wx", about_rect.left);
		_w_i("about_wy", about_rect.top);

		_w_i("loc_wx", loc_rect.left);
		_w_i("loc_wy", loc_rect.top);

		_w_i("time_wx", time_rect.left);
		_w_i("time_wy", time_rect.top);

		_w_i("load_wx", load_rect.left);
		_w_i("load_wy", load_rect.top);
	}
	WI(autoload_eq);
	WI(use_eq);
	WI(eq_ws);
	WI(wx);
	WI(wy);
	WI(minimized);
	WI(aot);
	WI(shuffle);
	WI(repeat);
	WI(volume);
	WI(pan);
	WI(easymove);
	WI(dsize);
	WI(timeleftmode);
	WI(autoscrollname);
	WI(sa);
	WI(safire);
	WI(saref);
	WI(safalloff);
	WI(sa_peaks);
	WI(sa_peak_falloff);
	WI(eq_wx);
	WI(eq_wy);
	WI(eq_open);
	WI(mw_open);
	WI(pe_wx);
	WI(pe_wy);
	WI(pe_open);
	WI(pe_width);
	WI(pe_height);
	WI(pe_height_ws);

	WI(eq_limiter);
	WI(eq_type);
	WI(eq_frequencies);

	/*
	WI(si_wx);
	WI(si_wy);
	WI(si_width);
	WI(si_height);
	WI(si_autoshow);
	WI(si_autosize);
	WI(si_autohide);
	WI(si_open);
	*/

	WI(windowshade);
	WI(preamp);
	WI(pilp);
	WI(randskin);
	WSW(cwd);
	WSW(skin);
	WS(outname);
	WI(pladv);
	{
		char config_eq_data[256] = {0};
		StringCchPrintfA(config_eq_data, 256, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
			eq_tab[0], eq_tab[1], eq_tab[2], eq_tab[3], eq_tab[4],
			eq_tab[5], eq_tab[6], eq_tab[7], eq_tab[8], eq_tab[9]);
		WS(eq_data);
	}

	WI(playback_thread_priority);
	WI(audio_bits);
	WI(audio_mono);
	WI(audio_surround);
	WI(audio_dither);
	WI(replaygain);
	WI(replaygain_mode);
	WI(replaygain_source);
	WI(replaygain_preferred_only);
	_w_i("replaygain_non_rg_gain", (intptr_t)(config_replaygain_non_rg_gain.GetFloat()*1000.0f));
	_w_i("replaygain_preamp", (intptr_t)(config_replaygain_preamp.GetFloat()*1000.0f));
	WI(wav_do_header);
	WI(wav_convert);
	WS(wav_ext);
	_w_sW("playlist_custom_font", playlist_custom_fontW);
	WI(custom_plfont);
	WI(last_classic_skin_page);
	WI(last_playback_page);
	WI(last_fileinfo_page);
	WI(zeropadplnum);
	WI(accessibility_modalbeep);
	WI(accessibility_modalflash);

	WI(noml_ratings_prompt);
	WI(noml_ratings);
}

void config_read(int i)
{
	wchar_t programname[MAX_PATH] = {0};
	GetModuleFileNameW(hMainInstance, programname, MAX_PATH);

	RI(utf8);
	RI(eq_ws);
	RI(inet_mode);
	RI(pe_fontsize);
	RI(pe_direction);
	RI(ospb);
	RI(visplugin_num);
	RI(visplugin_priority);
	RI(visplugin_autoexec);
	RI(dspplugin_num);
	RI(sticon);
	RI(splash);
	RI(taskbar);
	RI(dropaotfs);
	RI(nomwheel);
	RI(ascb_new);
	RI(ttips);
	RI(keeponscreen);
	RI(riol);
	RI(addtolist);
	RI(snap);
	RI(snaplen);
	RI(parent);
	RI(hilite);
	RI(disvis);
	RI(minst);
	RI(eqdsize);
	RI(pladv);
	RI(bifont);
	RI(bifont_alt);
	RI(autoload_eq);
	RI(use_eq);
	RI(wx);
	RI(wy);
	RI(minimized);
	RI(aot);
	RI(shuffle);
	RI(repeat);
	RI(volume);
	RI(pan);
	RI(easymove);
	RI(dsize);
	RI(timeleftmode);
	RI(autoscrollname);
	RI(sa);
	RI(safire);
	RI(saref);
	RI(safalloff);
	RI(sa_peaks);
	RI(sa_peak_falloff);
	RI(eq_wx);
	RI(eq_wy);
	RI(eq_open);
	RI(mw_open);
	RI(pe_wx);
	RI(pe_wy);
	RI(pe_open);
	RI(pe_width);
	RI(pe_height);
	RI(pe_height_ws);

	RB(eq_limiter);
	RI(eq_type);
	RI(eq_frequencies);
	/*
	RI(si_wx);
	RI(si_wy);
	RI(si_height);
	RI(si_width);
	RI(si_autoshow);
	RI(si_autosize);
	RI(si_autohide);
	RI(si_open);
	*/

	RI(windowshade);
	RI(preamp);
	RI(pilp);
	RI(randskin);
	if (!*config_cwd) // don't read if the user has overridden it through paths.ini
		RSW(cwd);
	RSW(skin);

	RI_NP(prefs_last_page);
	about_lastpage = prefs_last_page >> 8;
	prefs_last_page &= 255;

	prefs_rect.left = _r_i("prefs_wx", -1);
	prefs_rect.top = _r_i("prefs_wy", -1);

	alt3_rect.left = _r_i("alt3_wx", -1);
	alt3_rect.top = _r_i("alt3_wy", -1);

	editinfo_rect.left = _r_i("editinfo_wx", -1);
	editinfo_rect.top = _r_i("editinfo_wy", -1);

	ctrle_rect.left = _r_i("ctrle_wx", -1);
	ctrle_rect.top = _r_i("ctrle_wy", -1);

	about_rect.left = _r_i("about_wx", -1);
	about_rect.top = _r_i("about_wy", -1);

	loc_rect.left = _r_i("loc_wx", -1);
	loc_rect.top = _r_i("loc_wy", -1);

	time_rect.left = _r_i("time_wx", -1);
	time_rect.top = _r_i("time_wy", -1);

	load_rect.left = _r_i("load_wx", -1);
	load_rect.top = _r_i("load_wy", -1);

	RI(rofiob);
	RI(shownumsinpl);
	RS(outname);
	RI(usecursors);
	RI(fixtitles);
	RI(priority);

	if (!_r_i("fixpriority", 0))
	{
		if (config_priority > 1)
		{
			config_priority++;
			WI(priority);
		}
		_w_i("fixpriority", 1);
	}

	RI(shuffle_morph_rate);
	RI(useexttitles);
	RI(newverchk);
	RI(newverchk2);
	RI(newverchk3);
	RI(newverchk_rc);
	RI(user_consent_join_channels);
	RI(embedwnd_freesize);
	RI(dotitlenum);
	RI(dotasknum);
	RI(plscrollsize);
	RI(plmw2xscroll);
	RI(playback_thread_priority);
	RI(audio_bits);
	RB(audio_mono);
	RB(audio_surround);
	RB(audio_dither);
	RB(replaygain);
	RI(replaygain_mode);
	RI(replaygain_source);
	RB(replaygain_preferred_only);
	int gain10 = _r_i("replaygain_non_rg_gain", (int)(config_replaygain_non_rg_gain.GetFloat()*1000.0f));	
	config_replaygain_non_rg_gain = (float)((float)gain10/1000.0f);
	gain10 = _r_i("replaygain_preamp", (int)(config_replaygain_preamp.GetFloat()*1000.0f));
	config_replaygain_preamp = (float)((float)gain10/1000.0f);

	RI(last_classic_skin_page);
	RI(last_playback_page);
	RI(last_fileinfo_page);
	RI(upd_mode);

	RI(noml_ratings_prompt);
	RI(noml_ratings);

	RI(jtf_check);
	RI(wav_do_header);
	RI(wav_convert);
	RS(wav_ext);
	RI(no_visseh);
	_r_sW("playlist_custom_font", playlist_custom_fontW, sizeof(playlist_custom_fontW)/sizeof(*playlist_custom_fontW));
	if (!*playlist_custom_fontW) StringCbCopyW(playlist_custom_fontW, sizeof(playlist_custom_fontW), DEFAULT_FONT);
	WideCharToMultiByteSZ(CP_ACP, 0, playlist_custom_fontW, -1, playlist_custom_font, 128, 0, 0);
	RI(custom_plfont);
	RS(defext);
	RSW(titlefmt);
	RSW(visplugin_name);
	RSW(dspplugin_name);
	RI(zeropadplnum);
	RB(accessibility_modalbeep);
	RB(accessibility_modalflash);

	{
		char eq_data[256] = {0, };
		int x;
		char *p = eq_data;
		RS_NP(eq_data);
		for (x = 0; x < 10; x ++)
		{
			int b = 0, s = 0;
			while (p && *p >= '0' && *p <= '9')
			{
				s = 1;b = b * 10 + *p++ -'0';
			}
			if (!s) break;
			p++;
			eq_tab[x] = min(63, max(b, 0));
		}
	}

	//SetCurrentDirectoryW(config_cwd);

	config_utf8=1;
	WI(utf8);


	if (!i)
	{
		if (!FilePath::Exists(M3U_FILE))
		{
			LoadPlaylistByExtension(OLD_M3U_FILE, L".m3u", 1, 1);
			savem3ufn(M3U_FILE, 0, 1);
		}
		else
			LoadPlaylistByExtension(M3U_FILE, L".m3u8", 1, 1);
		PlayList_setposition(config_pilp);
	}
}


void trimPathW(wchar_t *pathStart)
{
	FilePath::RemoveBlanks(pathStart);
	FilePath::RemoveBackslash(pathStart);
}

void config_setinifile(wchar_t *inifile)
{
	lstrcpynW(INI_FILE, inifile, sizeof(INI_FILE) / sizeof(INI_FILE[0]));
}

void config_setinidir(const wchar_t *inidir)
{
	if (SUCCEEDED(StringCchCopyW(CONFIGDIR, MAX_PATH, inidir)))
		trimPathW(CONFIGDIR);
	else
		CONFIGDIR[0] = 0;
}

void config_setm3udir(const wchar_t *m3udir)
{
	if (SUCCEEDED(StringCchCopyW(M3UDIR, MAX_PATH, m3udir)))
		trimPathW(M3UDIR);
	else
		M3UDIR[0] = 0;
}

void config_setm3ubase(const wchar_t *m3ubase)
{
	if (SUCCEEDED(StringCchCopyW(M3UBASE, MAX_PATH, m3ubase)))
		trimPathW(M3UBASE);
	else
		M3UBASE[0] = 0;
}

void init_config()
{
	GetModuleFileNameW(hMainInstance, PROGDIR, MAX_PATH);
	FilePath::RemoveFileSpec(PROGDIR);
	SetEnvironmentVariableW(L"WINAMP_PROGRAM_DIR", PROGDIR);
	wchar_t winamp_root[MAX_PATH] = {0};
	StringCchCopyW(winamp_root, MAX_PATH, PROGDIR);
	FilePath::StripToRoot(winamp_root);
	SetEnvironmentVariableW(L"WINAMP_ROOT_DIR", winamp_root);
}

void setup_config(void)
{
	if (!CONFIGDIR[0])
		StringCchCopyW(CONFIGDIR, MAX_PATH, PROGDIR);

	if (!M3UDIR[0])
		StringCchCopyW(M3UDIR, MAX_PATH, CONFIGDIR);

	if (!M3UBASE[0])
		StringCchCopyW(M3UBASE, MAX_PATH, M3UDIR);

	std::error_code error;
	std::filesystem::create_directory(M3UDIR, error);
	std::filesystem::create_directory(CONFIGDIR, error);

	// basic config files
	FilePath::Combine(OLD_M3U_FILE, M3UDIR, L"Winamp.m3u");
	FilePath::Combine(M3U_FILE, M3UDIR, L"Winamp.m3u8");
	FilePath::Combine(BOOKMARKFILE, CONFIGDIR, L"Winamp.bm");
	FilePath::Combine(BOOKMARKFILE8, CONFIGDIR, L"Winamp.bm8");
	// just make sure if a winamp.bm8 doesn't exist then
	// go make one from winamp.bm - implemented for 5.58+
	if(!FilePath::Exists(BOOKMARKFILE8))
	{
		FilePath::Copy(BOOKMARKFILE,BOOKMARKFILE8,true);
	}
	FilePath::Combine(EQDIR1, CONFIGDIR, L"Winamp.q1");
	FilePath::Combine(EQDIR2, CONFIGDIR, L"Winamp.q2");
	StringCchCopyW(TEMP_FILE, MAX_PATH, (FilePath::TempDirectory() / L"Winamp.tmp").c_str());
	FilePath::Combine(DEMOMP3, M3UDIR, L"demo.mp3");
	FilePath::Combine(ML_INI_FILE, CONFIGDIR, L"Plugins\\gen_ml.ini");

	// override INI_FILE if specified
	if (INI_FILE[0])
	{
		if (FilePath::IsFileSpec(INI_FILE) || FilePath::IsRelative(INI_FILE))
		{
			wchar_t temp[MAX_PATH] = {0};
			FilePath::Combine(temp, CONFIGDIR, INI_FILE);
			lstrcpynW(INI_FILE, temp, MAX_PATH);
		}
	}
	else
		FilePath::Combine(INI_FILE, CONFIGDIR, L"Winamp.ini");

	// maintain a ansi version of INI_FILE for some aspects
	StringCchCopyA(INI_FILEA, MAX_PATH, AutoCharFn(INI_FILE));

	RI(utf8);

	// skin and plugin directories
	FilePath::Combine(SKINDIR, PROGDIR, L"Skins");
	FilePath::Combine(PLUGINDIR, PROGDIR, L"Plugins");
	FilePath::Combine(SYSPLUGINDIR, PROGDIR, L"System");
	FilePath::Combine(SKINDIR, PROGDIR, L"Skins");
	FilePath::Combine(LANGDIR, PROGDIR, L"Lang");
	FilePath::Combine(VISDIR, PROGDIR, L"Plugins");
	FilePath::Combine(DSPDIR, PROGDIR, L"Plugins");

	// override skin/plugin directories from config
	{
		wchar_t bW[MAX_PATH] = {0}, *pW;

		bW[0]=0;
		_r_sW("PluginDir", bW, MAX_PATH);
		pW = bW;
		while (pW && (*pW == L' ' || *pW == L'\t')) pW++;
		if (pW && *pW)
		{
			StringCchCopyW(PLUGINDIR, MAX_PATH, bW);
		}

		ZeroMemory(bW, sizeof(bW));
		_r_sW("DSPDir", bW, MAX_PATH);
		pW = bW;
		while (pW && (*pW == L' ' || *pW == L'\t')) pW++;
		if (pW && *pW)
		{
			StringCchCopyW(DSPDIR, MAX_PATH, bW);
		}

		ZeroMemory(bW, sizeof(bW));
		_r_sW("VISDir", bW, MAX_PATH);
		pW = bW;
		while (pW && (*pW == L' ' || *pW == L'\t')) pW++;
		if (pW && *pW)
		{
			StringCchCopyW(VISDIR, MAX_PATH, bW);
		}

		ZeroMemory(bW, sizeof(bW));
		_r_sW("SkinDir", bW, MAX_PATH);
		pW = bW;
		while (pW && (*pW == L' ' || *pW == L'\t')) pW++;
		if (pW && *pW)
		{
			StringCchCopyW(SKINDIR, MAX_PATH, bW);
		}
	}

	// create a skin temp directory, too
	StringCchCopyW(SKINTEMPDIR, MAX_PATH, FilePath::UniqueName(FilePath::TempDirectory(), L"WAS").c_str());

	RI(minst);
}

