@echo off

rem Off-Line Warnings:
rem 4201: nonstandard extension used: nameless struct/union
rem 4100: unreferenced formal parameter
rem 4189: local variable is initialized but not referenced
rem 4505: unreferenced local function has been removed

set CommonComplierOptions=-nologo -MTd -Gm- -fp:fast -GR- -EHa -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -Zi -FC -DWASTALE_SLOW=1 -DWASTALE_INTERNAL=1
set CommonLinkerFlags=-incremental:no -opt:ref kernel32.lib user32.lib gdi32.lib ole32.lib winmm.lib

IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build

del *.pdb > NUL 2> NUL
echo "WAITING FOR PDB" > lock.tmp
cl %CommonComplierOptions% ..\wastale\code\wastale.cpp -Fmwastale.map -LD /link /PDB:wastale_%random%.pdb -incremental:no -opt:ref -EXPORT:GameUpdateAndRender -EXPORT:GameFillSound
del lock.tmp
cl %CommonComplierOptions% ..\wastale\code\win32_wastale.cpp -Fmwin32_wastale.map /link %CommonLinkerFlags%

popd
