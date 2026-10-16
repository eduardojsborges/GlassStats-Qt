# Parity checklist against GlassStats 0.1.3

All features remain part of the remake's contract. Implementation and validation are separate gates.

| Feature | Windows implementation | Linux/macOS |
| --- | --- | --- |
| CPU/GPU/RAM/VRAM, adapter identity | Native providers | Native providers; unavailable counters explained |
| Temperatures, sensor selection | Retained isolated bridge/service | hwmon/NVML on Linux; Mac provider unavailable |
| Full/compact, CPU→GPU→RAM→VRAM | QML layouts | Shared |
| Hover details, histories, rings | QML popups/Canvas | Shared; native window behavior unverified |
| Theme/colors/wheel/hex, opacity, fonts, size/radius, visibility | Settings controls | Shared |
| Protective veil, adaptive per-label colors, halo | GDI + shared contrast | Capture opt-in/unverified; halo fallback |
| Top-right anchor, drag/release, monitor fitting, temporary scaling | Native/shared | X11/Mac need device checks; Wayland restrictions explicit |
| Tray/menu-bar, context menu, show/hide/quit | Qt/native | Desktop integration unverified; tray availability varies |
| Click-through, lock/recovery shortcuts | Native Win32 | Native or capability-limited |
| Glue/select/cancel, move/resize, overlap, minimize/hide/close recovery | Native WinEvent controller | Experimental opt-in X11/AX; Wayland unavailable |
| Startup, separate single instance, preference persistence/reset | Native/shared | Native/shared; device verification required |
| Legacy settings import, startup switch, rollback coexistence | Separate profile/identities | No legacy Windows import needed |
| Daily/manual/beta updates, notes, confirmation, SHA-256 | New repo/platform assets | Shared; package-specific installation |
| Helper setup/remove and clean uninstall | Separate protected service/scripts | Native providers; no Windows driver |
| Offline/no account, bounded logs, licenses | Preserved | Preserved |

Feature-complete status requires the tests in the approved plan and real-device coverage; it is not inferred from the existence of a control or backend.
