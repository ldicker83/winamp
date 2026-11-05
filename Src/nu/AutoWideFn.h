#ifndef NULLSOFT_UTILITY_AUTOWIDEFN_H
#define NULLSOFT_UTILITY_AUTOWIDEFN_H


/* Winamp defines this, but this little block lets us use this thing outside of Winamp */
#ifndef FILENAME_SIZE
#define FILENAME_SIZE (MAX_PATH*4)
#define REMOVE_FILENAME_SIZE
#endif

#include <windows.h>
#include "../nu/FilePath.h"
#include "AutoWide.h"
#include "AutoChar.h"
#include <shlwapi.h> 
#include <filesystem>
#include <string>
#include <vector>
/*
Tries to find a filename that underwent a destructive Unicode-to-ANSI conversion
*/

#pragma warning(push)
#pragma warning(disable:4995)
class AutoWideFn
{
public:
	AutoWideFn(const char *narrowFn)
	{
		wideFn[0]=0;
		if (!narrowFn)
			return;
		CreateFile_HACK(narrowFn, wideFn);
	}

	operator wchar_t *() { return wideFn; }
	// a listing carried between calls, so a directory can be retried when a later part of the path doesn't match
	struct FindState
	{
		bool started = false;
		std::vector<std::filesystem::directory_entry> entries;
		size_t next = 0;
	};

	bool unicode_find(/*char *path,*/ char *pattern, wchar_t *out, UINT out_ptr, bool dir, FindState &f)
	{
		if (!f.started)
		{
			out[out_ptr] = 0;
			f.entries = FilePath::List(*out ? out : L".");
			f.started = true;
		}

		char temp[MAX_PATH*2 + 1] = {0};
		while (f.next < f.entries.size())
		{
			const auto &entry = f.entries[f.next++];
			if (FilePath::IsDirectory(entry) != dir)
				continue;

			const std::wstring name = entry.path().filename().native();
			WideCharToMultiByte(CP_ACP, 0, name.c_str(), -1, temp, sizeof(temp), 0, 0);
			if (!_stricmp(temp, pattern))
			{ //found
				lstrcpyW(out + out_ptr, name.c_str());
				return 1;
			}
		}

		return 0;
	}

	bool unicode_open_recur(char *path, char *ptr, wchar_t *out, UINT out_ptr)
	{
		char * next = strchr(ptr, '\\');
		if (next)
		{ //dig another dir
			FindState f;
			bool found;
			do
			{
				next[0] = 0;
				char * zz = _strdup(ptr);
				char bk = *ptr;
				*ptr = 0;
				found = unicode_find(/*path,*/ zz, out, out_ptr, 1, f);
				free(zz);
				*ptr = bk;
				next[0] = '\\';
				if (found)
				{
					UINT op_bk = out_ptr;
					while (out_ptr < FILENAME_SIZE && out[out_ptr]) out_ptr++;
					out[out_ptr++] = '\\';
					if (unicode_open_recur(path, next + 1, out, out_ptr))
					{
						return 1;
					}
					out_ptr = op_bk;
					out[out_ptr] = 0;
				}
			} while (found);
		}
		else
		{ //final dir
			char * zz = _strdup(ptr);
			char bk = *ptr;
			*ptr = 0;
			FindState files, directories;
			bool found = unicode_find(/*path,*/ zz, out, out_ptr, 0, files);
			if (!found)
				found = unicode_find(/*path,*/ zz, out, out_ptr, 1, directories);
			free(zz);
			*ptr = bk;
			return found;
		}
		return 0;
	}


	void CreateFile_HACK(const char * path, wchar_t out[FILENAME_SIZE])
	{
		MultiByteToWideChar(CP_ACP, 0, path, -1, out, FILENAME_SIZE);

		if (FilePath::IsUrl(out))
			return ;

		if (!StrChrW(out, L'?'))
			return ; // no unconvertables?  Great!

//		if (FilePath::Exists(out))
	//		return ; // no unconvertables?  Great!

		bool found = false;

		memset(out, 0, FILENAME_SIZE * sizeof(wchar_t));

		char * _p = _strdup(path);
		char * t = strchr(_p, '\\');

		if (t)
		{
			char bk = t[1];
			t[1] = 0;
			UINT o = MultiByteToWideChar(CP_ACP, 0, _p, -1, out, FILENAME_SIZE);
			o--;
			t[1] = bk;
			found = unicode_open_recur(_p, t + 1, out, o);
		}
		else
			found = unicode_open_recur(_p, _p, out, 0);
		free(_p);

		if (!found)
			MultiByteToWideChar(CP_ACP, 0, path, -1, out, FILENAME_SIZE);
	}
private:
	wchar_t wideFn[FILENAME_SIZE];
};

#pragma warning(pop)

#ifdef REMOVE_FILENAME_SIZE
#undef FILENAME_SIZE
#endif

#endif
