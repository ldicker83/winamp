#pragma once
#include <windows.h>

// Copies a file or directory into destinationFolder, creating the folder if need be.
bool InstallItem(const wchar_t *source, const wchar_t *destinationFolder, const wchar_t *destinationFilename);

// Fails if something else already has that name, so that a rename can't destroy another file.
bool RenameItem(const wchar_t *oldName, const wchar_t *newName);

// Moves everything in oldDirectory into newDirectory, across volumes if need be.
bool MoveDirectoryContents(const wchar_t *oldDirectory, const wchar_t *newDirectory);

// Removing plug-ins and skins is left to the user, who gets to see the folder they live in.
void OpenFolder(HWND parent, const wchar_t *folder);
