@echo off
setlocal EnableDelayedExpansion

set SCRIPT_DIR=%~dp0
set ROOT=%SCRIPT_DIR%..
set IMGUI_DIR=%ROOT%\third_party\imgui
set SRC_DIR=%ROOT%\src
set OUTPUT_DIR=%SCRIPT_DIR%build

if not exist "%IMGUI_DIR%\imgui.cpp" (
    echo [compile] imgui missing. Run scripts\setup_deps.bat first.
    exit /b 1
)

set CXX=
set WINDRES=
if exist "%ROOT%\tools\mingw64\bin\g++.exe" (
    set "CXX=%ROOT%\tools\mingw64\bin\g++.exe"
    set "WINDRES=%ROOT%\tools\mingw64\bin\windres.exe"
) else (
    where g++.exe >nul 2>&1
    if !ERRORLEVEL! EQU 0 (
        set CXX=g++.exe
        set WINDRES=windres.exe
    )
)

if "%CXX%"=="" (
    echo [compile] MinGW g++ not found. Run scripts\fetch_toolchain.bat first
    echo           or install MinGW-w64 with g++.exe on PATH.
    exit /b 1
)

if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

pushd "%SCRIPT_DIR%"
"%WINDRES%" -J rc -O coff -i app.rc -o "%OUTPUT_DIR%\app.res.o"
set WINDRES_EC=%ERRORLEVEL%
popd
if not "%WINDRES_EC%"=="0" (
    echo [compile] windres failed on app.rc, exit code %WINDRES_EC%
    exit /b 1
)

set SOURCES=
for /R "%SRC_DIR%\ui" %%f in (*.cpp) do set SOURCES=!SOURCES! "%%f"
set SOURCES=!SOURCES! "%SRC_DIR%\core\runtime\runtime_state.cpp"

"%CXX%" -std=c++20 -O3 -DWIN32_LEAN_AND_MEAN ^
    -static -static-libgcc -static-libstdc++ ^
    -I"%IMGUI_DIR%" -I"%IMGUI_DIR%\backends" ^
    -I"%SRC_DIR%" -I"%SRC_DIR%\include" ^
    -o "%OUTPUT_DIR%\mimi_mod_desktop.exe" ^
    "%SCRIPT_DIR%main.cpp" ^
    !SOURCES! ^
    "%IMGUI_DIR%\imgui.cpp" "%IMGUI_DIR%\imgui_draw.cpp" ^
    "%IMGUI_DIR%\imgui_tables.cpp" "%IMGUI_DIR%\imgui_widgets.cpp" ^
    "%IMGUI_DIR%\backends\imgui_impl_win32.cpp" ^
    "%IMGUI_DIR%\backends\imgui_impl_opengl3.cpp" ^
    "%OUTPUT_DIR%\app.res.o" ^
    -lopengl32 -lgdi32 -ldwmapi -lwinmm -mwindows

if errorlevel 1 (
    echo.
    echo [compile] BUILD FAILED
    exit /b 1
)

echo.
echo ========================================
echo [compile] BUILD OK
echo Run: %OUTPUT_DIR%\mimi_mod_desktop.exe
echo ========================================
exit /b 0
