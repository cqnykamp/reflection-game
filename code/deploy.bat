@echo off

call "build.bat"

mkdir ..\deploy
copy ..\build\sdl_reflect.exe ..\deploy
xcopy /s /y /q ..\data ..\deploy
