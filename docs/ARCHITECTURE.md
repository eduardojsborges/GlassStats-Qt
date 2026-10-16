# Architecture

`core` contains nullable readings and availability states, history, adapter aggregation, proportional placement, contrast selection, settings migration and semantic update selection. It has no native window or hardware access. The QML UI consumes controller properties and shared actions; it never polls sensors or changes native stacking directly.

The main controller owns a UI dispatcher and one persistent worker thread. Monitoring and surface capture are queued there; each permits only one operation in flight. Surface requests snapshot geometry, cover rectangles and a generation. UI callbacks discard stale results after movement/layout/visibility changes. Per-label neighboring pixels exclude foreground UI, charts, rings, rounded corners, borders and covering windows. Nothing is recorded in normal use.

Platform services own native windows, hooks, selection, permissions, shortcuts and startup. Windows attachment preserves an independent top-level window, follows target/process identity, clips inside the target and restores stacking immediately above it. Visibility requested by the user stays separate from host minimization/hiding. No target reparenting, ownership, styles or positions are changed.

Windows metrics use PDH/DXGI/GlobalMemoryStatusEx. The retained .NET sensor bridge is an isolated child or optional privileged service with its own identifiers; only timestamped sensor JSON crosses the boundary. The service uses a local, read-only, user/SYSTEM pipe and opens sensors only while connected. Linux uses /proc/hwmon/DRM/NVML. macOS uses Mach and Metal, with explicit missing-data reasons for unsupported counters.

The update client checks the new repository, selects an exact platform/architecture asset, downloads only after confirmation, validates SHA-256 and opens a package interactively. It never replaces a running workspace binary. Preferences use an atomic separate JSON store; legacy import is a copy, not a shared mutable profile.
