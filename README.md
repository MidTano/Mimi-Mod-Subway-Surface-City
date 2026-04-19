# Mimi Mod - Subway Surfers

ImGui mod menu for Subway Surfers on Android.
Includes a Windows desktop viewer for UI iteration without rebuilding the APK.

## Preview

<div align="center">

[![Video Demo](https://img.shields.io/badge/Video_Demo-Watch_on_Imgur-blue?style=for-the-badge&logo=imgur)](https://imgur.com/a/gPqwJl3)

</div>

<div align="center">
<table>
<tr>
<td><img src="https://github.com/user-attachments/assets/7674c134-6aff-4a72-8e84-43e631b84fa8" width="270" /></td>
<td><img src="https://github.com/user-attachments/assets/3a4d667a-a13e-4957-b67e-402e902ad3ea" width="270" /></td>
<td><img src="https://github.com/user-attachments/assets/d4211249-b811-4855-9d06-c263969fa097" width="270" /></td>
</tr>
<tr>
<td><img src="https://github.com/user-attachments/assets/767bd88b-7e4d-45b1-b4ee-7bc9d271dcb0" width="270" /></td>
<td><img src="https://github.com/user-attachments/assets/de80885a-fb0b-4503-96dc-d7492988dd1a" width="270" /></td>
<td><img src="https://github.com/user-attachments/assets/6b039b13-d9ff-4b8b-8c88-df0466df36f9" width="270" /></td>
</tr>
</table>
</div>

## Compatibility

| | Version |
|---|---|
| **Game** | Subway Surfers **3.41.0** (tested, may work on nearby versions) |
| **Android** | 5.0+ (API 21), arm64-v8a / armeabi-v7a |
| **Desktop viewer** | Windows 10/11 x64 |

## Features

- Tab-based dashboard: MAIN, VISUALS, GAMEPLAY, POWER-UPS, SKINS
- Live telemetry: speed, distance, time, seed
- Toggles, sliders, segmented pills, color picker, navigation rows, text input numpad
- Floating Quick Action (FQA) dock with pulse, engaged and dimmed states
- Glitch overlay, animated top banner with hazard stripes and ticker
- HSV + RGB color picker modal with live preview, hex readout and preset palette
- Russian / English localization
- About modal with version info, build id and disclaimer

## Project structure

```
.
|-- src/
|   |-- ui/               UI layer (layout, widgets, fx, sections, state, fonts, i18n)
|   |-- core/             Native integration: hooks, il2cpp bridge, features, config
|   |-- include/          Public native headers
|   \-- jni_entry.cpp     JNI bridge for Android
|-- android/              Gradle + NDK project
|-- desktop/              Win32 + WGL viewer
|-- scripts/              Build scripts (see below)
|-- patches/              ModLoader smali patches
|-- docs/                 Architecture and build docs
|-- third_party/          imgui + nlohmann/json (auto-fetched, git-ignored)
\-- tools/                MinGW, apktool, baksmali (auto-fetched / manual, git-ignored)
```

## Tested dependency versions

These are the exact versions the project is built and tested with. If something breaks, roll back to these.

| Dependency | Version | Auto-fetched | Download |
|------------|---------|:---:|----------|
| **imgui** | v1.92.3 | yes | https://github.com/ocornut/imgui/tree/v1.92.3 |
| **nlohmann/json** | v3.11.3 | yes | https://github.com/nlohmann/json/releases/tag/v3.11.3 |
| **MinGW-w64** | 13.2.0 (ucrt, seh) | yes | https://github.com/brechtsanders/winlibs_mingw/releases |
| **Gradle** | 8.5 | yes (wrapper) | shipped in `android/gradle/` |
| **Android Gradle Plugin** | 8.0.2 | yes (Gradle) | -- |
| **JDK** | 17 | no | https://adoptium.net/temurin/releases/?version=17 |
| **Android SDK** | API 33 | no | https://developer.android.com/studio |
| **Android NDK** | 25.1.8937393 | no | `sdkmanager "ndk;25.1.8937393"` |
| **CMake** | 3.18.1+ | no | `sdkmanager "cmake;3.22.1"` or https://cmake.org/download/ |
| **Ninja** | any | no | bundled with SDK CMake or https://ninja-build.org/ |
| **Android build-tools** | 35.0.0 / 30.0.3 | no | `sdkmanager "build-tools;35.0.0"` |
| **apktool** | 2.9+ | no | https://github.com/iBotPeaches/Apktool/releases |
| **baksmali** | 2.5+ | no | https://github.com/JesusFreke/smali/releases |

## Quick start

### Option 1: Desktop viewer only (Windows)

Needs only **Git** and **PowerShell 5.1+**. Everything else is auto-fetched.

```powershell
scripts\build_desktop.bat
```

Output: `desktop\build\mimi_mod_desktop.exe` (statically linked, no extra DLLs needed).

### Option 2: Full build (Desktop + Android APK + Deploy)

#### Step 1 -- Install prerequisites

Install the tools marked "no" in the table above. The fastest path:

1. Install [JDK 17](https://adoptium.net/temurin/releases/?version=17) and set `JAVA_HOME`.
2. Install [Android Studio](https://developer.android.com/studio), then in SDK Manager install:
   - SDK Platform API 33
   - NDK 25.1.8937393
   - CMake 3.22.1
   - Build-Tools 35.0.0

#### Step 2 -- Place manual tools

Download and place these files into the `tools\` folder (create it if it does not exist):

```
tools\apktool.jar    <-- https://github.com/iBotPeaches/Apktool/releases (download the .jar)
tools\baksmali.jar   <-- https://github.com/JesusFreke/smali/releases (download baksmali-X.X.X.jar, rename to baksmali.jar)
```

#### Step 3 -- Place template APK

Copy the original Subway Surfers 3.41.0 **base APK** to:

```
build\base_template.apk
```

You also need the split APKs in `build\`:

```
build\split_config.arm64_v8a.apk
build\split_config.armeabi_v7a.apk
```

These can be extracted from the installed game using `adb` or a file manager on a rooted device.

#### Step 4 -- Set keystore passwords

Copy the example file and fill in your passwords:

```powershell
copy scripts\local.env.bat.example scripts\local.env.bat
```

Edit `scripts\local.env.bat`:

```bat
set MOD_STORE_PASS=your_password_here
set MOD_KEY_PASS=your_password_here
```

A keystore at `android\keystore\mimi_mod.keystore` is auto-generated on the first run.

#### Step 5 -- Build

```powershell
scripts\build_all.bat
```

The script will:
1. Check all prerequisites and report what is missing or found
2. Fetch imgui, nlohmann/json, MinGW (auto)
3. Build the desktop viewer
4. Build native .so for arm64-v8a and armeabi-v7a
5. Decode template APK, patch manifest, inject ModLoader + .so, rebuild and sign
6. Deploy to a connected device via adb (skipped if no device)

Output files:

```
desktop\build\mimi_mod_desktop.exe    <-- desktop viewer
build\subway_mod_signed.apk           <-- patched + signed APK
```

### Build scripts overview

| Script | Purpose |
|--------|---------|
| `scripts\build_desktop.bat` | Build desktop viewer (one command) |
| `scripts\build_all.bat` | Full pipeline: desktop + android + deploy |
| `scripts\setup_deps.bat` | Auto-fetch imgui and nlohmann/json into `third_party\` |
| `scripts\fetch_toolchain.bat` | Auto-fetch MinGW-w64 into `tools\mingw64\` |
| `scripts\build_android_native.bat` | Build native .so via CMake/NDK |
| `scripts\build_android_apk.bat` | Decode, patch, rebuild and sign APK |
| `scripts\build_modloader.bat` | Compile ModLoader.java to smali patches |

## Documentation

- [Architecture](docs/ARCHITECTURE.md) -- rendering model, layers, state, input
- [Building](docs/BUILDING.md) -- detailed build instructions and troubleshooting

## Legal

This mod is **unofficial** and is not affiliated with, endorsed by, or sponsored by SYBO Games ApS or Kiloo. Use at your own risk.
