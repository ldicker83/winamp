@echo off
rem Installs the third party libraries this build links against.
rem Requires vcpkg with "vcpkg integrate install" already run.
rem
rem vcpkg-overlay-ports pins libopenmpt to a newer release than vcpkg's own
rem port. If an older libopenmpt is already installed, remove it first:
rem   vcpkg remove libopenmpt:x86-windows-static-md

vcpkg install --overlay-ports="%~dp0vcpkg-overlay-ports" ^
	libflac:x86-windows-static-md ^
	libopenmpt:x86-windows-static-md ^
	libsndfile:x86-windows-static-md ^
	minizip:x86-windows-static-md ^
	mpg123:x86-windows-static-md ^
	zlib:x86-windows-static-md
