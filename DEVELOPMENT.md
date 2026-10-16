# Development

Pinned toolchain: C++20, Qt **6.12.0**, CMake **3.24+**, Ninja, MSVC 2022 x64 on Windows, GCC on Ubuntu 24.04, Xcode 16+ on macOS 14.4+. Qt 6.12 is the final Qt branch advertising Windows 10 support; do not upgrade the baseline without reviewing that target.

## Windows

Install Visual Studio 2022 Build Tools with C++ x64 tools, CMake, Ninja and Python 3.11+. From the project root:

```powershell
./scripts/Bootstrap.ps1
./scripts/Build.ps1
./scripts/Build-SensorBridge.ps1 -OutputDirectory build/sensor-bridge
$env:PATH="$PWD/.tools/qt/bin;"+$env:PATH
$env:QT_PLUGIN_PATH="$PWD/.tools/qt/plugins"
$env:QML_IMPORT_PATH="$PWD/.tools/qt/qml"
./build/GlassStats.exe
```

The optional bridge needs .NET SDK **10.0.401**, LibreHardwareMonitorLib **0.9.6** and ServiceController **10.0.0**; it publishes self-contained. Set `DOTNET_EXE` to a local SDK if needed. The main app has no .NET dependency.

Qt bootstrap reads the official architecture-specific installer metadata, validates archive SHA-1 checksums supplied by Qt, and records SHA-256 download manifests locally. The upstream metadata/checksums share the download trust boundary. `.tools`, build outputs and QA artifacts stay out of Git. Binary dependencies are dynamically linked and deployed alongside the app.

```powershell
./scripts/Build.ps1 -Package
```

Package prerequisites: Inno Setup 6.7.3, Microsoft's signed x64 VC redistributable, and signed PawnIO setup from the matching LibreHardwareMonitor source release. Configure explicit paths with the package script parameters. The script can reuse verified local prerequisites from a sibling legacy checkout; that checkout is never modified.

## Linux/macOS

```sh
python3 -m venv .tools/python
.tools/python/bin/pip install py7zr==1.1.4
.tools/python/bin/python scripts/download_qt.py --host linux # or macos
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$PWD/.tools/qt"
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
```

Linux needs X11 development libraries and Qt's runtime XCB/Wayland dependencies; see CI for the explicit Ubuntu package list. Qt's own macOS binaries are universal; configure `CMAKE_OSX_ARCHITECTURES=arm64` or `x86_64` for the requested app package. Real Mac/Linux behavior remains unverified until exercised on those desktops.

## Tests

- Core Qt Test suite ports the original cases and extends them for platform capabilities, unified memory, architecture assets, migration isolation and foreign update URLs.
- `GlassStats --qml-check` loads the UI/provider in an isolated profile and exits. This checks initialization, not hardware/desktop parity.
- `GlassStats --smoke --qa-dir <absolute-directory> --profile <isolated-directory>` runs Windows native/visual checks with a separate test-host process and writes JSON/PNG artifacts.
- `GlassStats --soak --qa-dir <absolute-directory> --profile <isolated-directory>` runs 120 minutes, alternates layouts/backgrounds and records process CPU, memory, handles, GDI, attachment and sample freshness. `GLASSSTATS_SOAK_MINUTES` shortens development checks only.
- `GlassStats --test-host --qa-dir <directory>` starts a separate host and covering window; Windows QA messages are `0x8101`–`0x810F`.

Validation modes never register startup/global shortcuts or write normal preferences. Screenshots are produced only by explicit QA runs; production capture does not save pixels. Validation profiles use separate single-instance endpoints. Release/package claims must distinguish automated checks from real-device testing.

## Publication

Version source is `GLASSSTATS_VERSION` in CMake; keep installer/resource/bridge metadata aligned. App asset names include version, OS and architecture; see the updater's `assetName` function. Include SHA-256 checksums, all licenses, and corresponding-source access for redistributed LGPL libraries. Public publication is separate from CI artifact production. Never publish binaries as a feature-complete replacement while parity gates remain open.
