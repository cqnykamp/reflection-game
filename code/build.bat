
@echo off

SETLOCAL

set Pincludes=w:\reflect\build\include
set Plibs=w:\reflect\build\lib


:: -EHa-
set CommonCompilerFlags=-MT -nologo -Gm- -GR- /EHsc -Od -Oi -Z7 -WX -W4 -wd4201 -wd4100 -wd4189 -wd4101 /I%Pincludes% -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_WIN32=1 -FC

set CommonLinkerFlags=/incremental:no /opt:ref /LIBPATH:%Plibs% SDL2.lib SDL2main.lib opengl32.lib 


IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\build


::cl /Zi /EHsc /MD /Iinclude ..\code\reflect.cpp gdi32.lib opengl32.lib kernel32.lib user32.lib shell32.lib glfw-3.3.2.bin.WIN64\lib-vc2019\glfw3.lib gl.c

cl %CommonCompilerFlags% ..\code\sdltest.cpp gl.c /link %CommonLinkerFlags%

::cl  -Z7 -Fmwin32_handmade.map /std:c++17 ..\handmade\code\win32_handmade.cpp /link -opt:ref  user32.lib gdi32.lib


popd
