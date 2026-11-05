#include "main.h"
#include "../nu/FilePath.h"
#include "gen.h"
#include <vector>
#include "../nu/AutoWide.h"

#include "..\WAT\wa_logger.h"


std::vector<winampGeneralPurposePlugin*> gen_plugins;
extern LARGE_INTEGER freq;

typedef struct _PLUGINORDER
{
	LPCWSTR	name;
	bool	found;
} PLUGINORDER;

static PLUGINORDER preload[] =
{
	{ L"gen_hotkeys.dll", false },  //
	{ L"gen_tray.dll",    false },  //
	{ L"gen_jumpex.dll",  false },
};

void LoadGenPlugin(const wchar_t* filename)
{
	wchar_t file[MAX_PATH] = { 0 };
	FilePath::Combine(file, PLUGINDIR, filename);

	if (!wa::files::file_exists(file))
	{
		wsprintfW( _log_message_w, L"The plugin '%s' is not found in the \"Plugins\" folder!", filename );

		LOG_ERROR( _log_message_w );


		return;
	}


	HMODULE hLib = LoadLibraryW(file);
	if (hLib == NULL)
	{
		DWORD l_error_code = ::GetLastError();

		wsprintfW( _log_message_w, L"Error when loading the plugin '%s'! Error code : %d!", filename, l_error_code );


		LOG_ERROR( _log_message_w );


		return;
	}

	winampGeneralPurposePlugin* (*pr)();
	pr = (winampGeneralPurposePlugin * (__cdecl*)(void)) GetProcAddress(hLib, "winampGetGeneralPurposePlugin");
	if ( pr == NULL )
	{
		wsprintfW( _log_message_w, L"No entry point found for the plugin '%s'!", filename );

		LOG_ERROR( _log_message_w );


		FreeModule(hLib);
		return;
	}


	winampGeneralPurposePlugin* plugin = pr();
	if ( plugin && (plugin->version == GPPHDR_VER || plugin->version == GPPHDR_VER_U) )
	{
		wsprintfW( _log_message_w, L"The plugin '%s' is correctly loaded!", filename );

		LOG_DEBUG( _log_message_w );


		if ( g_safeMode )
		{
			char desc[128] = { 0 };
			lstrcpynA(desc, plugin->description, sizeof(desc));
			if ( desc[0] && !memcmp(desc, "nullsoft(", 9) )
			{
				char* p = strrchr(desc, ')');
				if ( p )
				{
					*p = 0;
					if ( _wcsicmp(filename, AutoWide(desc + 9)) )
					{
						FreeModule(hLib);
						return;
					}
				}
			}
			else
			{
				// TODO need to look into making this into a controlled
				//		list or something like that without the need for
				//		a client update so it's possible for user to fix
				//
				// for some plug-ins, we sadly need to leave them loaded
				// otherwise they'll just cause Winamp to keep crashing!
				if ( _wcsicmp(FilePath::FindFileName(filename), L"gen_Wake_up_call.dll") )
				{
					FreeModule(hLib);
				}
				return;
			}
		}

		plugin->hwndParent   = hMainWindow;
		plugin->hDllInstance = hLib;

		if ( plugin->init() == GEN_INIT_SUCCESS )
		{
			wsprintfW( _log_message_w, L"The plugin '%s' is initialized!", filename );

			LOG_DEBUG( _log_message_w );


			gen_plugins.push_back(plugin);
		}
		else
		{
			wsprintfW( _log_message_w, L"An error occurs when initializing the plugin '%s'!", filename );

			LOG_ERROR( _log_message_w );
			
			
			FreeModule( hLib );
		}
	}
	else
	{
		wsprintfW( _log_message_w, L"Either the plugin '%s' can't be loaded, either its version is incorrect!", filename );

		LOG_ERROR( _log_message_w );

		FreeModule( hLib );
	}
}

void load_genplugins()
{
	int i = 0, count = sizeof(preload) / sizeof(PLUGINORDER);
	for ( ; i < count; i++ )
		LoadGenPlugin(preload[i].name);

	for ( const auto &entry : FilePath::List(PLUGINDIR, L"GEN_*.DLL") )
	{
		const std::wstring name = entry.path().filename().native();
		for ( i = 0; i < count && (preload[i].found || lstrcmpiW(preload[i].name, name.c_str())); i++ );

		if ( i == count )
			LoadGenPlugin(name.c_str());
		else
			preload[i].found = true;
	}
}

void unload_genplugins()
{
	size_t x = gen_plugins.size();
	while ( x-- )
	{
		if ( gen_plugins[x] )
		{
			wchar_t filename[MAX_PATH] = { 0 };

			if ( !(config_no_visseh & 4) )
			{
				try
				{
					if ( gen_plugins[x]->quit )
						gen_plugins[x]->quit();
				}
				catch ( ... )
				{
				}
			}
			else
			{
				if ( gen_plugins[x]->quit )
					gen_plugins[x]->quit();
			}

			gen_plugins[x] = 0;
		}
	}
	try
	{
		gen_plugins.clear();
	}
	catch ( ... )
	{
	}

}