#ifndef NULLSOFT_IN_FLAC_METADATA_H
#define NULLSOFT_IN_FLAC_METADATA_H

#include <FLAC/all.h>

class FLACMetadata
{
public:
	FLACMetadata();
	~FLACMetadata();
	bool Open(const wchar_t *filename, bool optimize=false);
	void Reset();
	const char *GetMetadata(const char *tag);
	void SetMetadata(const char *tag, const char *value);
	void RemoveMetadata(const char *tag);
	void RemoveMetadata(int n);
	bool Save(const wchar_t *filename);
	const FLAC__StreamMetadata_StreamInfo *GetStreamInfo();
	__int64 GetFileSize() { return filesize; }
	bool GetLengthMilliseconds(unsigned __int64 *length);
	int GetNumMetadataItems();
	const char* EnumMetadata(int n, char *tag, int len);
	void SetTag(int n, const char *tag);
private:
	FLAC__Metadata_Chain *chain;
	FLAC__Metadata_Iterator *itr;
	FLAC__StreamMetadata *block;
	FLAC__StreamMetadata *streamInfo;
	__int64 filesize;
};

class Info
{
public:
	FLACMetadata metadata;
	const wchar_t *filename;
};

extern FLACMetadata *getMetadata;
extern wchar_t *getFileInfoFn;
extern Info *info;

#endif