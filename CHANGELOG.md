# Changelog

## 0.2.0-alpha.1 — Qt remake preview

- Recover Settings inside an available monitor when opened or after display changes, restore it from minimization, and fit oversized windows while keeping the title bar accessible.
- Fixed white rectangular backing behind rounded Windows cards. Request alpha surfaces before window creation and preserve Qt/native rounded masks through visibility changes. Custom-rounded cards use translucent tint; native system Acrylic remains available for square cards.

- Added C++20/Qt Quick full and compact overlays, independent appearance controls, graphs, memory rings, hover details, tray/context actions, and settings import.
- Ported Windows monitoring, adaptive text sampling, placement/recovery, click-through, shortcuts, app selection/attachment, and isolated temperature bridge.
- Added experimental Linux and macOS native backends with explicit capability/permission reporting and unavailable metric reasons.
- Added architecture-aware GitHub update selection, verified downloads, isolated tests, native host fixtures, packaging scripts, and multi-platform CI.
- Preserved the C# repository and settings as a separate comparison/rollback path. See validation and compatibility documents before using on other systems.
