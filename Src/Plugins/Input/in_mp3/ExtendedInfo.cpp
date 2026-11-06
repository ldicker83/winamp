#include "main.h"
#include <filesystem>
#include <system_error>
#include "../nu/FilePath.h"
#include "Metadata.h"
#include "../Winamp/wa_ipc.h"
#include "../nu/ns_wc.h"
#include "Stopper.h"
#include <shlwapi.h>
#include "../Agave/Language/api_language.h"
#include <strsafe.h>

static std::filesystem::file_time_type lastWriteTime;

// is used to determine if the last write time of the file has changed when
// asked to get the metadata for the same cached file so we can update things
BOOL HasFileTimeChanged(const wchar_t *fn)
{
	if (!fn)
		return FALSE;

	std::error_code error;
	const auto time = std::filesystem::last_write_time(fn, error);
	if (!error && time != lastWriteTime)
	{
		lastWriteTime = time;
		return TRUE;
	}
	return FALSE;
}

void UpdateFileTimeChanged(const wchar_t *fn)
{
	if (!fn)
		return;

	std::error_code error;
	const auto time = std::filesystem::last_write_time(fn, error);
	if (!error)
	{
		lastWriteTime = time;
	}
}

Metadata *m_ext_set_mp3info = NULL;
Metadata *m_ext_get_mp3info = NULL;
extern "C" __declspec(dllexport)
int winampGetExtendedFileInfoW(const wchar_t *fn, const char *data, wchar_t *dest, size_t destlen)
{
	if (!_stricmp(data, "type"))
	{
		dest[0] = '0';
		dest[1] = 0;
		return 1;
	}
	if (!_stricmp(data, "rateable"))
	{
		dest[0] = '1';
		dest[1] = 0;
		return 1;
	}
	else if (!_stricmp(data, "family"))
	{
		if (!fn || !fn[0]) return 0;
		int len = lstrlenW(fn);
		if (len < 4 || L'.' != fn[len - 4])  return 0;
		const wchar_t *p = &fn[len - 3];
		if (!_wcsicmp(p, L"MP3")) { WASABI_API_LNGSTRINGW_BUF(IDS_FAMILY_STRING_MP3, dest, destlen); return 1; }
		if (!_wcsicmp(p, L"MP2")) { WASABI_API_LNGSTRINGW_BUF(IDS_FAMILY_STRING_MP2, dest, destlen); return 1; }
		if (!_wcsicmp(p, L"MP1")) { WASABI_API_LNGSTRINGW_BUF(IDS_FAMILY_STRING_MP1, dest, destlen); return 1; }
		if (!_wcsicmp(p, L"AAC")) { WASABI_API_LNGSTRINGW_BUF(IDS_FAMILY_STRING_MPEG2_AAC, dest, destlen); return 1; }
		if (!_wcsicmp(p, L"VLB")) { WASABI_API_LNGSTRINGW_BUF(IDS_FAMILY_STRING_DOLBY, dest, destlen); return 1; }
		return 0;
	}
	else if (!_stricmp(data, "mime"))
	{
		if (!fn || !fn[0]) return 0;
		int len = lstrlenW(fn);
		if (len < 4 || L'.' != fn[len - 4])  return 0;
		const wchar_t *p = &fn[len - 3];
		if (!_wcsicmp(p, L"MP3")) { StringCchCopyW(dest, destlen, L"audio/mpeg"); return 1; }
		if (!_wcsicmp(p, L"MP2")) { StringCchCopyW(dest, destlen, L"audio/mpeg"); return 1; }
		if (!_wcsicmp(p, L"MP1")) { StringCchCopyW(dest, destlen, L"audio/mpeg"); return 1; }
		if (!_wcsicmp(p, L"AAC")) { StringCchCopyW(dest, destlen, L"audio/aac"); return 1; }
		if (!_wcsicmp(p, L"VLB")) { StringCchCopyW(dest, destlen, L"audio/vlb"); return 1; }
		return 0;
	}

	if (!fn || !fn[0]) return 0;

	if (FilePath::IsUrl(fn))
		return 0;

	if (m_ext_get_mp3info && (!m_ext_get_mp3info->IsMe(fn) || HasFileTimeChanged(fn)))
	{
		m_ext_get_mp3info->Release();
		m_ext_get_mp3info=0;
	}

	if (!m_ext_get_mp3info)
	{
		m_ext_get_mp3info = new Metadata;
		m_ext_get_mp3info->Open(fn);
	}

	return m_ext_get_mp3info->GetExtendedData(data, dest, (int)destlen);
}

extern "C"
	__declspec(dllexport) int winampSetExtendedFileInfoW(const wchar_t *fn, const char *data, const wchar_t *val)
{
	if (!m_ext_set_mp3info || (m_ext_set_mp3info && !m_ext_set_mp3info->IsMe(fn)))
	{
		if(m_ext_set_mp3info) m_ext_set_mp3info->Release();
		m_ext_set_mp3info = new Metadata;
		m_ext_set_mp3info->Open(fn);
	}
	return m_ext_set_mp3info->SetExtendedData(data, val);
}

extern "C"
	__declspec(dllexport) int winampWriteExtendedFileInfo()
{
	// flush our read cache too :)
	if(m_ext_get_mp3info) m_ext_get_mp3info->Release();
	m_ext_get_mp3info = NULL;

	if (!m_ext_set_mp3info) return 0;

	Stopper stopper;
	if (m_ext_set_mp3info->IsMe(lastfn))
		stopper.Stop();

	// just in-case something changed
	if (!m_ext_set_mp3info) return 0;

	int ret = m_ext_set_mp3info->Save();
	stopper.Play();
	m_ext_set_mp3info->Release();
	m_ext_set_mp3info = NULL;

	// update last modified so we're not re-queried on our own updates
	UpdateFileTimeChanged(lastfn);

	return !ret;
}

extern "C" __declspec(dllexport)
	int winampClearExtendedFileInfoW(const wchar_t *fn)
{
	Metadata meta;
	if (meta.Open(fn)==METADATA_SUCCESS)
	{
		meta.id3v2.Clear();
		Stopper stopper;
		if (meta.IsMe(lastfn))
			stopper.Stop();
		meta.Save();
		stopper.Play();

		// update last modified so we're not re-queried on our own updates
		UpdateFileTimeChanged(fn);

		return 1;
	}
	return 0;
}