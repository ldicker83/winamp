#pragma once

#include <windows.h>
#include <unknwn.h>

// Plug-ins get this interface from IPC_GET_FILEREGISTRAR_OBJECT, so the methods
// and their order are part of the plug-in ABI. Don't remove or reorder them.
struct __declspec(uuid("2E74C695-8E9C-4179-B0A0-BC2EBDEB5C2B")) __declspec(novtable) IFileTypeRegistrar : public IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE RegisterMIMEType(LPCWSTR mimeType, LPCWSTR programName, LPCWSTR extension, BOOL netscapeOnly) = 0;
	virtual HRESULT STDMETHODCALLTYPE RegisterCDPlayer(LPCWSTR programName) = 0;
	virtual HRESULT STDMETHODCALLTYPE UnregisterCDPlayer(LPCWSTR programName) = 0;
	virtual HRESULT STDMETHODCALLTYPE RegisterType(LPCWSTR extension, LPCWSTR which_str, LPCWSTR prog_name) = 0;
	virtual HRESULT STDMETHODCALLTYPE UnregisterType(LPCWSTR extension, LPCWSTR which_str, LPCWSTR prog_name, int is_playlist) = 0;
	virtual HRESULT STDMETHODCALLTYPE AddDirectoryContext(LPCWSTR commandLine, LPCWSTR which_str, LPCWSTR description) = 0;
	virtual HRESULT STDMETHODCALLTYPE RemoveDirectoryContext(LPCWSTR which_str) = 0;
	virtual HRESULT STDMETHODCALLTYPE AddAgent(LPCWSTR agentFilename) = 0;
	virtual HRESULT STDMETHODCALLTYPE RemoveAgent() = 0;
	virtual HRESULT STDMETHODCALLTYPE RegisterMediaPlayer(DWORD accessEnabled, LPCWSTR programName, LPCWSTR prog_name, int iconNumber) = 0;
	virtual HRESULT STDMETHODCALLTYPE RegisterMediaPlayerProtocol(LPCWSTR protocol, LPCWSTR prog_name) = 0;
	virtual HRESULT STDMETHODCALLTYPE UnregisterMediaPlayerProtocol(LPCWSTR protocol, LPCWSTR prog_name) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetupFileType(LPCWSTR programName, LPCWSTR winamp_file, LPCWSTR name, int iconNumber, LPCWSTR defaultShellCommand, LPCWSTR iconPath) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetupShell(LPCWSTR commandLine, LPCWSTR winamp_file, LPCWSTR description, LPCWSTR commandName, LPCWSTR dragAndDropGUID) = 0;
	virtual HRESULT STDMETHODCALLTYPE RemoveShell(LPCWSTR winamp_file, LPCWSTR commandName) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetupDefaultFileType(LPCWSTR winamp_file, LPCWSTR defaultShellCommand) = 0;
	virtual HRESULT STDMETHODCALLTYPE RegisterTypeShell(LPCWSTR programName, LPCWSTR which_file, LPCWSTR description, int iconNumber, LPCWSTR commandName) = 0;
	virtual HRESULT STDMETHODCALLTYPE RegisterGUID(LPCWSTR programName, LPCWSTR guidString) = 0;
	virtual HRESULT STDMETHODCALLTYPE RegisterDVDPlayer(LPCWSTR programName, int iconNumber, LPCWSTR which_file, LPCWSTR commandName, LPCWSTR provider, LPCWSTR description) = 0;
	virtual HRESULT STDMETHODCALLTYPE InstallItem(LPCWSTR sourceFile, LPCWSTR destinationFolder, LPCWSTR destinationFilename) = 0;
	virtual HRESULT STDMETHODCALLTYPE DeleteItem(LPCWSTR file) = 0;
	virtual HRESULT STDMETHODCALLTYPE RenameItem(LPCWSTR oldFile, LPCWSTR newFile, BOOL force) = 0;
	virtual HRESULT STDMETHODCALLTYPE CleanupDirectory(LPCWSTR directory) = 0;
	virtual HRESULT STDMETHODCALLTYPE MoveDirectoryContents(LPCWSTR oldDirectory, LPCWSTR newDirectory) = 0;
	virtual HRESULT STDMETHODCALLTYPE WriteProKey(LPCWSTR name, LPCWSTR key) = 0;
	virtual HRESULT STDMETHODCALLTYPE WriteClientUIDKey(LPCWSTR path, LPCWSTR uid_str) = 0;
	virtual HRESULT STDMETHODCALLTYPE RegisterProtocol(LPCWSTR protocol, LPCWSTR command, LPCWSTR icon) = 0;
	virtual HRESULT STDMETHODCALLTYPE RegisterCapability(LPCWSTR programName, LPCWSTR winamp_file, LPCWSTR extension) = 0;
};
