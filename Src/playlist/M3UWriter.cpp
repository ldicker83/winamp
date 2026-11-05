#include "M3UWriter.h"
#include "../nu/AutoChar.h"
#include <filesystem>

// a title or filename that can't be converted comes through as null
static const char *Text( const char *text )
{
	return text ? text : "(null)";
}

M3UWriter::~M3UWriter()
{
	Close();
}

int M3UWriter::Open( const wchar_t *filename )
{
	if ( fp.is_open() )
		return 0;

	fp.open( std::filesystem::path( filename ) );
	if ( !fp.is_open() )
		return 0;

	fp << "#EXTM3U\n";

	return 1;
}

void M3UWriter::Write( const wchar_t *filename )
{
	if ( fp.is_open() )
		fp << Text( AutoChar( filename ) ) << "\n";
}

void M3UWriter::Write( const wchar_t *filename, const wchar_t *title, int length )
{
	if ( fp.is_open() )
		fp << "#EXTINF:" << length << "," << Text( AutoChar( title ) ) << "\n" << Text( AutoChar( filename ) ) << "\n";
}

void M3UWriter::Close()
{
	if ( fp.is_open() )
		fp.close();
}
