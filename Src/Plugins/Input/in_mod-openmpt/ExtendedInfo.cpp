#include <bfc/platform/types.h>
#include <windows.h>
#include <strsafe.h>
#include "api__in_mod.h"
#include "resource.h"
#include <libopenmpt/libopenmpt.h>
#include <string>

static wchar_t *open_filename = 0;
static openmpt_module *info_mod = 0;
openmpt_module *OpenMod(const wchar_t *filename);

// Copies one libopenmpt metadata value into dest, truncating it to fit. dest is always
// terminated; returns 0 if there is no module or the library has no value for the key.
static int CopyMetadata(openmpt_module *mod, const char *key, wchar_t *dest, size_t destlen)
{
	const char *value = openmpt_module_get_metadata(mod, key);
	if (!value)
		return 0;

	std::wstring wide;
	const int length = MultiByteToWideChar(CP_UTF8, 0, value, -1, nullptr, 0);
	if (length > 0)
	{
		wide.resize(length);
		MultiByteToWideChar(CP_UTF8, 0, value, -1, wide.data(), length);
	}
	openmpt_free_string(value);

	StringCchCopyW(dest, destlen, wide.c_str()); // truncates long values such as the module message
	return 1;
}

extern "C" __declspec(dllexport)
	int winampGetExtendedFileInfoW(const wchar_t *fn, const char *data, wchar_t *dest, size_t destlen)
{
	if (!_stricmp(data, "type"))
	{
		dest[0]='0';
		dest[1]=0;
		return 1;
	}
	else if (!_stricmp(data, "family"))
	{
		size_t len;
		const wchar_t *p;
		if (!fn || !fn[0]) {
			return 0;
		}
		len = wcslen(fn);
		if (len < 4 || L'.' != fn[len - 4]) {
			return 0;
		}
		p = &fn[len - 3];
		StringCchPrintfW(dest, destlen, L"%s Module", p);
		return 1;
	} else {
		if (!open_filename || _wcsicmp(open_filename,fn)) {
			free(open_filename);
			open_filename = _wcsdup(fn);
			if (info_mod)
				openmpt_module_destroy(info_mod);
			info_mod = 0;
			info_mod = OpenMod(fn);
		}

		// unreadable or unsupported file: report nothing rather than empty values
		if (!info_mod || !dest || !destlen)
			return 0;

		if (!_stricmp(data, "length")) {
			double seconds = openmpt_module_get_duration_seconds(info_mod);
			StringCchPrintf(dest, destlen, L"%.0f", seconds*1000.0);
			return 1;
		} else if (!_stricmp(data, "artist")) {
			return CopyMetadata(info_mod, "artist", dest, destlen);
		} else if (!_stricmp(data, "tool")) {
			return CopyMetadata(info_mod, "tracker", dest, destlen);
		} else if (!_stricmp(data, "title")) {
			return CopyMetadata(info_mod, "title", dest, destlen);
		} else if (!_stricmp(data, "year")) {
			return CopyMetadata(info_mod, "date", dest, destlen);
		} else if (!_stricmp(data, "comment")) {
			return CopyMetadata(info_mod, "message", dest, destlen);
		}

		return 0;
	}
}