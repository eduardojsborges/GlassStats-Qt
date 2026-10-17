# Validation

This is the **0.2.0-alpha.1 development preview**, not a declaration of universal compatibility.

## Current evidence

- Windows x64 Release build: **27 core cases passed** (29 Qt Test results including initialization/cleanup), with no failures or skips.
- Windows native smoke run: **34 checks passed**, using separate host/cover processes. Checks include both layouts, bright-background foreground contrast, unchanged opacity, large fonts/Fahrenheit, hidden metrics, attachment movement/minimization/overlap/tiny-host fitting, selection cancellation, locked recovery, target closure and first-open Settings visibility.
- The clean portable payload also passed all 34 smoke checks with the development Qt environment paths cleared. Hover details were verified outside the card while the host retained foreground focus. This exposed a focus-stealing Qt popup; the final implementation uses an explicit non-activating tooltip window. Composed-screen images of the full and compact cards over white/black content were inspected, including their borders and adaptive toolbar icons.
- Windows production installer and portable ZIP were built, with SHA-256 sidecars and dependency licenses/source access. The portable payload includes verified app-local MSVC DLLs; initialization succeeded with development paths cleared, and process-module inspection confirmed MSVCP140/VCRUNTIME140 loaded from the payload rather than System32. A separate per-user QA installer installed into the workspace, launched its deployed UI/provider successfully, then uninstalled successfully and removed the app executable. This does **not** validate administrator service/driver installation or all production installer paths.
- Four-platform CI run [38985343855](https://github.com/eduardojsborges/GlassStats-Qt/actions/runs/38985343855) passed Windows x64, Ubuntu x64, Intel Mac and Apple Silicon builds/tests/packaging. Later revisions must pass their own CI run. Build success does not establish native hardware/window compatibility.
- The final native application revision `e26bfa3` passed the standalone 34-check smoke suite. A new **two-hour stability run is pending**, started at **2026-10-17 00:34:56 UTC** from an isolated portable copy with app-local runtime DLLs, adaptation and attachment enabled. It alternates layouts and white/black/split/patterned backgrounds. `artifacts/qa/stability-current.json` records the candidate path, executable SHA-256 and run metadata; that directory contains `soak-progress.json` and writes `soak-result.json` on completion. Closing its QA host invalidates the run. Earlier short runs were stopped during delivery fixes; none is a completed two-hour result.


Local evidence is under `artifacts/qa` and is not committed. The legacy repository remained clean; its installation, updater and preference file were not changed. Screen captures are explicit QA artifacts only; ordinary monitoring never records screen buffers.

## Required manual/device coverage

- Windows 10 22H2, physical mixed-DPI monitor moves/removal, sleep/resume, virtual desktops and borderless games.
- Optional administrator service/driver setup, UAC cancellation, ACL behavior and uninstall on a disposable Windows environment.
- Linux X11/Wayland desktop-specific blur, shortcuts, trays, selection, overlap and GPU/sensor hardware coverage.
- macOS permissions, native materials, menu-bar recovery, spaces/fullscreen behavior, experimental attachment and actual Apple/Intel hardware.
- Real higher-version update download/install/restart on every package type; inert checksum tests do not establish an end-to-end installer update.

Unavailable/unverified integrations remain visible in Settings capabilities. Exclusive fullscreen and secure desktops are outside attachment coverage. macOS system GPU utilization, temperature and dedicated-memory accounting are not fabricated. Do not label untested systems supported based only on a passing build.
