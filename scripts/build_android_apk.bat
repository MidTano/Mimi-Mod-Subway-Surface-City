@echo off
setlocal EnableDelayedExpansion

set SCRIPT_DIR=%~dp0
set ROOT=%SCRIPT_DIR%..
set TOOLS=%ROOT%\tools

if defined MOD_TEMPLATE_APK (
    set TEMPLATE_APK=%MOD_TEMPLATE_APK%
) else (
    set TEMPLATE_APK=%ROOT%\build\base_template.apk
)
set KEYSTORE=%ROOT%\android\keystore\mimi_mod.keystore
set KEY_ALIAS=mimi_mod_key
if not defined MOD_STORE_PASS (
    echo [build_android_apk] set MOD_STORE_PASS and MOD_KEY_PASS environment variables
    exit /b 1
)
set STORE_PASS=%MOD_STORE_PASS%
set KEY_PASS=%MOD_KEY_PASS%

set ANDROID_HOME=%LOCALAPPDATA%\Android\Sdk
set BUILD_TOOLS=%ANDROID_HOME%\build-tools\35.0.0
if not exist "%BUILD_TOOLS%\zipalign.exe" set BUILD_TOOLS=%ANDROID_HOME%\build-tools\30.0.3
if not exist "%BUILD_TOOLS%\zipalign.exe" (
    echo [build_android_apk] Android build-tools not found in %ANDROID_HOME%\build-tools
    exit /b 1
)

if not exist "%TEMPLATE_APK%" (
    echo [build_android_apk] template APK not found at %TEMPLATE_APK%
    exit /b 1
)

set APKTOOL=%TOOLS%\apktool.jar
if not exist "%APKTOOL%" (
    echo [build_android_apk] apktool.jar not found at %APKTOOL%
    exit /b 1
)

set LIB64=%ROOT%\build\libs\arm64-v8a\libsubway_mod.so
set LIB32=%ROOT%\build\libs\armeabi-v7a\libsubway_mod.so
if not exist "%LIB64%" (
    echo [build_android_apk] missing %LIB64%. Run scripts\build_android_native.bat
    exit /b 1
)
if not exist "%LIB32%" (
    echo [build_android_apk] missing %LIB32%. Run scripts\build_android_native.bat
    exit /b 1
)

set SPLIT64=%ROOT%\build\split_config.arm64_v8a.apk
set SPLIT32=%ROOT%\build\split_config.armeabi_v7a.apk
if not exist "%SPLIT64%" (
    echo [build_android_apk] missing %SPLIT64% - copy split APKs from original game
    exit /b 1
)
if not exist "%SPLIT32%" (
    echo [build_android_apk] missing %SPLIT32% - copy split APKs from original game
    exit /b 1
)

set OUT_DIR=%ROOT%\build
set DECODED=%OUT_DIR%\decoded
set REBUILT=%OUT_DIR%\subway_mod_unsigned.apk
set ALIGNED_APK=%OUT_DIR%\subway_mod_aligned.apk
set SIGNED_APK=%OUT_DIR%\subway_mod_signed.apk
set PATCHES=%ROOT%\patches

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

echo ========================================
echo [build_android_apk] step 1: build ModLoader smali
echo ========================================
call "%SCRIPT_DIR%build_modloader.bat"
if errorlevel 1 (
    echo [build_android_apk] build_modloader.bat failed
    exit /b 1
)

if not exist "%PATCHES%\ModLoader.smali" (
    echo [build_android_apk] missing %PATCHES%\ModLoader.smali
    exit /b 1
)

echo ========================================
echo [build_android_apk] step 2: decode template APK ^(if needed^)
echo ========================================
if not exist "%DECODED%\apktool.yml" (
    if exist "%DECODED%" cmd /c rmdir /s /q "%DECODED%"
    echo [build_android_apk] decoding %TEMPLATE_APK% ...
    java -Xmx2G -jar "%APKTOOL%" d -f --only-main-classes "%TEMPLATE_APK%" -o "%DECODED%"
    if errorlevel 1 (
        echo [build_android_apk] apktool d failed
        exit /b 1
    )
) else (
    echo [build_android_apk] using cached decoded folder %DECODED%
)

echo ========================================
echo [build_android_apk] step 2.5: remove split requirements from manifest
echo ========================================
powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%patch_manifest.ps1" -ManifestPath "%DECODED%\AndroidManifest.xml"
if errorlevel 1 (
    echo [build_android_apk] manifest patch failed
    exit /b 1
)

echo ========================================
echo [build_android_apk] step 3: stage patches and libraries
echo ========================================

set MODLOADER_DIR=
for %%D in ("%DECODED%\smali" "%DECODED%\smali_classes2" "%DECODED%\smali_classes3" "%DECODED%\smali_classes4" "%DECODED%\smali_classes5") do (
    if exist "%%~D\com\mod\loader\ModLoader.smali" set MODLOADER_DIR=%%~D\com\mod\loader
)
if "%MODLOADER_DIR%"=="" (
    echo [build_android_apk] ModLoader.smali not found in decoded smali; defaulting to smali\com\mod\loader
    set MODLOADER_DIR=%DECODED%\smali\com\mod\loader
    if not exist "!MODLOADER_DIR!" mkdir "!MODLOADER_DIR!"
)
echo [build_android_apk] target ModLoader dir: !MODLOADER_DIR!

del /q "!MODLOADER_DIR!\ModLoader*.smali" 2>nul
copy /Y "%PATCHES%\ModLoader*.smali" "!MODLOADER_DIR!\" >nul
if errorlevel 1 (
    echo [build_android_apk] copy smali failed
    exit /b 1
)
echo [build_android_apk] copied:
dir /B "!MODLOADER_DIR!\ModLoader*.smali"

echo [build_android_apk] extracting native libraries from split APKs ...
powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%extract_split_libs.ps1" -SplitApk "%SPLIT64%" -OutputDir "%DECODED%"
if errorlevel 1 (
    echo [build_android_apk] failed to extract arm64-v8a libs
    exit /b 1
)
powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%extract_split_libs.ps1" -SplitApk "%SPLIT32%" -OutputDir "%DECODED%"
if errorlevel 1 (
    echo [build_android_apk] failed to extract armeabi-v7a libs
    exit /b 1
)

echo [build_android_apk] copying mod library ...
copy /Y "%LIB64%" "%DECODED%\lib\arm64-v8a\libsubway_mod.so" >nul
copy /Y "%LIB32%" "%DECODED%\lib\armeabi-v7a\libsubway_mod.so" >nul

echo ========================================
echo [build_android_apk] step 4: rebuild APK
echo ========================================
if exist "%REBUILT%" del /q "%REBUILT%"
java -Xmx2G -jar "%APKTOOL%" b "%DECODED%" -o "%REBUILT%" --use-aapt2
if errorlevel 1 (
    echo [build_android_apk] apktool b failed
    exit /b 1
)

echo ========================================
echo [build_android_apk] step 5: zipalign + sign
echo ========================================
"%BUILD_TOOLS%\zipalign.exe" -f -p 4 "%REBUILT%" "%ALIGNED_APK%" || exit /b 1

if exist "%BUILD_TOOLS%\apksigner.bat" (
    call "%BUILD_TOOLS%\apksigner.bat" sign ^
        --ks "%KEYSTORE%" ^
        --ks-pass pass:%STORE_PASS% ^
        --key-pass pass:%KEY_PASS% ^
        --ks-key-alias %KEY_ALIAS% ^
        --out "%SIGNED_APK%" ^
        "%ALIGNED_APK%" || exit /b 1
) else (
    echo [build_android_apk] apksigner.bat not found in %BUILD_TOOLS%
    exit /b 1
)

echo.
echo ========================================
echo [build_android_apk] APK: %SIGNED_APK%
echo ========================================
exit /b 0
