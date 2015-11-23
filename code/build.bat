@echo off

IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build

cl -nologo -Zi -FC ..\wastale\code\win32_wastale.cpp kernel32.lib user32.lib gdi32.lib ole32.lib winmm.lib

popd
