@echo off
setlocal EnableDelayedExpansion

set SCRIPT_DIR=%~dp0
set ROOT=%SCRIPT_DIR%..
set PACKAGE=com.sybogames.subway.surfers.game
set SIGNED_APK=%ROOT%\build\subway_mod_signed.apk
set KEYSTORE=%ROOT%\android\keystore\mimi_mod.keystore
set KEY_ALIAS=mimi_mod_key
set TOOLS=%ROOT%\tools

echo ========================================
echo  MIMI MOD - FULL BUILD
echo ========================================
echo.

if exist "%SCRIPT_DIR%local.env.bat" (
    call "%SCRIPT_DIR%local.env.bat"
)

echo [check] checking prerequisites ...
echo.
set CHECKS_OK=1

where git >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    for /f "tokens=3" %%V in ('git --version') do echo [  OK  ] git %%V
) else (
    echo [ FAIL ] git not found
    echo          install: https://git-scm.com/downloads
    set CHECKS_OK=0
)

where powershell >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    for /f "tokens=*" %%V in ('powershell -NoProfile -Command "$PSVersionTable.PSVersion.ToString()"') do echo [  OK  ] PowerShell %%V
) else (
    echo [ FAIL ] PowerShell not found
    set CHECKS_OK=0
)

set JDK_OK=0
set JDK_VER=0
if defined JAVA_HOME (
    if exist "%JAVA_HOME%\bin\javac.exe" (
        for /f "tokens=2" %%V in ('"%JAVA_HOME%\bin\javac.exe" -version 2^>^&1') do (
            set JDK_OK=1
            set JDK_VER_STR=%%V
            for /f "tokens=1 delims=." %%M in ("%%V") do set JDK_VER=%%M
        )
    )
)
if "!JDK_OK!"=="0" (
    where javac >nul 2>&1
    if !ERRORLEVEL! EQU 0 (
        for /f "tokens=2" %%V in ('javac -version 2^>^&1') do (
            set JDK_OK=1
            set JDK_VER_STR=%%V
            for /f "tokens=1 delims=." %%M in ("%%V") do set JDK_VER=%%M
        )
    )
)
if !JDK_VER! LSS 17 (
    set JDK_OK=0
    set JDK_VER=0
    for %%P in (
        "%ProgramFiles%\Eclipse Adoptium"
        "%ProgramFiles%\Java"
        "%ProgramFiles%\Microsoft"
        "%ProgramFiles%\AdoptOpenJDK"
        "%ProgramFiles%\Zulu"
    ) do (
        if !JDK_OK! EQU 0 (
            for /d %%D in ("%%~P\*") do (
                if !JDK_OK! EQU 0 (
                    if exist "%%~D\bin\javac.exe" (
                        for /f "tokens=2" %%V in ('"%%~D\bin\javac.exe" -version 2^>^&1') do (
                            set _TMP_VER=0
                            for /f "tokens=1 delims=." %%M in ("%%V") do set _TMP_VER=%%M
                            if !_TMP_VER! GEQ 17 (
                                set "JAVA_HOME=%%~D"
                                set JDK_OK=1
                                set JDK_VER_STR=%%V
                                set JDK_VER=!_TMP_VER!
                            )
                        )
                    )
                )
            )
        )
    )
)
if "!JDK_OK!"=="0" (
    echo [ FAIL ] JDK 17+ not found
    echo          install: https://adoptium.net/temurin/releases/?version=17
    echo          or set JAVA_HOME to an existing JDK 17+ root
    set CHECKS_OK=0
) else (
    echo [  OK  ] JDK !JDK_VER_STR! at !JAVA_HOME!
    set "PATH=!JAVA_HOME!\bin;!PATH!"
)

set ANDROID_HOME_DIR=%LOCALAPPDATA%\Android\Sdk
if defined ANDROID_HOME set ANDROID_HOME_DIR=%ANDROID_HOME%
if exist "%ANDROID_HOME_DIR%\platforms" (
    echo [  OK  ] Android SDK at %ANDROID_HOME_DIR%
) else (
    echo [ FAIL ] Android SDK not found at %ANDROID_HOME_DIR%
    echo          install Android Studio: https://developer.android.com/studio
    echo          or set ANDROID_HOME to your SDK path
    set CHECKS_OK=0
)

set NDK_DIR=
if defined ANDROID_NDK_HOME (
    set NDK_DIR=%ANDROID_NDK_HOME%
) else if defined ANDROID_NDK_ROOT (
    set NDK_DIR=%ANDROID_NDK_ROOT%
) else (
    set NDK_DIR=%ANDROID_HOME_DIR%\ndk\25.1.8937393
)
if not exist "!NDK_DIR!\build\cmake\android.toolchain.cmake" (
    for /d %%D in ("!NDK_DIR!\*") do (
        if exist "%%~D\build\cmake\android.toolchain.cmake" set "NDK_DIR=%%~D"
    )
)
if exist "!NDK_DIR!\build\cmake\android.toolchain.cmake" (
    echo [  OK  ] Android NDK at !NDK_DIR!
) else (
    echo [ FAIL ] Android NDK 25.1.8937393 not found
    echo          install via Android Studio SDK Manager or:
    echo          sdkmanager "ndk;25.1.8937393"
    set CHECKS_OK=0
)

set CMAKE_BIN=
where cmake >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    set CMAKE_BIN=cmake
) else if exist "%ANDROID_HOME_DIR%\cmake\3.22.1\bin\cmake.exe" (
    set "CMAKE_BIN=%ANDROID_HOME_DIR%\cmake\3.22.1\bin\cmake.exe"
) else if exist "%ANDROID_HOME_DIR%\cmake\3.18.1\bin\cmake.exe" (
    set "CMAKE_BIN=%ANDROID_HOME_DIR%\cmake\3.18.1\bin\cmake.exe"
)
if defined CMAKE_BIN (
    echo [  OK  ] CMake found
) else (
    echo [ FAIL ] CMake 3.18+ not found
    echo          install via SDK Manager: sdkmanager "cmake;3.22.1"
    echo          or https://cmake.org/download/
    set CHECKS_OK=0
)

set NINJA_BIN=
where ninja >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    set NINJA_BIN=ninja
) else if exist "%ANDROID_HOME_DIR%\cmake\3.22.1\bin\ninja.exe" (
    set "NINJA_BIN=%ANDROID_HOME_DIR%\cmake\3.22.1\bin\ninja.exe"
) else if exist "%ANDROID_HOME_DIR%\cmake\3.18.1\bin\ninja.exe" (
    set "NINJA_BIN=%ANDROID_HOME_DIR%\cmake\3.18.1\bin\ninja.exe"
)
if defined NINJA_BIN (
    echo [  OK  ] Ninja found
) else (
    echo [ FAIL ] Ninja not found
    echo          install via SDK Manager: sdkmanager "cmake;3.22.1"
    echo          or https://ninja-build.org/
    set CHECKS_OK=0
)

set BUILD_TOOLS_DIR=
for %%V in (35.0.0 34.0.0 33.0.2 33.0.1 33.0.0 30.0.3) do (
    if not defined BUILD_TOOLS_DIR (
        if exist "%ANDROID_HOME_DIR%\build-tools\%%V\zipalign.exe" (
            set "BUILD_TOOLS_DIR=%ANDROID_HOME_DIR%\build-tools\%%V"
        )
    )
)
if defined BUILD_TOOLS_DIR (
    echo [  OK  ] Android build-tools at !BUILD_TOOLS_DIR!
) else (
    echo [ FAIL ] Android build-tools not found, need zipalign + apksigner
    echo          install: sdkmanager "build-tools;35.0.0"
    set CHECKS_OK=0
)

if exist "%TOOLS%\apktool.jar" (
    echo [  OK  ] apktool.jar
) else (
    echo [ WARN ] apktool.jar not found in tools\
    echo          download: https://github.com/iBotPeaches/Apktool/releases
    echo          place as: tools\apktool.jar
    echo          (required for APK patching step)
)

if exist "%TOOLS%\baksmali.jar" (
    echo [  OK  ] baksmali.jar
) else (
    echo [ WARN ] baksmali.jar not found in tools\
    echo          download: https://github.com/JesusFreke/smali/releases
    echo          place as: tools\baksmali.jar
    echo          (required for ModLoader build step)
)

set TEMPLATE_APK=%ROOT%\build\base_template.apk
if defined MOD_TEMPLATE_APK set TEMPLATE_APK=%MOD_TEMPLATE_APK%
if exist "%TEMPLATE_APK%" (
    echo [  OK  ] template APK at %TEMPLATE_APK%
) else (
    echo [ WARN ] template APK not found at %TEMPLATE_APK%
    echo          place the original Subway Surfers 3.41.0 base APK there
    echo          or set MOD_TEMPLATE_APK env var to its path
    echo          (required for APK patching step)
)

echo.

if "%CHECKS_OK%"=="0" (
    echo ========================================
    echo  PREREQUISITE CHECK FAILED
    echo  Fix the items marked [ FAIL ] above
    echo ========================================
    exit /b 1
)

if not defined MOD_STORE_PASS (
    echo [ FAIL ] MOD_STORE_PASS not set.
    echo.
    echo          Option 1: copy scripts\local.env.bat.example to scripts\local.env.bat
    echo                    and set your passwords there.
    echo.
    echo          Option 2: set environment variables:
    echo                    set MOD_STORE_PASS=your_password
    echo                    set MOD_KEY_PASS=your_password
    echo.
    goto :fail
)
if not defined MOD_KEY_PASS set MOD_KEY_PASS=%MOD_STORE_PASS%

if not exist "%KEYSTORE%" (
    echo [build] generating keystore at %KEYSTORE% ...
    if not exist "%ROOT%\android\keystore" mkdir "%ROOT%\android\keystore"
    keytool -genkeypair -v ^
        -keystore "%KEYSTORE%" ^
        -alias %KEY_ALIAS% ^
        -keyalg RSA -keysize 2048 -validity 36500 ^
        -storepass %MOD_STORE_PASS% ^
        -keypass %MOD_KEY_PASS% ^
        -dname "CN=MIMI MOD, OU=Mod, O=MimiMod, L=Unknown, ST=Unknown, C=US" || goto :fail
    echo [build] keystore created
)

echo ========================================
echo  step 1/5: dependencies
echo ========================================
call "%SCRIPT_DIR%setup_deps.bat" || goto :fail
powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%gen_build_stamp.ps1" || goto :fail

echo.
echo ========================================
echo  step 2/5: desktop viewer
echo ========================================
call "%SCRIPT_DIR%fetch_toolchain.bat" || goto :fail
call "%ROOT%\desktop\compile.bat" || goto :fail

echo.
echo ========================================
echo  step 3/5: native libs (arm64-v8a + armeabi-v7a)
echo ========================================
call "%SCRIPT_DIR%build_android_native.bat" || goto :fail

echo.
echo ========================================
echo  step 4/5: APK (decode, patch, rebuild, sign)
echo ========================================
call "%SCRIPT_DIR%build_android_apk.bat" || goto :fail

if not exist "%SIGNED_APK%" (
    echo [build] signed APK not found at %SIGNED_APK%
    goto :fail
)

echo.
echo ========================================
echo  step 5/5: deploy to device
echo ========================================

set ADB=adb
where adb >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    set "ADB=%ANDROID_HOME_DIR%\platform-tools\adb.exe"
)

"%ADB%" version >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [deploy] adb not found, skipping deploy
    echo          install: sdkmanager "platform-tools"
    echo          APK ready at: %SIGNED_APK%
    echo          install manually: adb install -r "%SIGNED_APK%"
    goto :done_no_deploy
)

"%ADB%" devices | findstr /R /C:"device$" >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [deploy] no device connected, skipping deploy
    echo          connect a device with USB debugging enabled
    echo          APK ready at: %SIGNED_APK%
    echo          install manually: adb install -r "%SIGNED_APK%"
    goto :done_no_deploy
)

echo [deploy] uninstalling %PACKAGE% ...
"%ADB%" uninstall %PACKAGE% 2>nul
if %ERRORLEVEL% EQU 0 (
    echo [deploy] old APK removed
) else (
    echo [deploy] package was not installed, skipping
)

echo [deploy] installing %SIGNED_APK% ...
"%ADB%" install -r "%SIGNED_APK%"
if errorlevel 1 (
    echo [deploy] adb install failed
    goto :fail
)

echo.
echo ========================================
echo  BUILD + DEPLOY OK
echo  Desktop:  %ROOT%\desktop\build\mimi_mod_desktop.exe
echo  APK:      %SIGNED_APK%
echo  Package:  %PACKAGE%
echo ========================================
exit /b 0

:done_no_deploy
echo.
echo ========================================
echo  BUILD OK (deploy skipped)
echo  Desktop:  %ROOT%\desktop\build\mimi_mod_desktop.exe
echo  APK:      %SIGNED_APK%
echo ========================================
exit /b 0

:fail
echo.
echo ========================================
echo  BUILD FAILED
echo ========================================
exit /b 1
