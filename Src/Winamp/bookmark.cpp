/** (c) Nullsoft, Inc.         C O N F I D E N T I A L
 ** Filename: 
 ** Project:
 ** Description:
 ** Author: 
 ** Created:
 **/
#include "bookmark.h"
#include "../nu/FilePath.h"

BookmarkWriter::BookmarkWriter():fp(0)
{
}

void BookmarkWriter::Open(const wchar_t *filename)
{
	 fp=FilePath::Open(filename, L"a+t");
}

void BookmarkWriter::New(const wchar_t *filename)
{
	 fp=FilePath::Open(filename, L"wt");
}

void BookmarkWriter::Write(const char *filename, const char *title)
{
	fprintf(fp,"%s\n%s\n",filename,title);
}

void BookmarkWriter::Close()
{
	if (fp) fclose(fp);
}