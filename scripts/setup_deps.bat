@echo off
setlocal EnableDelayedExpansion

set SCRIPT_DIR=%~dp0
set ROOT=%SCRIPT_DIR%..
set THIRD_PARTY=%ROOT%\third_party
set IMGUI_DIR=%THIRD_PARTY%\imgui
set JSON_DIR=%THIRD_PARTY%\nlohmann

if not exist "%THIRD_PARTY%" mkdir "%THIRD_PARTY%"

if exist "%IMGUI_DIR%\imgui.cpp" (
    echo [setup_deps] imgui already present, skipping
) else (
    echo [setup_deps] cloning imgui v1.92.3 ...
    git clone --depth 1 --branch v1.92.3 https://github.com/ocornut/imgui.git "%IMGUI_DIR%"
    if errorlevel 1 goto :err
)

if exist "%JSON_DIR%\nlohmann\json.hpp" (
    echo [setup_deps] nlohmann/json already present, skipping
) else (
    echo [setup_deps] downloading nlohmann/json single header ...
    if not exist "%JSON_DIR%\nlohmann" mkdir "%JSON_DIR%\nlohmann"
    powershell -NoProfile -Command "Invoke-WebRequest https://raw.githubusercontent.com/nlohmann/json/v3.11.3/single_include/nlohmann/json.hpp -OutFile '%JSON_DIR%\nlohmann\json.hpp'"
    if errorlevel 1 goto :err
    copy /Y "%JSON_DIR%\nlohmann\json.hpp" "%JSON_DIR%\json.hpp" >nul
)

echo.
echo [setup_deps] OK
exit /b 0

:err
echo [setup_deps] FAILED
exit /b 1
