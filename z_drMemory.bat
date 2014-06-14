@echo off
set "DRMEM=D:\Dr. Memory"

:: -----------------------------------------------------------------

set "PATH=%QTDIR%\bin;%PATH%"

pushd "%~dp0\bin\Win32\Debug"
"%~dp0\etc\inifile.exe" .\vld.ini [Options] VLD=off

"%DRMEM%\bin\drmemory.exe" -suppress "%~dp0\z_drMemory.txt" -- ".\DoubleFileScanner.exe"

"%~dp0\etc\inifile.exe" .\vld.ini [Options] VLD=on
popd

:: -----------------------------------------------------------------

echo.
pause
