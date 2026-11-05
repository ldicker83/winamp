#ifndef NULLSOFT_PLSWRITERH
#define NULLSOFT_PLSWRITERH

#include <fstream>
#include "PlaylistWriter.h"
class PLSWriter : public PlaylistWriter
{
public:
	PLSWriter();
	int Open(const wchar_t *filename);
	void Write(const wchar_t *filename);
	void Write(const wchar_t *filename, const wchar_t *title, int length);
	void Write( const wchar_t *p_filename, const wchar_t *p_title, const wchar_t *p_extended_infos, int p_length ) override
	{};
	void Close();
private:
	size_t numEntries;
	std::ofstream fp;
};

#endif