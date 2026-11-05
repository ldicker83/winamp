/*
** Copyright (C) 2007-2011 Nullsoft, Inc.
**
** This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held 
** liable for any damages arising from the use of this software. 
**
** Permission is granted to anyone to use this software for any purpose, including commercial applications, and to 
** alter it and redistribute it freely, subject to the following restrictions:
**
**   1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. 
**      If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
**
**   2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
**
**   3. This notice may not be removed or altered from any source distribution.
**
** Author: Ben Allison benski@winamp.com
** Created: March 1, 2007
**
*/
#include "Metadata.h"
#include <FLAC/all.h>
#include "api__in_flv.h"
#include "../nu/FilePath.h"
#include <strsafe.h>
#include <filesystem>
#include <fstream>
#include <system_error>

struct MetadataReader
{
	bool Open(const std::filesystem::path &filename, std::ios::openmode mode)
	{
		file.open(filename, mode | std::ios::binary);
		return file.is_open();
	}

	// a stream has to be repositioned when it changes between reading and writing
	void SwitchTo(bool write)
	{
		if (writing != write)
		{
			file.clear();
			file.seekg(file.tellg());
			writing = write;
		}
	}

	std::fstream file;
	bool endOfFile = false;
	bool writing = false;
};

static size_t metadata_read(void *ptr, size_t size, size_t nmemb, FLAC__IOHandle handle)
{
	MetadataReader *reader = (MetadataReader *)handle;
	reader->SwitchTo(false);
	reader->file.read(static_cast<char *>(ptr), static_cast<std::streamsize>(size*nmemb));
	const size_t bytesRead = static_cast<size_t>(reader->file.gcount());
	if (bytesRead == 0 && !reader->file.bad())
		reader->endOfFile=true;
	// a short read sets eofbit and failbit, which would stop the next tell or seek
	if (!reader->file.bad())
		reader->file.clear();
	return bytesRead/size;
}

static size_t metadata_write(const void *ptr, size_t size, size_t nmemb, FLAC__IOHandle handle)
{
	MetadataReader *reader = (MetadataReader *)handle;
	reader->SwitchTo(true);
	if (!reader->file.write(static_cast<const char *>(ptr), static_cast<std::streamsize>(size*nmemb)))
		return 0;
	return nmemb;
}

static int metadata_seek(FLAC__IOHandle handle, FLAC__int64 offset, int whence)
{
	MetadataReader *reader = (MetadataReader *)handle;
	const std::ios::seekdir direction = whence == SEEK_SET ? std::ios::beg : (whence == SEEK_CUR ? std::ios::cur : std::ios::end);
	reader->file.clear();
	reader->file.seekg(static_cast<std::streamoff>(offset), direction);
	if (!reader->file)
		return -1;
	reader->endOfFile = false;
	reader->writing = false;
	return 0;
}

static FLAC__int64 metadata_tell(FLAC__IOHandle handle)
{
	MetadataReader *reader = (MetadataReader *)handle;
	return static_cast<std::streamoff>(reader->file.tellg());
}

static int metadata_eof(FLAC__IOHandle handle)
{
	MetadataReader *reader = (MetadataReader *)handle;
	return !!reader->endOfFile;
}

static int metadata_close(FLAC__IOHandle handle)
{
	MetadataReader *reader = (MetadataReader *)handle;

	reader->file.close();
	return 0;
}

static FLAC__IOCallbacks unicodeIO =
{
  metadata_read,
  metadata_write,
  metadata_seek,
  metadata_tell,
  metadata_eof,
  metadata_close,
};

FLACMetadata::FLACMetadata()
{
	chain=FLAC__metadata_chain_new();
	itr = FLAC__metadata_iterator_new();
	block=0;
	streamInfo=0;
	filesize=0;
}

FLACMetadata::~FLACMetadata()
{
	if (chain)
		FLAC__metadata_chain_delete(chain);
	if (itr)
		FLAC__metadata_iterator_delete(itr);
}

void FLACMetadata::Reset()
{
	if (chain)
		FLAC__metadata_chain_delete(chain);
	if (itr)
		FLAC__metadata_iterator_delete(itr);
	chain=FLAC__metadata_chain_new();
	itr = FLAC__metadata_iterator_new();
	block=0;
	streamInfo=0;
	filesize=0;
}

const FLAC__StreamMetadata_StreamInfo *FLACMetadata::GetStreamInfo()
{
	if (streamInfo)
		return &streamInfo->data.stream_info;
	else
		return 0;
}

bool FLACMetadata::Open(const wchar_t *filename, bool optimize)
{
	if (!chain || !itr)
		return false;

	MetadataReader reader;
	if (!reader.Open(filename, std::ios::in))
		return false;

	std::error_code error;
	filesize = static_cast<__int64>(std::filesystem::file_size(filename, error));
	if (error)
		return false;

	FLAC__bool success = FLAC__metadata_chain_read_with_callbacks(chain, &reader, unicodeIO);
	reader.file.close();
	if (!success)
		return false;

	if (optimize)
	{
		FLAC__metadata_chain_sort_padding(chain);
		FLAC__metadata_chain_merge_padding(chain);
	}

	FLAC__metadata_iterator_init(itr, chain);
	while (1)
	{
		FLAC__MetadataType type=FLAC__metadata_iterator_get_block_type(itr);
		switch (type)
		{
		case FLAC__METADATA_TYPE_VORBIS_COMMENT:
			block = FLAC__metadata_iterator_get_block(itr);
			break;
		case FLAC__METADATA_TYPE_STREAMINFO:
			streamInfo = FLAC__metadata_iterator_get_block(itr);
			break;
		}
		if (FLAC__metadata_iterator_next(itr) == false)
			break;
	}
	return true;
}

const char *FLACMetadata::GetMetadata(const char *tag)
{
	if (!block)
		return 0;

	int pos = FLAC__metadata_object_vorbiscomment_find_entry_from(block, 0, tag);
	if (pos < 0)
	{
		// fail
	}
	else
	{
		const char *entry = (const char *)block->data.vorbis_comment.comments[pos].entry;
		const char *metadata = strchr(entry, '='); // find the first equal
		if (metadata)
		{
			return metadata+1;
		}
	}

	return 0;
}

void FLACMetadata::SetMetadata(const char *tag, const char *value)
{
	if (!block)
	{
		FLAC__metadata_iterator_init(itr, chain);
		do
		{
			if (FLAC__METADATA_TYPE_VORBIS_COMMENT == FLAC__metadata_iterator_get_block_type(itr))
			{
				block = FLAC__metadata_iterator_get_block(itr);
				break;
			}
		}
		while (FLAC__metadata_iterator_next(itr) != 0);
		if (!block)
		{
			block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);
			FLAC__metadata_iterator_insert_block_after(itr, block);
		}		
	}

	if (!block)
		return;

	FLAC__StreamMetadata_VorbisComment_Entry entry;
	size_t totalLen = strlen(tag) + 1 /* = */ + strlen(value);
	entry.entry = (FLAC__byte *)malloc(totalLen + 1);
	entry.length = totalLen;

	StringCchPrintfA((char *)entry.entry, totalLen+1, "%s=%s", tag, value);

	int pos = FLAC__metadata_object_vorbiscomment_find_entry_from(block, 0, tag);
	if (pos < 0)
	{
		//new comment
		FLAC__metadata_object_vorbiscomment_append_comment(block, entry, true);
		// would love to not copy, but we can't guarantee that FLAC links to the same CRT as us
	}
	else
	{
		FLAC__metadata_object_vorbiscomment_set_comment(block, pos, entry, true);
		// would love to not copy, but we can't guarantee that FLAC links to the same CRT as us
	}
	free(entry.entry);
}

void FLACMetadata::RemoveMetadata(const char *tag)
{
	if (!block)
		return;

	FLAC__metadata_object_vorbiscomment_remove_entries_matching(block, tag);
}

void FLACMetadata::RemoveMetadata(int n)
{
	if (!block)
		return;

	FLAC__metadata_object_vorbiscomment_delete_comment(block, (unsigned int)n);
}

static FLAC__StreamMetadata *GetOrMakePadding(FLAC__Metadata_Chain *chain, FLAC__Metadata_Iterator *itr)
{
	FLAC__metadata_iterator_init(itr, chain);
	while (1)
	{
		if (FLAC__METADATA_TYPE_PADDING == FLAC__metadata_iterator_get_block_type(itr))
		{
			FLAC__StreamMetadata *block = FLAC__metadata_iterator_get_block(itr);
			return block;
		}

		if (FLAC__metadata_iterator_next(itr) == false)
			break;
	}
	FLAC__StreamMetadata *padding = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING);
	if (padding)
		FLAC__metadata_iterator_insert_block_after(itr, padding);

	return padding;
}

bool FLACMetadata::Save(const wchar_t *filename)
{
	if (FLAC__metadata_chain_check_if_tempfile_needed(chain, true))
	{
		// since we needed to write a tempfile, let's add some more padding so it doesn't happen again
		FLAC__metadata_chain_sort_padding(chain);

		FLAC__StreamMetadata *padding = GetOrMakePadding(chain, itr);
		if (padding && padding->length < 16384)
			padding->length = 16384; // TODO: configurable padding size

		MetadataReader reader;
		if (!reader.Open(filename, std::ios::in))
			return false;

		const std::filesystem::path tempFile = FilePath::UniqueName(FilePath::TempDirectory(), L"waf");
		MetadataReader tempReader;
		if (!tempReader.Open(tempFile, std::ios::in | std::ios::out | std::ios::trunc))
			return false;

		FLAC__bool res = FLAC__metadata_chain_write_with_callbacks_and_tempfile(chain, false, &reader, unicodeIO, &tempReader, unicodeIO);

		reader.file.close();
		tempReader.file.close();

		// only replace the original if the new file was written in full
		const bool replaced = res && FilePath::Copy(tempFile, filename, true);
		FilePath::Remove(tempFile);
		return replaced;
	}
	else
	{
		MetadataReader reader;
		if (!reader.Open(filename, std::ios::in | std::ios::out))
			return false;

		FLAC__bool res = FLAC__metadata_chain_write_with_callbacks(chain, true, &reader, unicodeIO);

		reader.file.close();
		return !!res;
	}
}

bool FLACMetadata::GetLengthMilliseconds(unsigned __int64 *length)
{
	if (!streamInfo)
		return false;
	*length = (__int64)((double)(FLAC__int64)streamInfo->data.stream_info.total_samples / (double)streamInfo->data.stream_info.sample_rate * 1000.0 + 0.5);
	return true;
}

int FLACMetadata::GetNumMetadataItems()
{
	if (block) return block->data.vorbis_comment.num_comments;
	else return 0;
}

const char* FLACMetadata::EnumMetadata(int n, char *tag, int taglen)
{
	if (tag) tag[0]=0;
	if (!block) return 0;
	const char *entry = (const char *)block->data.vorbis_comment.comments[n].entry;
	const char *metadata = strchr(entry, '='); // find the first equal
	if (metadata)
	{
		if (tag) lstrcpynA(tag,entry,min(metadata-entry+1,taglen));
		return metadata+1;
	}
	else return 0;
}

void FLACMetadata::SetTag(int pos, const char *tag)
{
	char * value = (char*)EnumMetadata(pos,0,0);
	value = _strdup(value?value:"");
	FLAC__StreamMetadata_VorbisComment_Entry entry;
	size_t totalLen = strlen(tag) + 1 /* = */ + strlen(value);
	entry.entry = (FLAC__byte *)malloc(totalLen + 1);
	entry.length = totalLen;
	StringCchPrintfA((char *)entry.entry, totalLen+1, "%s=%s", tag, value);
	FLAC__metadata_object_vorbiscomment_set_comment(block, pos, entry, true);
	free(value);
}
