@echo off
setlocal EnableDelayedExpansion

set SCRIPT_DIR=%~dp0
set ROOT=%SCRIPT_DIR%..
set TOOLS=%ROOT%\tools
set APKTOOL=%TOOLS%\apktool.jar
set BAKSMALI=%TOOLS%\baksmali.jar
set JAVA_SRC=%ROOT%\android\app\src\main\java\com\mod\loader\ModLoader.java
set PATCHES=%ROOT%\patches
set WORK=%ROOT%\build\modloader

set ANDROID_HOME=%LOCALAPPDATA%\Android\Sdk
set ANDROID_JAR=%ANDROID_HOME%\platforms\android-33\android.jar
set BUILD_TOOLS=%ANDROID_HOME%\build-tools\35.0.0
if not exist "%BUILD_TOOLS%\d8.bat" set BUILD_TOOLS=%ANDROID_HOME%\build-tools\30.0.3
if not exist "%BUILD_TOOLS%\d8.bat" (
    echo [build_modloader] d8 not found in Android SDK build-tools
    exit /b 1
)
if not exist "%ANDROID_JAR%" (
    echo [build_modloader] android.jar not found at %ANDROID_JAR%
    exit /b 1
)
if not exist "%BAKSMALI%" (
    echo [build_modloader] baksmali.jar missing in %TOOLS%
    exit /b 1
)
if not exist "%JAVA_SRC%" (
    echo [build_modloader] source not found: %JAVA_SRC%
    exit /b 1
)

if exist "%WORK%" rmdir /s /q "%WORK%"
mkdir "%WORK%\classes"
mkdir "%WORK%\smali"
mkdir "%PATCHES%" 2>nul

echo [build_modloader] javac ...
javac -source 8 -target 8 -bootclasspath "%ANDROID_JAR%" -d "%WORK%\classes" "%JAVA_SRC%"
if errorlevel 1 (
    echo [build_modloader] javac failed
    exit /b 1
)

echo [build_modloader] d8 ...
pushd "%WORK%\classes"
call "%BUILD_TOOLS%\d8.bat" --output "%WORK%" --lib "%ANDROID_JAR%" com\mod\loader\*.class
set D8_EC=%ERRORLEVEL%
popd
if not "%D8_EC%"=="0" (
    echo [build_modloader] d8 failed, code %D8_EC%
    exit /b %D8_EC%
)

echo [build_modloader] baksmali ...
java -jar "%BAKSMALI%" d "%WORK%\classes.dex" -o "%WORK%\smali"
if errorlevel 1 (
    echo [build_modloader] baksmali failed
    exit /b 1
)

echo [build_modloader] copying smali to patches\ ...
del /q "%PATCHES%\ModLoader*.smali" 2>nul
copy /Y "%WORK%\smali\com\mod\loader\ModLoader*.smali" "%PATCHES%\" >nul
if errorlevel 1 (
    echo [build_modloader] copy failed
    exit /b 1
)

dir /B "%PATCHES%\ModLoader*.smali"

echo ========================================
echo [build_modloader] OK -> %PATCHES%
echo ========================================
exit /b 0
