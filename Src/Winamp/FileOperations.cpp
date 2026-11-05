#include "FileOperations.h"
#include "../nu/FilePath.h"
#include <shellapi.h>
#include <filesystem>
#include <system_error>

static constexpr auto kCopyEverything = std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing;

bool InstallItem(const wchar_t *source, const wchar_t *destinationFolder, const wchar_t *destinationFilename)
{
	const std::filesystem::path from(source), to = std::filesystem::path(destinationFolder) / destinationFilename;

	std::error_code error;
	if (std::filesystem::equivalent(from, to, error))
		return true; // it's already there

	std::filesystem::create_directories(to.parent_path(), error);
	error.clear();
	std::filesystem::copy(from, to, kCopyEverything, error);
	return !error;
}

bool RenameItem(const wchar_t *oldName, const wchar_t *newName)
{
	std::error_code error;
	// the same file under another name is fine, which is how a change of case looks
	if (FilePath::Exists(newName) && !std::filesystem::equivalent(oldName, newName, error))
		return false;

	return FilePath::Rename(oldName, newName);
}

static bool MoveItem(const std::filesystem::path &from, const std::filesystem::path &to)
{
	if (FilePath::Rename(from, to))
		return true;

	// a rename can't cross volumes, so copy, and only then remove the original
	std::error_code error;
	std::filesystem::copy(from, to, kCopyEverything, error);
	if (error)
		return false;

	std::filesystem::remove_all(from, error);
	return true;
}

bool MoveDirectoryContents(const wchar_t *oldDirectory, const wchar_t *newDirectory)
{
	const std::filesystem::path destination(newDirectory);

	std::error_code error;
	std::filesystem::create_directories(destination, error);

	bool moved = true;
	for (const auto &entry : FilePath::List(oldDirectory))
	{
		if (!MoveItem(entry.path(), destination / entry.path().filename()))
			moved = false;
	}
	return moved;
}

void OpenFolder(HWND parent, const wchar_t *folder)
{
	ShellExecuteW(parent, L"open", folder, nullptr, nullptr, SW_SHOWNORMAL);
}
