# MosuanPlayer2027

Windows-first teaching video player. Proprietary application code; third-party dependencies retain their own licenses.

## Product priorities
1. Two independent folder-backed dynamic playlists.
2. Folder favorites for fast navigation.
3. SMB folders are scanned at application startup and only on explicit manual refresh; no periodic SMB polling or background watcher.
4. Lightweight GPU video inversion, configurable inverted background, teacher-camera exclusion rectangle with optional dimming.
5. Crop, playback speed, seek, volume, fullscreen and sequential playback.
6. Later integration with MosuanLockdown via signed, ACL-protected policy.

## Chosen technology
- C++20, Qt 6 Widgets, CMake, MSVC x64.
- Playback engine: dynamically loaded libmpv (libmpv-2.dll).
- Distribution target: closed-source application + audited LGPL-only libmpv build (-Dgpl=false) using an LGPL-only FFmpeg build.
- Do not use the default GPL mpv build. Verify actual build flags, transitive dependencies, source correspondence, notices and replaceability for every release.
- Qt modules must be individually checked for LGPL eligibility; avoid GPL-only Qt modules.

## Playback integration status
The code now contains a dynamic libmpv loader, native Windows video surface, playlist double-click playback, pause/resume and speed controls. The CI package currently does not include libmpv; playback requires an audited LGPL libmpv-2.dll beside the EXE. Missing DLLs are reported clearly. The next step is to pin, verify and package the exact LGPL binary and corresponding source bundle.

## SMB refresh contract
- Each configured SMB playlist is scanned once on application startup.
- Manual Refresh triggers one scan.
- No periodic polling, watcher, or automatic scan on playlist panel activation.
- Playback itself never triggers a directory scan.
- If SMB is unavailable, keep the last known list and show an offline/error state.

## Status
Prototype, not yet a distributable commercial release. See docs/LICENSING.md and docs/ROADMAP.md.
