# Validation

This is the **0.2.0-alpha.1 development preview**, not a declaration of universal compatibility.

## Current evidence

- The Windows x64 C++/Qt build and ported core tests run locally.
- Native UI automation uses a separate host/cover process. Early checks exposed a Qt/native visibility mismatch; visibility was synchronized before the composed-screen readability checks.
- Final test counts, installer results and stability measurements are recorded after their runs complete. Local evidence lives under `artifacts/qa` and is not committed.
- CI builds/checks Windows, Ubuntu, Intel Mac and Apple Silicon Mac independently. Build success proves compilation and test results only, not native hardware/window behavior.

## Required manual/device coverage

- Windows 10 22H2, physical mixed-DPI monitor moves/removal, sleep/resume, virtual desktops and borderless games.
- Optional administrator service/driver setup, UAC cancellation, ACL behavior and uninstall on a disposable Windows environment.
- Linux X11/Wayland desktop-specific blur, shortcuts, trays, selection, overlap and GPU/sensor hardware coverage.
- macOS permissions, native materials, menu-bar recovery, spaces/fullscreen behavior, experimental attachment and actual Apple/Intel hardware.
- Real higher-version update download/install/restart on every package type; inert checksum tests do not establish an end-to-end installer update.

Unavailable/unverified integrations remain visible in Settings capabilities. Exclusive fullscreen and secure desktops are outside attachment coverage. macOS system GPU utilization, temperature and dedicated-memory accounting are not fabricated. Do not label untested systems supported based only on a passing build.
