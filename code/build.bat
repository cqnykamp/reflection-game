
@echo off

SETLOCAL

set BUILD_VERSION=REFLECT_INTERNAL
::set BUILD_VERSION=REFLECT_RELEASE

set Pincludes=w:\reflect\build\include
set Plibs=w:\reflect\build\lib

set YEAR=%date:~10,4%
set MONTH=%date:~4,2%
set DAY=%date:~7,2%
set HOUR=%time:~0,2%
if "%HOUR:~0,1%" == " " set HOUR=0%HOUR:~1,1%
set MINUTE=%time:~3,2%
set SECOND=%time:~6,2%
set MICROSEC=%time:~9,2%

set PDB_FILE_NAME=reflect_%YEAR%%MONTH%%DAY%-%HOUR%%MINUTE%%SECOND%%MICROSEC%.pdb

:: -EHa-
set CommonCompilerFlags=-MT -nologo -Gm- -GR- /EHsc -Od -Oi  -WX -W4 -wd4201 -wd4100 -wd4189 -wd4101 /I%Pincludes% -Zi -FC /std:c++17 -DREFLECT_WIN32 -D%BUILD_VERSION%


set CommonLinkerFlags=/incremental:no /opt:ref /MAP


IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\build

del *.pdb > NUL 2> NUL

cl %CommonCompilerFlags% ..\code\reflect.cpp /LD /link %CommonLinkerFlags% /PDB:%PDB_FILE_NAME% /EXPORT:gameUpdateAndRender


SETLOCAL EnableExtensions
set EXE=sdl_reflect.exe

FOR /F %%x IN ('tasklist /NH /FI "IMAGENAME eq %EXE%"') DO IF %%x == %EXE% goto ProcessFound

goto ProcessNotFound

:ProcessFound

echo %EXE% is running
goto END

:ProcessNotFound

cl %CommonCompilerFlags% ..\code\sdl_reflect.cpp gl.c /link %CommonLinkerFlags% /LIBPATH:%Plibs% SDL2.lib SDL2main.lib opengl32.lib freetype.lib user32.lib

goto END



:END

echo Done

popd
