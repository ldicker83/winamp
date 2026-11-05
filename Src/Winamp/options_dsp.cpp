/** (c) Nullsoft, Inc.         C O N F I D E N T I A L
 ** Filename:
 ** Project:
 ** Description:
 ** Author:
 ** Created:
 **/
#include "Main.h"
#include "../nu/FilePath.h"
#include "FileOperations.h"
#include "resource.h"
#include "Options.h"
#include "dsp.h"
#include "main.hpp"
#include "../nu/AutoChar.h"
#include "../nu/AutoWide.h"

static bool pluginsLoaded;

static void DSPUpdateSel( HWND hwndDlg, HWND listWindow, INT iItem )
{
	wchar_t fn[ MAX_PATH * 2 ] = { 0 }, *libname = fn;
	int skip = 0;

	if ( ListView_GetNextItem( listWindow, -1, LVIS_SELECTED ) != -1 )
	{
		ListView_GetItemTextW( listWindow, iItem, 1, fn, ARRAYSIZE( fn ) );
		SendDlgItemMessageA( hwndDlg, IDC_DSPMOD, CB_RESETCONTENT, 0, 0 );

		int root = lstrcmpW( fn, getStringW( IDS_NOT_LOADED, NULL, 0 ) );

		if ( fn[ 0 ] && root )
		{
			if ( lstrcmpW( config_dspplugin_name, libname ) )
				config_dspplugin_num = 0;
		}
		else
		{
			if ( fn[ 0 ] )
				skip = 1;

			config_dspplugin_num = 0;
		}

		StringCchCopyW( config_dspplugin_name, MAX_PATH, libname );

		if ( *libname && !skip )
		{
			wchar_t b[ 1024 ] = { 0 };
			FilePath::Combine( b, DSPDIR, libname );

			HINSTANCE hLib = LoadLibraryW( b );

			if ( hLib )
			{
				winampDSPGetHeaderType pr = (winampDSPGetHeaderType) GetProcAddress( hLib, "winampDSPGetHeader2" );
				if ( pr )
				{
					int i = 0;

					for ( ;;)
					{
						winampDSPModule *module = pr( hMainWindow )->getModule( i++ );

						if ( !module )
							break;

						SendDlgItemMessageA( hwndDlg, IDC_DSPMOD, CB_ADDSTRING, 0, (LPARAM) (LPCSTR)module->description );
						// The description must be send in ASCII
					}
				}

				FreeModule( hLib );
			}

			SendDlgItemMessageA( hwndDlg, IDC_DSPMOD, CB_SETCURSEL, config_dspplugin_num, 0 );
		}
	}

	config_dspplugin_num = (unsigned char) SendDlgItemMessageA( hwndDlg, IDC_DSPMOD, CB_GETCURSEL, 0, 0 );

	if ( config_dspplugin_num == CB_ERR )
		config_dspplugin_num = 0;

	if ( pluginsLoaded )
	{
		dsp_quit();
		dsp_init();
	}

	if ( skip )
		fn[ 0 ] = 0;

	EnableWindow( GetDlgItem( hwndDlg, IDC_DSPCONF ), libname && !!*libname );
	EnableWindow( GetDlgItem( hwndDlg, IDC_DSPMOD ), libname && !!*libname );
	EnableWindow( GetDlgItem( hwndDlg, IDC_PLUGINLABEL ), libname && !!*libname );
}

// dsp tab procedure
INT_PTR CALLBACK DspProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	hi helpinfo[] = {
		{IDC_DSPLIB,IDS_P_DSP_LIB},
		{IDC_DSPMOD,IDS_P_DSP_MOD},
		{IDC_DSPCONF,IDS_P_DSP_CONF},
	};

	DO_HELP();

	if ( uMsg == WM_INITDIALOG )
	{
		pluginsLoaded = false;

		link_startsubclass( hwndDlg, IDC_PLUGINVERS );

		HWND listWindow = GetDlgItem( hwndDlg, IDC_DSPLIB );
		if ( IsWindow( listWindow ) )
		{
			RECT r = { 0 };
			GetWindowRect( listWindow, &r );
			GetClientRect( listWindow, &r );
			MapWindowPoints( listWindow, hwndDlg, (LPPOINT) &r, 2 );
			InflateRect( &r, 2, 2 );
			DestroyWindow( listWindow );

			listWindow = CreateWindowExW( WS_EX_NOPARENTNOTIFY | WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
										  WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL |
										  LVS_SHOWSELALWAYS | LVS_SORTASCENDING | LVS_NOCOLUMNHEADER | LVS_NOSORTHEADER,
										  r.left, r.top, r.right - r.left, r.bottom - r.top,
										  hwndDlg, (HMENU) IDC_DSPLIB, NULL, NULL );

			SetWindowPos( listWindow, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOSENDCHANGING );

			ListView_SetExtendedListViewStyleEx( listWindow, LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP, LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP );
			SendMessageW( listWindow, WM_SETFONT, SendMessageW( hwndDlg, WM_GETFONT, 0, 0 ), FALSE );

			LVCOLUMNW lvc = { 0 };
			ListView_InsertColumnW( listWindow, 0, &lvc );
			ListView_InsertColumnW( listWindow, 1, &lvc );

			if ( !g_safeMode )
			{
				SendMessageW( listWindow, LB_SETITEMDATA,
							  SendMessageW( listWindow, LB_ADDSTRING, 0, (LPARAM) getStringW( IDS_DSP_NONE, NULL, 0 ) ),
							  (LPARAM) 0 );
				SendMessageW( listWindow, LB_SETCURSEL, 0, 0 );

				LVITEMW lvi = { LVIF_TEXT | LVIF_STATE, 0, 0 };
				lvi.pszText = getStringW( IDS_DSP_NONE, NULL, 0 );
				lvi.state = LVIS_SELECTED | LVIS_FOCUSED;
				ListView_InsertItemW( listWindow, &lvi );
			}

			for ( const auto &entry : FilePath::List( DSPDIR, L"DSP_*.DLL" ) )
			{
				std::wstring name = entry.path().filename().native();
				if ( !g_safeMode )
				{
					wchar_t dsp_dir[ 1024 ] = { 0 }, namestr[ MAX_PATH + 256 ] = { 0 };
					FilePath::Combine( dsp_dir, DSPDIR, name.c_str() );

					HINSTANCE hLib = LoadLibraryW( dsp_dir );
					if ( hLib )
					{
						winampDSPGetHeaderType pr = (winampDSPGetHeaderType) GetProcAddress( hLib, "winampDSPGetHeader2" );
						if ( pr )
						{
							winampDSPHeader *header = pr( hMainWindow );

							if ( header && header->version >= DSP_HDRVER && header->version < DSP_HDRVER + 0x10 )
								StringCchCopyW( namestr, MAX_PATH + 256, AutoWide( header->description ) );
							else
								StringCchCopyW( namestr, MAX_PATH + 256, L"!" );
						}
						else
						{
							StringCchCopyW( namestr, MAX_PATH + 256, L"!" );
						}
						FreeModule( hLib );
					}
					else
					{
						StringCchCopyW( namestr, MAX_PATH + 256, L"!" );
					}

					if ( wcscmp( namestr, L"!" ) )
					{
						LVITEMW lvi = { LVIF_TEXT, 0, 0 };
						lvi.pszText = namestr;
						lvi.iItem = ListView_InsertItemW( listWindow, &lvi );

						lvi.mask = LVIF_TEXT;
						lvi.iSubItem = 1;
						lvi.pszText = name.data();
						ListView_SetItemW( listWindow, &lvi );

						if ( !_wcsicmp( name.c_str(), config_dspplugin_name ) )
						{
							ListView_SetItemState( listWindow, lvi.iItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
							DSPUpdateSel( hwndDlg, listWindow, lvi.iItem );
						}
					}
					else
					{
						LVITEMW lvi = { LVIF_TEXT, 0, 0 };
						lvi.pszText = name.data();
						lvi.iItem = ListView_InsertItemW( listWindow, &lvi );

						lvi.mask = LVIF_TEXT;
						lvi.iSubItem = 1;
						lvi.pszText = getStringW( IDS_NOT_LOADED, NULL, 0 );
						ListView_SetItemW( listWindow, &lvi );
					}
				}
				else
				{
					LVITEMW lvi = { LVIF_TEXT, 0, 0 };
					lvi.pszText = name.data();
					lvi.iItem = ListView_InsertItemW( listWindow, &lvi );

					lvi.mask = LVIF_TEXT;
					lvi.iSubItem = 1;
					lvi.pszText = getStringW( IDS_NOT_LOADED, NULL, 0 );
					ListView_SetItemW( listWindow, &lvi );
				}
			}

			if ( g_safeMode && !ListView_GetItemCount( listWindow ) )
			{
				LVITEMW lvi = { LVIF_TEXT | LVIF_STATE, 0, 0 };
				lvi.pszText = getStringW( IDS_DSP_NONE, NULL, 0 );
				lvi.state = LVIS_SELECTED | LVIS_FOCUSED;
				ListView_InsertItemW( listWindow, &lvi );
			}

			GetClientRect( listWindow, &r );
			ListView_SetColumnWidth( listWindow, 1, LVSCW_AUTOSIZE );
			ListView_SetColumnWidth( listWindow, 0, ( r.right - r.left ) - ListView_GetColumnWidth( listWindow, 1 ) );

			DirectMouseWheel_EnableConvertToMouseWheel( listWindow, TRUE );

			pluginsLoaded = true;
		}
	}
	else if ( uMsg == WM_DESTROY )
	{
		HWND listWindow = GetDlgItem( hwndDlg, IDC_DSPLIB );

		if ( IsWindow( listWindow ) )
			DirectMouseWheel_EnableConvertToMouseWheel( listWindow, FALSE );
	}
	else if ( uMsg == WM_NOTIFY )
	{
		static int own_update;

		LPNMHDR p = (LPNMHDR) lParam;
		if ( p->idFrom == IDC_DSPLIB )
		{
			if ( p->code == LVN_ITEMCHANGED && pluginsLoaded && !own_update )
			{
				if ( !g_safeMode )
				{
					LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam;
					LVITEM lvi = { LVIF_PARAM, pnmv->iItem };

					if ( ListView_GetItem( p->hwndFrom, &lvi ) && ( pnmv->uNewState & LVIS_SELECTED ) )
					{
						own_update = 1;
						DSPUpdateSel( hwndDlg, p->hwndFrom, pnmv->iItem );
						own_update = 0;
					}
				}
			}
			else if ( p->code == NM_DBLCLK || p->code == NM_CLICK )
			{
				// helps to keep the selection on things...
				LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE) lParam;
				if ( lpnmitem->iItem == -1 )
				{
					int which = ListView_GetNextItem( p->hwndFrom, -1, LVIS_SELECTED );
					if ( which == -1 )
					{
						for ( int i = 0; i < ListView_GetItemCount( p->hwndFrom ); i++ )
						{
							wchar_t fn[ MAX_PATH * 2 ] = { 0 };
							ListView_GetItemTextW( p->hwndFrom, i, 1, fn, ARRAYSIZE( fn ) );

							if ( !_wcsicmp( fn, config_dspplugin_name ) )
							{
								own_update = 1;
								ListView_SetItemState( p->hwndFrom, i, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
								own_update = 0;
								break;
							}
						}
					}
					else
						ListView_SetItemState( p->hwndFrom, which, LVIS_SELECTED, LVIS_SELECTED );
				}

				if ( p->code == NM_DBLCLK )
				{
					PostMessageW( hwndDlg, WM_COMMAND, MAKEWPARAM( IDC_DSPCONF, 0 ), (LPARAM) GetDlgItem( hwndDlg, IDC_DSPCONF ) );
				}
			}
		}
		else if ( p->code == HDN_ITEMCHANGINGW )
		{
			if ( pluginsLoaded )
			{
#ifdef WIN64
				SetWindowLongPtrW( hwndDlg, DWLP_MSGRESULT, TRUE );
#else
				SetWindowLongW( hwndDlg, DWL_MSGRESULT, TRUE );
#endif
				return TRUE;
			}
		}
	}
	else if ( uMsg == WM_COMMAND )
	{
		switch ( LOWORD( wParam ) )
		{
			case IDC_DSPMOD:
				if ( HIWORD( wParam ) == CBN_SELCHANGE )
				{
					config_dspplugin_num = (unsigned char) SendDlgItemMessageA( hwndDlg, IDC_DSPMOD, CB_GETCURSEL, 0, 0 );

					if ( config_dspplugin_num == CB_ERR )
						config_dspplugin_num = 0;

					if ( pluginsLoaded && config_dspplugin_name[ 0 ] )
					{
						dsp_quit();
						dsp_init();
					}
				
					return FALSE;
				}


			case IDC_DSPCONF:
			{
				if ( g_safeMode ) return FALSE;

				if ( IsWindowEnabled( GetDlgItem( hwndDlg, IDC_DSPCONF ) ) )
				{
					wchar_t b[ 1024 ] = { 0 };
					FilePath::Combine( b, DSPDIR, config_dspplugin_name );
					HINSTANCE hLib = LoadLibraryW( b );
					if ( hLib )
					{
						winampDSPGetHeaderType pr = (winampDSPGetHeaderType) GetProcAddress( hLib, "winampDSPGetHeader2" );
						winampDSPModule *module = pr( hMainWindow )->getModule( SendDlgItemMessageA( hwndDlg, IDC_DSPMOD, CB_GETCURSEL, 0, 0 ) );

						if ( module )
						{
							module->hDllInstance = hLib;
							module->hwndParent = hMainWindow;

							if ( !( config_no_visseh & 2 ) )
							{
								try
								{
									module->Config( module );
								}
								catch ( ... )
								{
									LPMessageBox( hwndDlg, IDS_PLUGINERROR, IDS_ERROR, MB_OK | MB_ICONEXCLAMATION );
								}
							}
							else
							{
								module->Config( module );
							}
						}
						else
						{
							LPMessageBox( hwndDlg, IDS_ERRORLOADINGPLUGIN, IDS_ERROR, MB_OK );
						}

						FreeLibrary( hLib );
					}
					else
					{
						LPMessageBox( hwndDlg, IDS_ERRORLOADINGPLUGIN, IDS_ERROR, MB_OK );
					}
				}
			
				return FALSE;
			}

			case IDC_OPENPLUGINDIR_DSP:
			{
				OpenFolder( hwndDlg, DSPDIR );
				return FALSE;
			}

		}
	}

	link_handledraw( hwndDlg, uMsg, wParam, lParam );
	return FALSE;
} //dsp