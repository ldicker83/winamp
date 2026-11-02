@echo off
rem Installs the third party libraries this build links against.
rem Requires vcpkg with "vcpkg integrate install" already run.

vcpkg install ^
	expat:x86-windows-static-md ^
	libflac:x86-windows-static-md ^
	libopenmpt:x86-windows-static-md ^
	libsndfile:x86-windows-static-md ^
	minizip:x86-windows-static-md ^
	mpg123:x86-windows-static-md ^
	zlib:x86-windows-static-md
