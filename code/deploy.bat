@echo off

call "build.bat"

mkdir ..\deploy
copy ..\build\win32_reflect.exe ..\deploy
xcopy /s /y /q ..\data ..\deploy
