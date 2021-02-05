
@echo off

mkdir ..\build
pushd ..\build

cl /Zi /EHsc /MD /Iinclude ..\code\reflect.cpp gdi32.lib opengl32.lib kernel32.lib user32.lib shell32.lib glfw-3.3.2.bin.WIN64\lib-vc2019\glfw3.lib gl.c
::cl -Zi /Iinclude ..\code\win32_reflect.cpp gl.c
popd
