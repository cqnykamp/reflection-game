
@echo off

SETLOCAL

set Pincludes=w:\reflect\build\include
set Plibs=w:\reflect\build\lib


:: -EHa-
set CommonCompilerFlags=-MT -nologo -Gm- -GR- /EHsc -Od -Oi  -WX -W4 -wd4201 -wd4100 -wd4189 -wd4101 /I%Pincludes%  -FC -Zi /std:c++17 -DREFLECT_WIN32

set CommonLinkerFlags=/incremental:no /opt:ref /LIBPATH:%Plibs% SDL2.lib SDL2main.lib opengl32.lib freetype.lib


IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\build

cl %CommonCompilerFlags% ..\code\reflect.cpp /LD /link /MAP /EXPORT:gameUpdateAndRender
cl %CommonCompilerFlags% ..\code\sdl_reflect.cpp gl.c /link /MAP %CommonLinkerFlags%




popd
