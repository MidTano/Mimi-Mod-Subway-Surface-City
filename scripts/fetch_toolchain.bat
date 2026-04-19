@echo off
setlocal EnableDelayedExpansion

set SCRIPT_DIR=%~dp0
set ROOT=%SCRIPT_DIR%..
set TOOLS=%ROOT%\tools
set MINGW_DIR=%TOOLS%\mingw64

if exist "%MINGW_DIR%\bin\g++.exe" (
    echo [fetch_toolchain] MinGW already present at %MINGW_DIR%
    exit /b 0
)

if not exist "%TOOLS%" mkdir "%TOOLS%"

where g++.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [fetch_toolchain] g++ found on PATH, skipping download
    exit /b 0
)

echo [fetch_toolchain] downloading MinGW-w64 (WinLibs 13.2.0 ucrt) ...
set ZIP=%TEMP%\mingw64.zip
set URL=https://github.com/brechtsanders/winlibs_mingw/releases/download/13.2.0posix-17.0.6-11.0.1-ucrt-r5/winlibs-x86_64-posix-seh-gcc-13.2.0-mingw-w64ucrt-11.0.1-r5.zip
powershell -NoProfile -Command "Invoke-WebRequest -UseBasicParsing '%URL%' -OutFile '%ZIP%'"
if errorlevel 1 goto :err

powershell -NoProfile -Command "Expand-Archive -LiteralPath '%ZIP%' -DestinationPath '%TOOLS%' -Force"
if errorlevel 1 goto :err

del "%ZIP%"

if exist "%MINGW_DIR%\bin\g++.exe" (
    echo [fetch_toolchain] OK
    exit /b 0
)

:err
echo [fetch_toolchain] FAILED. Please install MinGW-w64 manually to %MINGW_DIR%.
exit /b 1
