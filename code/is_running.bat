@echo off
SETLOCAL EnableExtensions

set EXE=sdl_reflect.exe

FOR /F %%x IN ('tasklist /NH /FI "IMAGENAME eq %EXE%"') DO IF %%x == %EXE% goto ProcessFound

goto ProcessNotFound

:ProcessFound

echo %EXE% is running
goto END
:ProcessNotFound
echo %EXE% is not running
goto END
:END
echo Finished!
