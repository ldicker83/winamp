#include "M3U8Writer.h"
#include "../nu/AutoChar.h"
#include <bfc/platform/types.h>
#include <filesystem>

// a title or filename that can't be converted comes through as null
static const char *Text( const char *text )
{
	return text ? text : "(null)";
}

M3U8Writer::M3U8Writer()
{
}

int M3U8Writer::Open(const wchar_t *filename)
{
	fp.open( std::filesystem::path( filename ) );
	if ( !fp.is_open() )
		return 0;

	fp << "\xEF\xBB\xBF" << "#EXTM3U\n";

	return 1;
}

void M3U8Writer::Write(const wchar_t *filename)
{
	fp << Text( AutoChar( filename, CP_UTF8 ) ) << "\n";
}

void M3U8Writer::Write(const wchar_t *filename, const wchar_t *title, int length)
{
	fp << "#EXTINF:" << length << "," << Text( AutoChar( title, CP_UTF8 ) ) << "\n" << Text( AutoChar( filename, CP_UTF8 ) ) << "\n";
}

void M3U8Writer::Write( const wchar_t *p_filename, const wchar_t *p_title, const wchar_t *p_extended_infos, int p_length )
{
	fp << "#EXTINF:" << p_length << " " << Text( AutoChar( p_extended_infos, CP_UTF8 ) ) << "," << Text( AutoChar( p_title, CP_UTF8 ) ) << "\n" << Text( AutoChar( p_filename, CP_UTF8 ) ) << "\n";
}

void M3U8Writer::Close()
{
	fp.close();
}
