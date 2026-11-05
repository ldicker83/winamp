#include "WAT.h"
#include "../nu/FilePath.h"
#include <filesystem>
#include <system_error>

bool wa::files::file_exists( const char *p_filename )
{
	if ( p_filename == NULL )
		return false;

	return FilePath::Exists( p_filename );
}

bool wa::files::file_exists( const std::string &p_filename )
{
	return wa::files::file_exists( p_filename.c_str() );
}

bool wa::files::file_exists( const wchar_t *p_filename )
{
	if ( p_filename == NULL )
		return false;

	return FilePath::Exists( p_filename );
}


int wa::files::file_size( const char *p_filename )
{
	std::error_code l_error;
	const auto l_file_size = std::filesystem::file_size( p_filename, l_error );

	return l_error ? -1 : static_cast<int>( l_file_size );
}

int wa::files::file_size( const wchar_t *p_filename )
{
	std::error_code l_error;
	const auto l_file_size = std::filesystem::file_size( p_filename, l_error );

	return l_error ? -1 : static_cast<int>( l_file_size );
}


bool wa::files::folder_exists( const char *p_folder )
{
	return FilePath::IsDirectory( p_folder );
}


bool wa::files::getFilenamesFromFolder( std::vector<std::string> &p_result, const std::string &p_folder_path, const std::string &p_reg_ex, const size_t p_limit )
{
	const std::filesystem::path l_folder( p_folder_path );

	std::error_code l_error;
	if ( !std::filesystem::is_directory( l_folder, l_error ) )
		return false;

	const std::wstring l_mask = std::filesystem::path( p_reg_ex ).native();

	for ( const auto &l_entry : FilePath::List( l_folder, l_mask ) )
	{
		const std::string l_file_name = wa::strings::convert::to_string( l_entry.path().filename().native() );

		if ( FilePath::IsDirectory( l_entry ) )
			wa::files::getFilenamesFromFolder( p_result, p_folder_path + "\\" + l_file_name, p_reg_ex );
		else
			p_result.push_back( p_folder_path + "\\" + l_file_name );

		if ( p_result.size() >= p_limit - 1 )
			break;
	}

	return true;
}
