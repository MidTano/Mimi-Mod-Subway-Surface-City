@echo off
setlocal EnableDelayedExpansion
set SCRIPT_DIR=%~dp0
call "%SCRIPT_DIR%setup_deps.bat" || exit /b 1
call "%SCRIPT_DIR%fetch_toolchain.bat" || exit /b 1
call "%SCRIPT_DIR%..\desktop\compile.bat" || exit /b 1
exit /b 0
