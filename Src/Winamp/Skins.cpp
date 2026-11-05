/** (c) Nullsoft, Inc.         C O N F I D E N T I A L
** Filename: 
** Project:
** Description:
** Author:
** Created:
**/

#include "main.h"
#include "../nu/FilePath.h"
#include <fstream>
#include "../nu/AutoWide.h"
#include "../nu/AutoChar.h"
#include "../nu/ns_wc.h"
#include "minizip/unzip.h"
#include "api.h"

int g_skinloadedmanually;
int g_skinmissinggenff = 0;

HWND skin_hwnd;

BOOL _cleanupDirW(const wchar_t *dir)
{
	// a file that can't be deleted yet doesn't stop the rest going. anything left over is retried
	// by Skin_CleanupAfterCrash on the next start
	for (const auto &entry : FilePath::List(dir))
	{
		if (FilePath::IsDirectory(entry))
			_cleanupDirW(entry.path().c_str());
		else
			FilePath::Remove(entry.path());
	}

	return FilePath::Remove(dir);
}

// attempt to cleanup the last extracted temp folder for a skin incase Winamp crashed on exit
void Skin_CleanupAfterCrash(void)
{
	wchar_t buf[1024] = {0};
	char str[78] = {0};
	StringCchPrintfA(str,78,"skin_clean_up%ws",szAppName);
	_r_sW(str, buf, sizeof(buf));
	if (buf[0])
	{
		_cleanupDirW(buf);
		_w_sW(str, 0);
	}
}

void Skin_CleanupZip(void)
{
	if (!SKINTEMPDIR[0]) return ;
	if (_cleanupDirW(SKINTEMPDIR))
	{
		char str[78] = {0};
		StringCchPrintfA(str,78,"skin_clean_up%ws",szAppName);
		_w_s(str, 0);
	}
}

void CreateDirectoryForFileW(wchar_t *fn, wchar_t *base)
{
	wchar_t buf1[MAX_PATH] = {0};
	wchar_t *tmp;
	wchar_t *p;
	StringCchCopyW(buf1, MAX_PATH, fn);
	tmp = scanstr_backW(buf1, L"\\/", buf1);
	*tmp = 0;
	tmp = buf1;
	while (tmp && *tmp) { if (*tmp == L'/') *tmp = L'\\'; tmp++; }

	std::error_code error;
	p = buf1 + wcslen(base);
	while (p && *p)
	{
		while (p && *p != L'\\' && *p) p = CharNextW(p);
		if (p && !*p) std::filesystem::create_directory(buf1, error);
		else
		{
			if (p) *p = 0;
			std::filesystem::create_directory(buf1, error);
			if (p) *p++ = L'\\';
		}
	}
}

static void make_skin_dir(void)
{
	if (config_skin[0])
	{
		if (_wcsicmp(extensionW(config_skin), L"zip") && _wcsicmp(extensionW(config_skin), L"wsz") && _wcsicmp(extensionW(config_skin), L"wal"))
		{
			if (FilePath::IsFileSpec(config_skin) || FilePath::IsRelative(config_skin))
				FilePath::Combine(skin_directory, SKINDIR, config_skin);
			else 
				StringCchCopyW(skin_directory, MAX_PATH, config_skin);
		}
		else
		{
			wchar_t dirmask[MAX_PATH] = {0};
			char str[78] = {0};
			StringCchCopyW(skin_directory, MAX_PATH, SKINTEMPDIR);
			std::error_code error; std::filesystem::create_directory(SKINTEMPDIR, error);
			StringCchPrintfA(str, 78, "skin_clean_up%ws", szAppName);
			_w_sW(str, SKINTEMPDIR);
			{
				unzFile f;
				if (FilePath::IsFileSpec(config_skin) || FilePath::IsRelative(config_skin))
					FilePath::Combine(dirmask, SKINDIR, config_skin);
				else
					StringCchCopyW(dirmask, MAX_PATH, config_skin);
				f = unzOpen(AutoChar(dirmask)); 
				if (f)
				{
					int iswa3 = 0;

					if (unzGoToFirstFile(f) == UNZ_OK)
					{
						wchar_t buriedskinxml[MAX_PATH] = {0};
						wchar_t *p;
						StringCchCopyW(buriedskinxml, MAX_PATH, config_skin);
						p = wcschr(buriedskinxml, '.');
						if (p) *p = 0;
						StringCchCatW(buriedskinxml, MAX_PATH, L"/skin.xml");
						do
						{
							char filename[MAX_PATH] = {0};
							unzGetCurrentFileInfo(f, NULL, filename, sizeof(filename), NULL, 0, NULL, 0);
							if (!_stricmp(filename, "skin.xml") || !_stricmp(filename, AutoChar(buriedskinxml))) iswa3++;
						}
						while (!iswa3 && unzGoToNextFile(f) == UNZ_OK);
					}

					if (unzGoToFirstFile(f) == UNZ_OK)
					{
						do
						{
							char filename[MAX_PATH], *fn, *p;
							int success = 0;
							unzGetCurrentFileInfo(f, NULL, filename, sizeof(filename), NULL, 0, NULL, 0);

							//Only extract the file-types that could be in a skin
							//If we don't filter here it's a security hole
							if (iswa3)
							{
								if (unzOpenCurrentFile(f) == UNZ_OK)
								{
									fn = filename;
									if (strstr(fn, ":")) fn = strstr(fn, ":") + 1;
									while (fn && *fn == '\\') fn++;
									p = extension(fn);

									// TODO: really should enum image loaders so we only extract supported image files									
									if (fn[0] && (p != NULL && (!_stricmp(p, "xml") || !_stricmp(p, "png") ||
										!_stricmp(p, "cur") || !_stricmp(p, "bmp") || !_stricmp(p, "txt") ||
										!_stricmp(p, "gif") || !_stricmp(p, "ttf") || !_stricmp(p, "m") ||
										!_stricmp(p, "maki") || !_stricmp(p, "mp3") || !_stricmp(p, "wma") ||
										!_stricmp(p, "nsv") || !_stricmp(p, "nsa") || !_stricmp(p, "m4a") ||
										!_stricmp(p, "avi") || !_stricmp(p, "wav") || !_stricmp(p, "mp4") ||
										!_stricmp(p, "ogg") || IsPlaylistExtension(AutoWide(p)) ||
										!_stricmp(p, "mpg") || !_stricmp(p, "mid") || !_stricmp(p, "midi") ||
										!_stricmp(p, "mpeg") || !_stricmp(p, "url") || !_stricmp(p, "jpg") ||
										!_stricmp(p, "mi") ) ) )
									{
										FilePath::Combine(dirmask, SKINTEMPDIR, AutoWide(fn));
										CreateDirectoryForFileW(dirmask, SKINTEMPDIR);
										goto do_write;
									}
								}
							}
							else
							{
								fn = scanstr_back(filename, "\\/", filename - 1);
								p = extension(++fn);
								if (!_stricmp(p, "txt") || !_stricmp(p, "cur") || !_stricmp(p, "bmp") || !_stricmp(p, "ini"))
								{
									success = 0;
									if (unzOpenCurrentFile(f) == UNZ_OK)
									{
										FilePath::Combine(dirmask, SKINTEMPDIR, AutoWide(fn));

do_write:
										{
											std::ofstream fp{std::filesystem::path(dirmask), std::ios::binary};
											if (fp)
											{
												int l = 0;
												char buf[2048] = {0};
												success = 1;
												do
												{
													l = unzReadCurrentFile(f, buf, sizeof(buf));
													if (!l)
														unzCloseCurrentFile(f);
													if (l > 0 && !fp.write(buf, l))
														success = 0;
												} while (l > 0 && success);
											}
										}
									}
								}
							}
						}
						while (unzGoToNextFile(f) == UNZ_OK);
						} 
					unzClose(f);
				}
			}
		}
	}
	else skin_directory[0] = 0;
}

int Skin_PLColors[6] =
{
	RGB(0, 255, 0),
		RGB(255, 255, 255),
		RGB(0, 0, 0),
		RGB(0, 0, 198),
		RGB(0, 255, 0),
		RGB(0, 0, 0),
}, Skin_UseGenNums = 0;
char Skin_PLFont[128] = "";
wchar_t Skin_PLFontW[128] = L"";

static BOOL CALLBACK skinDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

static int *skin_rgn_points, *skin_rgn_numpoints, skin_rgn_numpoints_tot;
static int *skin_rgn_points_eq, *skin_rgn_numpoints_eq, skin_rgn_numpoints_tot_eq;
static int *skin_rgn_points_eq_ws, *skin_rgn_numpoints_eq_ws, skin_rgn_numpoints_tot_eq_ws;
static int *skin_rgn_points_ws, *skin_rgn_numpoints_ws, skin_rgn_numpoints_tot_ws;
static int *skin_rgn_points_pl, *skin_rgn_numpoints_pl, skin_rgn_numpoints_tot_pl;
static int *skin_rgn_points_pl_ws, *skin_rgn_numpoints_pl_ws, skin_rgn_numpoints_tot_pl_ws;

int Skin_GetRegionPointList( int eq, int **points, int **counts )
{
	if ( !eq )
	{
		if ( config_windowshade )
		{
			*points = skin_rgn_points_ws;
			*counts = skin_rgn_numpoints_ws;

			return skin_rgn_numpoints_tot_ws;
		}
		else
		{
			*points = skin_rgn_points;
			*counts = skin_rgn_numpoints;

			return skin_rgn_numpoints_tot;
		}
	}
	else if ( eq == 1 )
	{
		if ( config_eq_ws )
		{
			*points = skin_rgn_points_eq_ws;
			*counts = skin_rgn_numpoints_eq_ws;

			return skin_rgn_numpoints_tot_eq_ws;
		}
		else
		{
			*points = skin_rgn_points_eq;
			*counts = skin_rgn_numpoints_eq;

			return skin_rgn_numpoints_tot_eq;
		}
	}
	else if ( eq == 2 )
	{
		if ( config_pe_height == 14 )
		{
			*points = skin_rgn_points_pl_ws;
			*counts = skin_rgn_numpoints_pl_ws;

			return skin_rgn_numpoints_tot_pl_ws;
		}
		else
		{
			*points = skin_rgn_points_pl;
			*counts = skin_rgn_numpoints_pl;

			return skin_rgn_numpoints_tot_pl;
		}
	}


	return 0;
}

static int getnums( char *section, char *name, const wchar_t *fn, int **list )
{
	char buf[ 65535 ] = { 0 }, *p = buf;
	int n = 0, nn;
	GetPrivateProfileStringA( section, name, "", buf, sizeof( buf ), AutoChar( fn ) );

	while ( p )
	{
		int t = 0;
		while ( p && ( *p == ' ' || *p == '\t' || *p == ',' ) ) p++;
		if ( p && *p == '-' ) p++;
		while ( p && ( *p >= '0' && *p <= '9' ) )
		{
			t = 1; p++;
		}
		if ( t ) n++;
		else break;
	}

	if ( !n )
		return 0;

	p = buf;
	list[ 0 ] = (int *) GlobalAlloc( GPTR, sizeof( int ) * n );
	nn = n;
	n = 0;

	while ( p && n < nn )
	{
		int t = 0;
		int s = 0, sign = 1;

		while ( p && ( *p == ' ' || *p == '\t' || *p == ',' ) )
			p++;

		if ( p && *p == '-' )
		{
			sign = -1; p++;
		}

		while ( p && ( *p >= '0' && *p <= '9' ) )
		{
			s = s * 10 + *p++ - '0'; t = 1;
		}

		if ( t )
			list[ 0 ][ n++ ] = s * sign;
		else
			break;
	}
	return n;
}

static int hexGetPrivateProfileInt( char *sec, char *key, int def, const wchar_t *fn )
{
	char str[ 100 ] = { 0 }, *s = str;

	if ( !GetPrivateProfileStringA( sec, key, "", str, sizeof( str ), AutoChar( fn ) ) || !*str )
		return def;

	if ( s && *s == '#' )
		s++;

	if ( s )
		def = strtol( s, &s, 16 );

	int r = ( def >> 16 ) & 255;
	int g = ( def >> 8 ) & 255;
	int b = def & 255;

	return RGB( r, g, b );
}

HCURSOR Skin_Cursors[ N_CURSORS ];

static void Skin_LoadCursors( void )
{
	static struct
	{
		int cid;
		const wchar_t *fn;
	}
	cursor_loads[ N_CURSORS ] =
	{
		// main, non windowshade, start 0, end 8
		{IDC_LRSCROLL,     L"VolBal.cur"},   // vol & bal
		{IDC_LRSCROLL,     L"Posbar.cur"},   // pos
		{IDC_NORMALCURSOR, L"WinBut.cur"},   //wshade
		{IDC_NORMALCURSOR, L"Min.cur"},      //min
		{IDC_DANGER,       L"Close.cur"},    //close
		{IDC_NORMALCURSOR, L"MainMenu.cur"}, //mainmenu
		{IDC_MOVEMAIN,     L"TitleBar.cur"}, // titelbar
		{IDC_LRSCROLL,     L"SongName.cur"},
		{IDC_NORMALCURSOR, L"Normal.cur"},
		// main, windowshade, start 9, end 14
		{IDC_NORMALCURSOR, L"WinBut.cur"},   //wshade
		{IDC_NORMALCURSOR, L"Min.cur"},      //min
		{IDC_LRSCROLL,     L"WSPosbar.cur"}, // seeker
		{IDC_DANGER,       L"Close.cur"},    //close
		{IDC_NORMALCURSOR, L"MMenu.cur"},    //mainmenu
		{IDC_NORMALCURSOR, L"WSNormal.cur"},
		// playlist editor, normal, start 15 end 20
		{IDC_NORMALCURSOR, L"PWinBut.cur"},  //wshade
		{IDC_DANGER,       L"PClose.cur"},   //close
		{IDC_MOVEMAIN,     L"PTBar.cur"},    // titelbar
		{IDC_UDSCROLL,     L"PVScroll.cur"},
		{IDC_RESIZE,       L"PSize.cur"},
		{IDC_NORMALCURSOR, L"PNormal.cur"},
		// playlist editor, windowshade, start 21 end 24
		{IDC_NORMALCURSOR, L"PWinBut.cur"},  //wshade
		{IDC_DANGER,       L"PClose.cur"},   //close
		{IDC_LRSCROLL,     L"PWSSize.cur"},  //size
		{IDC_NORMALCURSOR, L"PWSNorm.cur"},
		// equalizer start 25, end 28
		{IDC_UDSCROLL,     L"EQSlid.cur"},
		{IDC_DANGER,       L"EQClose.cur"},  //close
		{IDC_MOVEMAIN,     L"EQTitle.cur"},  // titelbar
		{IDC_NORMALCURSOR, L"EQNormal.cur"},
	};

	int x;

	for ( x = 0; x < N_CURSORS; x++ )
	{
		if ( Skin_Cursors[ x ] )
			DestroyCursor( Skin_Cursors[ x ] );

		Skin_Cursors[ x ] = 0;
	}

	for ( x = 0; x < N_CURSORS; x++ )
	{
		HCURSOR hc = 0;

		if ( config_skin[ 0 ] )
		{
			wchar_t sn[ MAX_PATH ] = { 0 };
			FilePath::Combine( sn, skin_directory, cursor_loads[ x ].fn );
			hc = (HCURSOR) LoadImageW( NULL, sn, IMAGE_CURSOR, 0, 0, LR_LOADFROMFILE );
		}

		if ( !hc )
		{
			hc = (HCURSOR) LoadImageW( hMainInstance, MAKEINTRESOURCEW( cursor_loads[ x ].cid ), IMAGE_CURSOR, 0, 0, 0 );
		}

		Skin_Cursors[ x ] = hc;
	}
}

// .wal is a modern-only format, and only modern skin folders have a skin.xml
BOOL Skin_IsModern(const wchar_t *name, BOOL is_directory)
{
	if (!is_directory)
		return !_wcsicmp(extensionW((wchar_t *)name), L"wal");

	wchar_t path[MAX_PATH] = {0};
	FilePath::Combine(path, SKINDIR, name);
	FilePath::Append(path, L"skin.xml");
	return FilePath::Exists(path);
}

BOOL Skin_Check_Modern_Support()
{
	wchar_t fn[ MAX_PATH ] = { 0 };

	FilePath::Combine( fn, skin_directory, L"skin.xml" );

	if ( FilePath::Exists( fn ) && !GetModuleHandleW( L"gen_ff.dll" ) )
	{
		g_skinmissinggenff = 1;

		return FALSE;
	}

	g_skinmissinggenff = 0;

	return TRUE;
}

void Skin_Load(void)
{
	Skin_CleanupZip();
	make_skin_dir();
	g_skinmissinggenff = 0;

	if (skin_rgn_points) GlobalFree(skin_rgn_points);
	if (skin_rgn_numpoints) GlobalFree(skin_rgn_numpoints);
	skin_rgn_numpoints = NULL;
	skin_rgn_points = NULL;

	if (skin_rgn_points_ws) GlobalFree(skin_rgn_points_ws);
	if (skin_rgn_numpoints_ws) GlobalFree(skin_rgn_numpoints_ws);
	skin_rgn_numpoints_ws = NULL;
	skin_rgn_points_ws = NULL;

	if (skin_rgn_points_eq) GlobalFree(skin_rgn_points_eq);
	if (skin_rgn_numpoints_eq) GlobalFree(skin_rgn_numpoints_eq);
	skin_rgn_numpoints_eq = NULL;
	skin_rgn_points_eq = NULL;

	if (skin_rgn_points_eq_ws) GlobalFree(skin_rgn_points_eq_ws);
	if (skin_rgn_numpoints_eq_ws) GlobalFree(skin_rgn_numpoints_eq_ws);
	skin_rgn_numpoints_eq_ws = NULL;
	skin_rgn_points_eq_ws = NULL;

	if (skin_rgn_points_pl) GlobalFree(skin_rgn_points_pl);
	if (skin_rgn_numpoints_pl) GlobalFree(skin_rgn_numpoints_pl);
	skin_rgn_numpoints_pl = NULL;
	skin_rgn_points_pl = NULL;

	if (skin_rgn_points_pl_ws) GlobalFree(skin_rgn_points_pl_ws);
	if (skin_rgn_numpoints_pl_ws) GlobalFree(skin_rgn_numpoints_pl_ws);
	skin_rgn_numpoints_pl_ws = NULL;
	skin_rgn_points_pl_ws = NULL;

	skin_rgn_numpoints_tot = 0;
	skin_rgn_numpoints_tot_ws = 0;
	skin_rgn_numpoints_tot_eq = 0;
	skin_rgn_numpoints_tot_eq_ws = 0;
	skin_rgn_numpoints_tot_pl = 0;
	skin_rgn_numpoints_tot_pl_ws = 0;

	int t_Skin_PLColors[6] =
	{
		RGB(0, 255, 0),
		RGB(255, 255, 255),
		RGB(0, 0, 0),
		RGB(0, 0, 198),
		RGB(0, 255, 0),
		RGB(0, 0, 0),
	};
	memcpy(Skin_PLColors, t_Skin_PLColors, sizeof(Skin_PLColors));
	Skin_PLFont[0] = 0;
	Skin_PLFontW[0] = 0;
	Skin_UseGenNums = !config_skin[0];

	if (skin_directory[0])
	{
		if (!Skin_Check_Modern_Support())
		{
			Skin_LoadCursors();
			return;
		}

		wchar_t fn[MAX_PATH] = {0};
		FilePath::Combine(fn, skin_directory, L"region.txt");
		skin_rgn_numpoints_tot = getnums("Normal", "NumPoints", fn, &skin_rgn_numpoints);
		if (skin_rgn_numpoints_tot)
		{
			int all_p;
			if ((all_p = getnums("Normal", "PointList", fn, &skin_rgn_points)))
			{
				int o = 0;
				int x;
				for (x = 0; x < skin_rgn_numpoints_tot; x ++)
					o += skin_rgn_numpoints[x];
				if (o == all_p / 2 && !(all_p & 1))	goto s1;
				// else MessageBox(NULL,"region.txt:\n[Normal]\npoints in PointList\ndon't match NumPoints","Error in skin:",0);
			}
		}

		if (skin_rgn_numpoints) GlobalFree(skin_rgn_numpoints);
		if (skin_rgn_points) GlobalFree(skin_rgn_points);
		skin_rgn_numpoints_tot = 0;
		skin_rgn_numpoints = NULL;
		skin_rgn_points = NULL;

s1:
		skin_rgn_numpoints_tot_ws = getnums("WindowShade", "NumPoints", fn, &skin_rgn_numpoints_ws);
		if (skin_rgn_numpoints_tot_ws)
		{
			int all_p;
			if ((all_p = getnums("WindowShade", "PointList", fn, &skin_rgn_points_ws)))
			{
				int o = 0;
				int x;
				for (x = 0; x < skin_rgn_numpoints_tot_ws; x ++)
					o += skin_rgn_numpoints_ws[x];
				if (o == all_p / 2 && !(all_p & 1))	goto s2;
				// else MessageBox(NULL,"region.txt:\n[Windowshade]\npoints in PointList\ndon't match NumPoints","Error in skin:",0);
			}
		}
		if (skin_rgn_numpoints_ws) GlobalFree(skin_rgn_numpoints_ws);
		if (skin_rgn_points_ws) GlobalFree(skin_rgn_points_ws);
		skin_rgn_numpoints_ws = NULL;
		skin_rgn_numpoints_tot_ws = 0;
		skin_rgn_points_ws = NULL;

s2:
		skin_rgn_numpoints_tot_eq = getnums("Equalizer", "NumPoints", fn, &skin_rgn_numpoints_eq);
		if (skin_rgn_numpoints_tot_eq)
		{
			int all_p;
			if ((all_p = getnums("Equalizer", "PointList", fn, &skin_rgn_points_eq)))
			{
				int o = 0;
				int x;
				for (x = 0; x < skin_rgn_numpoints_tot_eq; x ++)
					o += skin_rgn_numpoints_eq[x];
				if (o == all_p / 2 && !(all_p & 1))	goto s3;
				// else MessageBox(NULL,"region.txt:\n[Equalizer]\npoints in PointList\ndon't match NumPoints","Error in skin:",0);
			}
		}
		if (skin_rgn_numpoints_eq) GlobalFree(skin_rgn_numpoints_eq);
		if (skin_rgn_points_eq) GlobalFree(skin_rgn_points_eq);
		skin_rgn_numpoints_eq = NULL;
		skin_rgn_numpoints_tot_eq = 0;
		skin_rgn_points_eq = NULL;

s3:
		skin_rgn_numpoints_tot_eq_ws = getnums("EqualizerWS", "NumPoints", fn, &skin_rgn_numpoints_eq_ws);
		if (skin_rgn_numpoints_tot_eq_ws)
		{
			int all_p;
			if ((all_p = getnums("EqualizerWS", "PointList", fn, &skin_rgn_points_eq_ws)))
			{
				int o = 0;
				int x;
				for (x = 0; x < skin_rgn_numpoints_tot_eq_ws; x ++)
					o += skin_rgn_numpoints_eq_ws[x];
				if (o == all_p / 2 && !(all_p & 1))	goto s4;
				// else MessageBox(NULL,"region.txt:\n[EqualizerWS]\npoints in PointList\ndon't match NumPoints","Error in skin:",0);
			}
		}
		if (skin_rgn_numpoints_eq_ws) GlobalFree(skin_rgn_numpoints_eq_ws);
		if (skin_rgn_points_eq_ws) GlobalFree(skin_rgn_points_eq_ws);
		skin_rgn_numpoints_eq_ws = NULL;
		skin_rgn_numpoints_tot_eq_ws = 0;
		skin_rgn_points_eq_ws = NULL;

s4:
		/*skin_rgn_numpoints_tot_pl = getnums("Playlist", "NumPoints", fn, &skin_rgn_numpoints_pl);
		if (skin_rgn_numpoints_tot_pl)
		{
			int all_p;
			if ((all_p = getnums("Playlist", "PointList", fn, &skin_rgn_points_pl)))
			{
				int o = 0;
				int x;
				for (x = 0; x < skin_rgn_numpoints_tot_pl; x ++)
					o += skin_rgn_numpoints_pl[x];
				if (o == all_p / 2 && !(all_p & 1))	goto s5;
				// else MessageBox(NULL,"region.txt:\n[Playlist]\npoints in PointList\ndon't match NumPoints","Error in skin:",0);
			}
		}*/
		if (skin_rgn_numpoints_pl) GlobalFree(skin_rgn_numpoints_pl);
		if (skin_rgn_points_pl) GlobalFree(skin_rgn_points_pl);
		skin_rgn_numpoints_pl = NULL;
		skin_rgn_numpoints_tot_pl = 0;
		skin_rgn_points_pl = NULL;


//s5:
		/*skin_rgn_numpoints_tot_pl_ws = getnums("PlaylistWS", "NumPoints", fn, &skin_rgn_numpoints_pl_ws);
		if (skin_rgn_numpoints_tot_pl_ws)
		{
			int all_p;
			if ((all_p = getnums("PlaylistWS", "PointList", fn, &skin_rgn_points_pl_ws)))
			{
				int o = 0;
				int x;
				for (x = 0; x < skin_rgn_numpoints_tot_pl_ws; x ++)
					o += skin_rgn_numpoints_pl_ws[x];
				if (o == all_p / 2 && !(all_p & 1))	goto s6;
				// else MessageBox(NULL,"region.txt:\n[PlaylistWS]\npoints in PointList\ndon't match NumPoints","Error in skin:",0);
			}
		}*/
//		if (skin_rgn_numpoints_pl_ws) GlobalFree(skin_rgn_numpoints_pl_ws);
//		if (skin_rgn_points_pl_ws) GlobalFree(skin_rgn_points_pl_ws);
//		skin_rgn_numpoints_pl_ws = NULL;
//		skin_rgn_numpoints_tot_pl_ws = 0;
//		skin_rgn_points_pl_ws = NULL;

//s6:
//		;

#if 0
		{
			char str[1024] = {0};
			FilePath::Combine(fn, skin_directory, "mb.ini");
			GetPrivateProfileString("Winamp", "MBOpen", "", str, sizeof(str), fn);
			str[1023] = 0;
			if (str[0])
			{
				if (hMBWindow)
				{
					if (g_skinloadedmanually) SendMessageW(hMainWindow, WM_USER, (WPARAM)0, IPC_MBOPEN);
					SendMessageW(hMainWindow, WM_USER, (WPARAM)str, IPC_MBOPEN);
				}
				else
				{
					extern char page_nav[1024];
					if (g_skinloadedmanually) config_mb_open = 1;
					lstrcpy(page_nav, str);
				}
			}
		}
#endif

		FilePath::Combine(fn, skin_directory, L"pledit.txt");

		Skin_PLColors[0] = hexGetPrivateProfileInt("Text", "Normal",     Skin_PLColors[0], fn);
		Skin_PLColors[1] = hexGetPrivateProfileInt("Text", "Current",    Skin_PLColors[1], fn);
		Skin_PLColors[2] = hexGetPrivateProfileInt("Text", "NormalBG",   Skin_PLColors[2], fn);
		Skin_PLColors[3] = hexGetPrivateProfileInt("Text", "SelectedBG", Skin_PLColors[3], fn);
		Skin_PLColors[4] = hexGetPrivateProfileInt("Text", "mbFG",       Skin_PLColors[4], fn);
		Skin_PLColors[5] = hexGetPrivateProfileInt("Text", "mbBG",       Skin_PLColors[5], fn);

		char plfont[256] = {0};

		GetPrivateProfileStringA("Text", "Font", "", plfont, 256, AutoChar(fn));
		MultiByteToWideCharSZ(CP_UTF8, 0, plfont, -1, Skin_PLFontW, sizeof(Skin_PLFontW)/sizeof(*Skin_PLFontW));
		WideCharToMultiByteSZ(CP_ACP, 0, Skin_PLFontW, -1, Skin_PLFont, sizeof(Skin_PLFont)/sizeof(*Skin_PLFont), 0, 0);
		// used to allow for the gen.bmp numbers support in 5.64 - need skin to say as too many have junk where it's taken from
		Skin_UseGenNums = GetPrivateProfileIntW(L"Misc", L"UseGenNums", 0, fn);

	}
	Skin_LoadCursors();
}

void Skin_Random(void)
{
	if (config_randskin)
	{
		const auto isSkin = [](const std::filesystem::directory_entry &entry) {
			if (FilePath::IsDirectory(entry))
				return true;
			const wchar_t *ext = extensionW(entry.path().filename().c_str());
			return !_wcsicmp(ext, L"zip") || !_wcsicmp(ext, L"wsz") || !_wcsicmp(ext, L"wal");
		};

		const auto entries = FilePath::List(SKINDIR);
		int v, x = 1;
		for (const auto &entry : entries)
		{
			if (isSkin(entry))
				x++;
		}
		v = warand() % x;
		if (!v)
		{
			config_skin[0] = 0;
			return ;
		}
		x = 1;
		for (const auto &entry : entries)
		{
			if (isSkin(entry))
			{
				if (x == v)
				{
					StringCchCopyW(config_skin, MAX_PATH, entry.path().filename().c_str());
					break;
				}
				x++;
			}
		}
PostMessageW(hMainWindow, WM_COMMAND, WINAMP_REFRESHSKIN, 0);

		if (prefs_last_page == 40 && IsWindow(prefs_hwnd))
		{
			prefs_last_page = 0;
			prefs_dialog(1);
			prefs_last_page = 40;
			prefs_dialog(1);
		}
	}
}