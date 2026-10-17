# Moving from the C# app

The Qt preview is a separate application. Keep the original installation available while evaluating it.

1. Launch the Qt build. On Windows, Settings offers to import `%LocalAppData%\GlassStats\settings.json` when that file exists.
2. Choose **Import a copy** to carry over appearance, sizes, metric visibility, device/sensor selections, shortcuts and placement. The original file is read without modification; a copy is saved as `legacy-import.json` beside the Qt settings. Invalid values receive safe defaults, and placement is fitted to an available monitor.
3. Attachment is session-only and starts detached. The imported overlay starts unlocked. Unavailable devices/sensors remain selectable or show unavailable readings; review their selections after hardware changes.
4. If the original app already starts at sign-in, Qt leaves that entry in place and disables its own startup preference. Use **Switch sign-in startup to Qt** in Settings only when ready. This removes the old startup entry, not the old app or preferences.
5. Quit the old overlay when comparing shortcuts: both apps cannot register the same global combination. Qt reports conflicts; the tray recovery command remains available.

Qt settings are stored under the OS application-local data location for organization `eduardojsborges`, application `GlassStats-Qt`. On Windows this is normally `%LocalAppData%\eduardojsborges\GlassStats-Qt`. `--profile <directory>` creates an independent profile and instance endpoint; use it only for deliberate testing.

The updaters remain independent. The C# app checks `GlassStats`; Qt checks `GlassStats-Qt`. The old updater never installs this remake automatically. No public Qt release is required to run a workspace build.

To roll back, quit Qt, disable its sign-in startup, and launch the original app. Re-enable the original app's startup there if it was switched. Removing Qt does not remove the original app, its preferences, its service or the shared PawnIO driver.

Very old left-based compact positions are converted using a legacy width estimate before monitor fitting; their exact original pixel position is not guaranteed. Current version-2 top-right positions are preserved directly, subject to fitting.
