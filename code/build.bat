@echo off

rem Off-Line Warnings:
rem 4201: nonstandard extension used: nameless struct/union
rem 4100: unreferenced formal parameter
rem 4189: local variable is initialized but not referenced

IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build

cl -nologo -W4 -wd4201 -wd4100 -wd4189 -Zi -FC ..\wastale\code\win32_wastale.cpp kernel32.lib user32.lib gdi32.lib ole32.lib winmm.lib

popd
