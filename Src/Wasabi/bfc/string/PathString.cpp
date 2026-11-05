#include "PathString.h"
#include "../nu/FilePath.h"
#include <shlwapi.h>

PathString::PathString(const wchar_t *directory, const wchar_t *filename)
{
	FilePath::Combine(path, directory, filename);
}