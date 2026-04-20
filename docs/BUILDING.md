# Building

## Prerequisites

Desktop builds need only **Git** and **PowerShell 5.1+**. The compiler (MinGW-w64) and libraries (imgui, nlohmann/json) are auto-fetched.

Full Android builds additionally require:

| Tool | Tested version | Install |
|------|---------------|---------|
| JDK | 17 | https://adoptium.net/temurin/releases/?version=17 |
| Android SDK | API 33 | https://developer.android.com/studio |
| Android NDK | 25.1.8937393 | `sdkmanager "ndk;25.1.8937393"` |
| CMake | 3.18.1+ | `sdkmanager "cmake;3.22.1"` or https://cmake.org/download/ |
| Ninja | any | bundled with SDK CMake or https://ninja-build.org/ |
| Android build-tools | 35.0.0 | `sdkmanager "build-tools;35.0.0"` |

`build_all.bat` checks every prerequisite at startup and reports exactly what is missing with install instructions.

## 1. Dependencies (auto)

The repository does not ship third-party code or compilers. Everything is fetched lazily into git-ignored folders:

| Script | Fetches | Version | Destination |
|--------|---------|---------|-------------|
| `scripts\setup_deps.bat` | imgui | v1.92.3 | `third_party\imgui\` |
| `scripts\setup_deps.bat` | nlohmann/json | v3.11.3 | `third_party\nlohmann\` |
| `scripts\fetch_toolchain.bat` | MinGW-w64 | 13.2.0 (ucrt, seh) | `tools\mingw64\` |

Re-running these is safe -- each step short-circuits when the target already exists.

## 2. Desktop viewer

```powershell
scripts\build_desktop.bat
```

This one command chains `setup_deps` -> `fetch_toolchain` -> `desktop\compile.bat`. The output is a statically-linked `desktop\build\mimi_mod_desktop.exe` (~2 MB) that runs on any Windows 10/11 without extra DLLs.

The viewer renders the full menu at 560x960 with fake telemetry for live UI iteration.

## 3. Full build (Desktop + Android + Deploy)

```powershell
scripts\build_all.bat
```

This runs every step in order:

1. Check all prerequisites (reports `[ OK ]` / `[ FAIL ]` / `[ WARN ]` for each)
2. Fetch dependencies (imgui, nlohmann/json, MinGW)
3. Build desktop viewer
4. Build native .so (arm64-v8a + armeabi-v7a)
5. Decode, patch, rebuild and sign the APK
6. Deploy to a connected device via adb (skipped if no device)

### Keystore setup

Copy `scripts\local.env.bat.example` to `scripts\local.env.bat` and fill in your passwords:

```bat
set MOD_STORE_PASS=your_keystore_password
set MOD_KEY_PASS=your_key_password
```

A keystore at `android\keystore\mimi_mod.keystore` is auto-generated on the first run if missing. **Never commit passwords or keystores to the repository.**

### Environment variables

| Variable | Purpose |
|----------|---------|
| `MOD_STORE_PASS` | keystore password (required) |
| `MOD_KEY_PASS` | key password (required) |
| `MOD_TEMPLATE_APK` | base Subway Surfers APK for patching (default: `build\base_template.apk`) |
| `ANDROID_NDK_HOME` | path to Android NDK (auto-detected from SDK Manager if unset) |
| `ANDROID_HOME` | path to Android SDK (default: `%LOCALAPPDATA%\Android\Sdk`) |
| `JAVA_HOME` | path to JDK root (auto-detected from PATH if unset) |

## 4. Manual files required for APK patching

These files are **not** auto-fetched. Download and place them manually:

| File | Where to place | Download |
|------|----------------|----------|
| apktool | `tools\apktool.jar` | https://github.com/iBotPeaches/Apktool/releases |
| baksmali | `tools\baksmali.jar` | https://github.com/JesusFreke/smali/releases |
| Base APK | `build\base_template.apk` | extract from Subway Surfers 3.41.0 on device |
| Split arm64 | `build\split_config.arm64_v8a.apk` | extract from installed game |
| Split armv7 | `build\split_config.armeabi_v7a.apk` | extract from installed game |

## 5. Flashing to a device

```powershell
adb install -r build\subway_mod_signed.apk
```

The JNI entry attaches at `eglSwapBuffers` via a PLT hook, no additional injection required.

## Troubleshooting

- **`PREREQUISITE CHECK FAILED`** -- read the `[ FAIL ]` lines printed by `build_all.bat`, each one includes an install command or link
- **`imgui not found`** -- re-run `scripts\setup_deps.bat`
- **`ninja: command not found`** -- `sdkmanager "cmake;3.22.1"` or add its `bin/` to PATH
- **Black screen on device** -- check `adb logcat` for native crash logs, verify NDK ABI matches device
- **`apktool.jar not found`** -- download from [GitHub](https://github.com/iBotPeaches/Apktool/releases), place as `tools\apktool.jar`
- **`baksmali.jar missing`** -- download from [GitHub](https://github.com/JesusFreke/smali/releases), rename to `tools\baksmali.jar`
- **`MOD_STORE_PASS not set`** -- copy `scripts\local.env.bat.example` to `scripts\local.env.bat` and set passwords
- **`template APK not found`** -- place the original Subway Surfers 2.10.0 base APK at `build\base_template.apk`
