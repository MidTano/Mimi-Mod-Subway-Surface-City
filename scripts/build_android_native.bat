@echo off
setlocal EnableDelayedExpansion

set SCRIPT_DIR=%~dp0
set ROOT=%SCRIPT_DIR%..

if defined ANDROID_NDK_HOME (
    set ANDROID_NDK=%ANDROID_NDK_HOME%
) else if defined ANDROID_NDK_ROOT (
    set ANDROID_NDK=%ANDROID_NDK_ROOT%
) else (
    set ANDROID_NDK=%LOCALAPPDATA%\Android\Sdk\ndk\25.1.8937393
)

where cmake >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    set CMAKE=cmake
) else (
    set CMAKE=%LOCALAPPDATA%\Android\Sdk\cmake\3.22.1\bin\cmake.exe
)

where ninja >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    set NINJA=ninja
) else (
    set NINJA=%LOCALAPPDATA%\Android\Sdk\cmake\3.22.1\bin\ninja.exe
)

if exist "%ANDROID_NDK%\build\cmake\android.toolchain.cmake" goto :ndk_ok
for /d %%D in ("%ANDROID_NDK%\*") do (
    if exist "%%D\build\cmake\android.toolchain.cmake" (
        set ANDROID_NDK=%%D
        goto :ndk_ok
    )
)
echo [build_android_native] NDK not found at %ANDROID_NDK%
echo [build_android_native] Set ANDROID_NDK_HOME or install NDK 25.1.8937393 via SDK Manager
exit /b 1
:ndk_ok
"%CMAKE%" --version >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [build_android_native] cmake not found. Install CMake or add it to PATH.
    exit /b 1
)
"%NINJA%" --version >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [build_android_native] ninja not found. Install Ninja or add it to PATH.
    exit /b 1
)

call "%SCRIPT_DIR%setup_deps.bat" || exit /b 1

powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%gen_build_stamp.ps1" || exit /b 1

set BUILD_ROOT=%ROOT%\build\android

for %%A in (arm64-v8a armeabi-v7a) do (
    set ABI=%%A
    set BDIR=%BUILD_ROOT%\!ABI!
    if not exist "!BDIR!" mkdir "!BDIR!"

    echo ========================================
    echo [build_android_native] configuring !ABI!
    echo ========================================

    "%CMAKE%" -S "%ROOT%\android\jni" -B "!BDIR!" -G Ninja ^
        -DCMAKE_MAKE_PROGRAM="%NINJA%" ^
        -DCMAKE_TOOLCHAIN_FILE="%ANDROID_NDK%\build\cmake\android.toolchain.cmake" ^
        -DANDROID_ABI=!ABI! ^
        -DANDROID_PLATFORM=android-21 ^
        -DCMAKE_BUILD_TYPE=Release || exit /b 1

    "%CMAKE%" --build "!BDIR!" || exit /b 1

    set OUT_DIR=%ROOT%\build\libs\!ABI!
    if not exist "!OUT_DIR!" mkdir "!OUT_DIR!"
    if exist "!OUT_DIR!\libsubway_mod.so" del /f /q "!OUT_DIR!\libsubway_mod.so"
    copy /Y "!BDIR!\libsubway_mod.so" "!OUT_DIR!\libsubway_mod.so" >nul || exit /b 1
    echo [build_android_native] !ABI! libsubway_mod.so -> !OUT_DIR!
)

echo.
echo ========================================
echo [build_android_native] OK
echo ========================================
exit /b 0
