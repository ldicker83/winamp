#include "ID3v2.h"
#include "id3.h"
#include "config.h"
#include "api__in_mp3.h"
#include "../nu/AutoChar.h"
#include "../nu/AutoWide.h"
#include <strsafe.h>

static inline const wchar_t *IncSafe(const wchar_t *val, int x)
{
	while (x--)
	{
		if (val && *val)
			val++;
	}
	return val;
}

extern const wchar_t *id3v1_genres[];
extern size_t numGenres;

ID3v2::ID3v2()
{
	hasData=false;
	dirty=false;
}

int ID3v2::Decode(const void *data, size_t len)
{
	if (!config_parse_id3v2 || !data)
	{
		hasData=false;
		return 0;
	}

	id3v2.Parse((uchar *)data, (uchar *)data+ID3_TAGHEADERSIZE);
	if (id3v2.NumFrames() > 0)
	{
		hasData=true;
		return 0;
	}
	else
		return 1;
}

// return -1 for empty, 1 for OK, 0 for "don't understand tag name"
int ID3v2::GetString(const char *tag, wchar_t *data, int dataLen)
{
	if (!_stricmp(tag, "title"))
		return ID3_GetTagText(&id3v2, ID3FID_TITLE, data, dataLen)?1:-1;
	else if (!_stricmp(tag, "album"))
		return ID3_GetTagText(&id3v2, ID3FID_ALBUM, data, dataLen)?1:-1;
	else if (!_stricmp(tag, "artist"))
		return ID3_GetTagText(&id3v2, ID3FID_LEADARTIST, data, dataLen)?1:-1;
	else if (!_stricmp(tag, "albumartist"))
	{
		if (!ID3_GetTagText(&id3v2, ID3FID_BAND, data, dataLen) && !ID3_GetUserText(&id3v2, L"ALBUM ARTIST", data, dataLen) && !ID3_GetUserText(&id3v2, L"ALBUMARTIST", data, dataLen))
			return ID3_GetUserText(&id3v2, L"Band", data, dataLen)?1:-1;
		else
			return 1;
	}
	else if (!_stricmp(tag, "comment"))
		return ID3_GetComment(&id3v2, data, dataLen)?1:-1;
	else if (!_stricmp(tag, "year"))
	{
		if (!ID3_GetTagText(&id3v2, ID3FID_RECORDINGTIME, data, dataLen))
			return ID3_GetTagText(&id3v2, ID3FID_YEAR, data, dataLen)?1:-1;
		else
			return 1;
	}
	else if (!_stricmp(tag, "composer"))
		return ID3_GetTagText(&id3v2, ID3FID_COMPOSER, data, dataLen)?1:-1;
	else if (!_stricmp(tag, "replaygain_track_gain"))
		return ID3_GetUserText(&id3v2, L"replaygain_track_gain", data, dataLen)?1:-1;
	else if (!_stricmp(tag, "replaygain_album_gain"))
		return ID3_GetUserText(&id3v2, L"replaygain_album_gain", data, dataLen)?1:-1;
	else if (!_stricmp(tag, "replaygain_track_peak"))
		return ID3_GetUserText(&id3v2, L"replaygain_track_peak", data, dataLen)?1:-1;
	else if (!_stricmp(tag, "replaygain_album_peak"))
		return ID3_GetUserText(&id3v2, L"replaygain_album_peak", data, dataLen)?1:-1;
	else if (!_stricmp(tag, "genre"))
	{
		data[0] = 0;
		if (ID3_GetTagText(&id3v2, ID3FID_CONTENTTYPE, data, dataLen))
		{
			wchar_t *tmp = data;
			while (tmp && *tmp == ' ') tmp++;
			if (tmp && (*tmp == '(' || (*tmp >= '0' && *tmp <= '9'))) // both (%d) and %d forms
			{
				int noparam = 0;
				if (*tmp == '(') tmp++;
				else noparam = 1;
				size_t genre_index = _wtoi(tmp);
				int cnt = 0;
				while (tmp && *tmp >= '0' && *tmp <= '9') cnt++, tmp++;
				while (tmp && *tmp == ' ') tmp++;

				if (tmp && (((!*tmp && noparam) || (!noparam && *tmp == ')')) && cnt > 0))
				{
					if (genre_index < numGenres)
						StringCchCopyW(data, dataLen, id3v1_genres[genre_index]);
				}
			}
			return 1;
		}
		return -1;
	}
	else if (!_stricmp(tag, "track"))
		return ID3_GetTagText(&id3v2, ID3FID_TRACKNUM, data, dataLen)?1:-1;
	else if (!_stricmp(tag, "disc"))
		return ID3_GetTagText(&id3v2, ID3FID_PARTINSET, data, dataLen)?1:-1;
	else if (!_stricmp(tag, "bpm"))
		return ID3_GetTagText(&id3v2, ID3FID_BPM, data, dataLen)?1:-1;
	else if (!_stricmp(tag, "rating"))
		return ID3_GetRating(&id3v2, data, dataLen)?1:-1;
	else if (!_stricmp(tag, "conductor"))
		return ID3_GetTagText(&id3v2, ID3FID_CONDUCTOR, data, dataLen)?1:-1;
	else if (!_stricmp(tag, "key"))
		return ID3_GetTagText(&id3v2, ID3FID_KEY, data, dataLen)?1:-1;
	else if (!_stricmp(tag, "mood"))
		return ID3_GetTagText(&id3v2, ID3FID_MOOD, data, dataLen)?1:-1;
	else if (!_stricmp(tag, "subtitle"))
		return ID3_GetTagText(&id3v2, ID3FID_SUBTITLE, data, dataLen)?1:-1;
	else if (!_stricmp(tag, "lyricist"))
		return ID3_GetTagText(&id3v2, ID3FID_LYRICIST, data, dataLen)?1:-1;
	else if (!_stricmp(tag, "ISRC"))
		return ID3_GetTagText(&id3v2, ID3FID_ISRC, data, dataLen)?1:-1;
	else if (!_stricmp(tag, "media"))
		return ID3_GetTagText(&id3v2, ID3FID_MEDIATYPE, data, dataLen)?1:-1;
	else if (!_stricmp(tag, "remixing"))
		return ID3_GetTagText(&id3v2, ID3FID_MIXARTIST, data, dataLen)?1:-1;
	else if (!_stricmp(tag, "originalartist"))
		return ID3_GetTagText(&id3v2, ID3FID_ORIGARTIST, data, dataLen)?1:-1;
	else if (!_stricmp(tag, "encoder"))
		return ID3_GetTagText(&id3v2, ID3FID_ENCODERSETTINGS, data, dataLen)?1:-1;
	else if (!_stricmp(tag, "publisher"))
		return ID3_GetTagText(&id3v2, ID3FID_PUBLISHER, data, dataLen)?1:-1;
	else if (!_stricmp(tag, "copyright"))
		return ID3_GetTagText(&id3v2, ID3FID_COPYRIGHT, data, dataLen)?1:-1;
	else if (!_stricmp(tag, "compilation"))
		return ID3_GetTagText(&id3v2, ID3FID_COMPILATION, data, dataLen)?1:-1;
	else if (!_stricmp(tag, "url"))
		return ID3_GetTagUrl(&id3v2, ID3FID_WWWUSER, data, dataLen)?1:-1;
	else if (!_stricmp(tag, "GracenoteFileID"))
		return ID3_GetGracenoteTagID(&id3v2, data, dataLen)?1:-1;
	else if (!_stricmp(tag, "GracenoteExtData"))
	{
		if (!ID3_GetUserText(&id3v2, L"GN_ExtData", data, dataLen))
			return ID3_GetUserText(&id3v2, L"GN/ExtData", data, dataLen)?1:-1;
		else
			return 1;
	}
	else if (!_stricmp(tag, "tool"))
		return ID3_GetTagText(&id3v2, ID3FID_ENCODEDBY, data, dataLen)?1:-1;
	else if (!_stricmp(tag, "pregap"))
	{
		data[0] = 0;
		// first, check for stupid iTunSMPB TXXX frame
		wchar_t gaps[128] = L"";
		const wchar_t *itr = ID3_GetComment(&id3v2, L"iTunSMPB", gaps, 128);
		if (itr && *itr)
		{
			itr = IncSafe(itr, 9);
			unsigned int prepad = wcstoul(itr, 0, 16);
			StringCchPrintfW(data, dataLen, L"%u", prepad);
			return 1;
		}
		return -1;
	}
	else if (!_stricmp(tag, "postgap"))
	{
		data[0] = 0;
		// first, check for stupid iTunSMPB TXXX frame
		wchar_t gaps[128] = L"";
		const wchar_t *itr = ID3_GetComment(&id3v2, L"iTunSMPB", gaps, 128);
		if (itr && *itr)
		{
			itr = IncSafe(itr, 18);
			unsigned int postpad = wcstoul(itr, 0, 16);
			StringCchPrintfW(data, dataLen, L"%u", postpad);
			return 1;
		}
		return -1;
	}
	else if (!_stricmp(tag, "numsamples"))
	{
		data[0] = 0;
		// first, check for stupid iTunSMPB TXXX frame
		wchar_t gaps[128] = L"";
		const wchar_t *itr = ID3_GetComment(&id3v2, L"iTunSMPB", gaps, 128);
		if (itr && *itr)
		{
			itr = IncSafe(itr, 27);
			unsigned __int64 samples = wcstoul(itr, 0, 16);
			StringCchPrintfW(data, dataLen, L"%I64u", samples);
			return 1;
		}
		return -1;
	}
	else if (!_stricmp(tag, "endoffset"))
	{
		data[0] = 0;
		// first, check for stupid iTunSMPB TXXX frame
		wchar_t gaps[128] = L"";
		const wchar_t *itr = ID3_GetComment(&id3v2, L"iTunSMPB", gaps, 128);
		if (itr && *itr)
		{
			itr = IncSafe(itr, 53);
			unsigned __int32 endoffset = wcstoul(itr, 0, 16);
			StringCchPrintfW(data, dataLen, L"%I32u", endoffset);
			return 1;
		}
		return -1;
	}
	else if (!_stricmp(tag, "category"))
	{
		return ID3_GetTagText(&id3v2, ID3FID_CONTENTGROUP, data, dataLen)?1:-1;
	}
	// things generally added by Musicbrainz tagging (either specific or additional)
	else if (!_stricmp(tag, "acoustid") || !_stricmp(tag, "acoustid_id"))
	{
		return ID3_GetUserText(&id3v2, L"Acoustid Id", data, dataLen)?1:-1;
	}
	else if (!_stricmp(tag, "acoustid_fingerprint"))
	{
		return ID3_GetUserText(&id3v2, L"Acoustid Fingerprint", data, dataLen)?1:-1;
	}
	else if (!_stricmp(tag, "asin"))
	{
		return ID3_GetUserText(&id3v2, L"ASIN", data, dataLen)?1:-1;
	}
	else if (!_stricmp(tag, "barcode"))
	{
		return ID3_GetUserText(&id3v2, L"BARCODE", data, dataLen)?1:-1;
	}
	else if (!_stricmp(tag, "catalognumber"))
	{
		return ID3_GetUserText(&id3v2, L"CATALOGNUMBER", data, dataLen)?1:-1;
	}
	else if (!_stricmp(tag, "script"))
	{
		return ID3_GetUserText(&id3v2, L"SCRIPT", data, dataLen)?1:-1;
	}
	else if (!_stricmp(tag, "musicbrainz_recordingid")) // (track id)
	{
		return ID3_GetMusicbrainzRecordingID(&id3v2, data, dataLen)?1:-1;
	}
	else if (!_stricmp(tag, "musicbrainz_trackid"))	// TODO not working (album track id)
	{
		return ID3_GetUserText(&id3v2, L"MusicBrainz Release Track Id", data, dataLen)?1:-1;
	}
	else if (!_stricmp(tag, "musicbrainz_albumid"))
	{
		return ID3_GetUserText(&id3v2, L"MusicBrainz Album Id", data, dataLen)?1:-1;
	}
	else if (!_stricmp(tag, "musicbrainz_artistid"))
	{
		return ID3_GetUserText(&id3v2, L"MusicBrainz Artist Id", data, dataLen)?1:-1;
	}
	else if (!_stricmp(tag, "musicbrainz_albumartistid"))
	{
		return ID3_GetUserText(&id3v2, L"MusicBrainz Album Artist Id", data, dataLen)?1:-1;
	}
	else if (!_stricmp(tag, "musicbrainz_releasestatus") || !_stricmp(tag, "musicbrainz_albumstatus"))
	{
		return ID3_GetUserText(&id3v2, L"MusicBrainz Album Status", data, dataLen)?1:-1;
	}
	else if (!_stricmp(tag, "musicbrainz_releasetype") || !_stricmp(tag, "musicbrainz_albumtype"))
	{
		return ID3_GetUserText(&id3v2, L"MusicBrainz Album Type", data, dataLen)?1:-1;
	}
	else if (!_stricmp(tag, "musicbrainz_releasecountry") || !_stricmp(tag, "musicbrainz_albumcountry"))
	{
		return ID3_GetUserText(&id3v2, L"MusicBrainz Album Release Country", data, dataLen)?1:-1;
	}
	else if (!_stricmp(tag, "musicbrainz_releasegroupid") || !_stricmp(tag, "musicbrainz_albumgroupid"))
	{
		return ID3_GetUserText(&id3v2, L"MusicBrainz Release Group Id", data, dataLen)?1:-1;
	}	
	else
	{
		return 0;
	}
}

void ID3v2::add_set_latin_id3v2_frame(ID3_FrameID id, const wchar_t *c)
{
	ID3_Frame *f = id3v2.Find(id);
	if (!c)
	{
		if (f)
			id3v2.RemoveFrame(f);
	}
	else
	{
		if (f)
		{
			SetFrameEncoding(f, ENCODING_FORCE_ASCII);
			AutoChar temp(c); //AutoChar temp(c, 28591); // todo: benski> changed back to local to keep old winamp tagged files working
			f->Field(ID3FN_URL).SetLocal(temp); //f->Field(ID3FN_TEXT).SetLatin(temp);// todo: benski> changed back to local to keep old winamp tagged files working
		}
		else
		{
			f = new ID3_Frame(id);
			SetFrameEncoding(f, ENCODING_FORCE_ASCII);
			AutoChar temp(c); //AutoChar temp(c, 28591); // todo: benski> changed back to local to keep old winamp tagged files working
			f->Field(ID3FN_URL).SetLocal(temp); //f->Field(ID3FN_TEXT).SetLatin(temp);// todo: benski> changed back to local to keep old winamp tagged files working
			id3v2.AddFrame(f, TRUE);
		}
	}
}

int ID3v2::SetString(const char *tag, const wchar_t *data)
{
	if (!_stricmp(tag, "artist"))
		add_set_id3v2_frame(ID3FID_LEADARTIST, data);
	else if (!_stricmp(tag, "album"))
		add_set_id3v2_frame(ID3FID_ALBUM, data);
	else if (!_stricmp(tag, "albumartist"))
	{
		add_set_id3v2_frame(ID3FID_BAND, data);
		if (!data || !*data) // if we're deleting the field
		{
			ID3_AddUserText(&id3v2, L"ALBUM ARTIST", data); // delete this alternate field also, or it's gonna look like it didn't "take" with a fb2k file
			ID3_AddUserText(&id3v2, L"ALBUMARTIST", data); // delete this alternate field also, or it's gonna look like it didn't "take" with an mp3tag file
			ID3_AddUserText(&id3v2, L"Band", data); // delete this alternate field also, or it's gonna look like it didn't "take" with an audacity file
		}
	}
	else if (!_stricmp(tag, "comment"))
		ID3_AddSetComment(&id3v2, data);
	else if (!_stricmp(tag, "title"))
		add_set_id3v2_frame(ID3FID_TITLE, data);
	else if (!_stricmp(tag, "year"))
	{
		add_set_id3v2_frame(ID3FID_YEAR, data);
		if (id3v2.version >= 4) // work around the fact that our id3 code doesn't handle versioning like this too well
			add_set_id3v2_frame(ID3FID_RECORDINGTIME, data);
		else
			add_set_id3v2_frame(ID3FID_RECORDINGTIME, (wchar_t *)0);
	}
	else if (!_stricmp(tag, "genre"))
		add_set_id3v2_frame(ID3FID_CONTENTTYPE, data);
	else if (!_stricmp(tag, "track"))
		add_set_id3v2_frame(ID3FID_TRACKNUM, data);
	else if (!_stricmp(tag, "disc"))
		add_set_id3v2_frame(ID3FID_PARTINSET, data);
	else if (!_stricmp(tag, "bpm"))
		add_set_id3v2_frame(ID3FID_BPM, data);
	else if (!_stricmp(tag, "rating"))
		ID3_AddSetRating(&id3v2, data);
	else if (!_stricmp(tag, "tool"))
		add_set_id3v2_frame(ID3FID_ENCODEDBY, data);
	else if (!_stricmp(tag, "composer"))
		add_set_id3v2_frame(ID3FID_COMPOSER, data);
	else if (!_stricmp(tag, "replaygain_track_gain"))
		ID3_AddUserText(&id3v2, L"replaygain_track_gain", data, ENCODING_FORCE_ASCII);
	else if (!_stricmp(tag, "replaygain_track_peak"))
		ID3_AddUserText(&id3v2, L"replaygain_track_peak", data, ENCODING_FORCE_ASCII);
	else if (!_stricmp(tag, "replaygain_album_gain"))
		ID3_AddUserText(&id3v2, L"replaygain_album_gain", data, ENCODING_FORCE_ASCII);
	else if (!_stricmp(tag, "replaygain_album_peak"))
		ID3_AddUserText(&id3v2, L"replaygain_album_peak", data, ENCODING_FORCE_ASCII);
	else if (!_stricmp(tag, "originalartist"))
		add_set_id3v2_frame(ID3FID_ORIGARTIST, data);
	else if (!_stricmp(tag, "encoder"))
		add_set_id3v2_frame(ID3FID_ENCODERSETTINGS, data);
	else if (!_stricmp(tag, "publisher"))
		add_set_id3v2_frame(ID3FID_PUBLISHER, data);
	else if (!_stricmp(tag, "copyright"))
		add_set_id3v2_frame(ID3FID_COPYRIGHT, data);
	else if (!_stricmp(tag, "compilation"))
		add_set_id3v2_frame(ID3FID_COMPILATION, data);
	else if (!_stricmp(tag, "remixing"))
		add_set_id3v2_frame(ID3FID_MIXARTIST, data);
	else if (!_stricmp(tag, "ISRC"))
		add_set_id3v2_frame(ID3FID_ISRC, data);
	else if (!_stricmp(tag, "url"))
		add_set_latin_id3v2_frame(ID3FID_WWWUSER, data); // TODO: we should %## escape invalid characters
	//add_set_id3v2_frame(ID3FID_WWWUSER, data);
	else if (!_stricmp(tag, "GracenoteFileID"))
		ID3_AddSetGracenoteTagID(&id3v2, data);
	else if (!_stricmp(tag, "GracenoteExtData"))
	{
		ID3_AddUserText(&id3v2, L"GN_ExtData", data, ENCODING_FORCE_ASCII);
		ID3_AddUserText(&id3v2, L"GN_ExtData",0); // delete this alternate field also
	}
	else if (!_stricmp(tag, "category"))
		add_set_id3v2_frame(ID3FID_CONTENTGROUP, data);
	else
		return 0;
	hasData=true;
	dirty=true;
	return 1;
}

void ID3v2::add_set_id3v2_frame(ID3_FrameID id, const wchar_t *c)
{
	ID3_Frame *f = id3v2.Find(id);
	if (!c || !*c)
	{
		if (f)
			id3v2.RemoveFrame(f);
	}
	else
	{
		if (f)
		{
			SetFrameEncoding(f);
			f->Field(ID3FN_TEXT).SetUnicode(c);
		}
		else
		{
			f = new ID3_Frame(id);
			SetFrameEncoding(f);
			f->Field(ID3FN_TEXT).SetUnicode(c);
			id3v2.AddFrame(f, TRUE);
		}
	}
}

uint32_t ID3v2::EncodeSize()
{
	if (!hasData)
		return 0; // simple :)

	return (uint32_t)id3v2.Size();
}

int ID3v2::Encode(const void *data, size_t len)
{
	id3v2.Render((uchar *)data);
	return 0;
}

void ID3v2::Clear()
{
	dirty=1;
	hasData=false;
	id3v2.Clear();
}