@echo off

IF NOT EXIST exported mkdir exported

pushd code
call "build.bat"
popd

xcopy /s /q /y data exported
copy build\reflect.exe exported


