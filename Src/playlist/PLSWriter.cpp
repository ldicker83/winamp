#include "PLSWriter.h"
#include "../nu/AutoChar.h"
#include <filesystem>

// a title or filename that can't be converted comes through as null
static const char *Text(const char *text)
{
	return text ? text : "(null)";
}

PLSWriter::PLSWriter() : numEntries(0)
{
}

int PLSWriter::Open(const wchar_t *filename)
{
	fp.open(std::filesystem::path(filename), std::ios::binary);
	if (!fp.is_open())
		return 0;

	fp << "[playlist]\r\n";

	return 1;
}

void PLSWriter::Write(const wchar_t *filename)
{
	fp << "File" << ++numEntries << "=" << Text(AutoChar(filename)) << "\r\n";
}

void PLSWriter::Write(const wchar_t *filename, const wchar_t *title, int length)
{
	Write(filename);
	fp << "Title" << numEntries << "=" << Text(AutoChar(title)) << "\r\n";
	fp << "Length" << numEntries << "=" << length << "\r\n";
}

void PLSWriter::Close()
{
	fp << "NumberOfEntries=" << numEntries << "\r\n";
	fp << "Version=2\r\n";
	fp.close();
}
