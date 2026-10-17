# Compatibility status

`0.2.0-alpha.1` is a development preview. A successful CI build or package is not proof of hardware readings, permissions, desktop effects or window-manager behavior.

| Platform | Target | Native behavior still requiring device testing |
| --- | --- | --- |
| Windows | Windows 10 22H2 / Windows 11, x64 | Windows 10, mixed-DPI monitor changes, virtual desktops, sleep/resume, borderless games, elevated helper setup/removal and production installer paths |
| Linux X11 | Ubuntu 24.04, x64 | Native GPU/sensors, tray integration, shortcut grabs, drag/placement and opt-in attachment/capture |
| Linux Wayland | Ubuntu 24.04, x64 | Compositor/tray behavior and GPU/sensors; global window attachment/positioning and native global shortcuts are unavailable |
| macOS | 14.4+, Intel and Apple Silicon | Native materials, menu bar, permissions, spaces/fullscreen, hardware readings and opt-in Accessibility attachment/screen capture |

Windows native smoke coverage and outstanding gates are recorded in [VALIDATION.md](../VALIDATION.md). Windows exclusive fullscreen and secure desktops are outside attachment coverage. Protected, elevated or inaccessible target windows can be rejected gracefully; GlassStats does not elevate itself to control them.

Windows uses PDH/DXGI for utilization and dedicated GPU memory, Windows memory APIs for RAM, and the separate LibreHardwareMonitor bridge for temperatures. Driver/counter availability varies. The optional self-contained .NET helper has its own OS/runtime support requirements.

Linux CPU/RAM come from `/proc`; temperatures come from available `hwmon` or vendor interfaces. DRM/sysfs and optional NVML provide GPU readings when supported. Unsupported or inaccessible counters are unavailable, not zero. X11 attachment/capture require explicit experimental environment flags; Wayland cannot obtain the same capabilities through standard Qt windows.

macOS CPU/RAM use Mach APIs and identify the memory accounting method. Metal supplies adapter identity. Aggregate system GPU utilization, temperatures and system GPU allocations do not currently have a validated provider. Apple unified memory is never presented as dedicated VRAM. Accessibility and Screen Recording denial leave monitoring operational, with unsupported window/capture features disabled and text halos available.

Native blur is capability-dependent. Translucent tint is the explicit fallback; the saved opacity is not rewritten. Packages are unsigned or ad-hoc signed and not notarized unless their release notes explicitly state otherwise. Qt is dynamically linked; corresponding source access and third-party notices accompany redistribution.
