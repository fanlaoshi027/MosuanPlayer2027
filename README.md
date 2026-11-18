# MosuanPlayer2027

Windows-first teaching video player. Proprietary application code; third-party dependencies must be independently license-audited.

## Product priorities
1. Two independent folder-backed dynamic playlists.
2. Folder favorites for fast navigation.
3. SMB folders are refreshed **only when the playlist is explicitly opened/re-opened or the user presses Refresh**. No periodic SMB polling and no background SMB watcher.
4. Lightweight GPU video inversion, with selectable inverted background color.
5. Rectangular teacher-camera exclusion region: preserve original colors and optionally dim only that region.
6. Crop, playback speed, seek, resume position, fullscreen, and sequential playback.
7. Later integration with MosuanLockdown via a signed, ACL-protected policy file.

## Chosen direction
- C++20, Qt 6 Widgets, CMake, Ninja, MSVC x64.
- Media pipeline: evaluate a minimal FFmpeg LGPL-only dynamic build plus a self-owned playback/render pipeline. Do not use libmpv: mpv is GPL-licensed, which conflicts with the selected proprietary distribution goal.
- Qt modules must be individually checked for LGPL availability. Avoid GPL-only Qt modules.
- Windows-first. Keep platform-specific code behind interfaces for possible future macOS port.

## SMB refresh contract
- Opening a folder-backed playlist triggers one scan and updates its snapshot.
- While that playlist remains open, do not periodically rescan SMB.
- Switching away and later opening it again triggers a new scan.
- Manual Refresh triggers a scan.
- Playback itself never triggers a directory scan.
- If SMB is unavailable, keep the last known list and show an offline/error state; do not erase the snapshot.
- Local folders may later support native change notifications, but must not change the SMB contract.

## Repository status
Initial architecture and technical validation phase. No claim of production-ready playback or license compliance until dependency binaries, exact build flags, source archives, and notices are audited.

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md), [docs/SMB_PLAYLIST_SPEC.md](docs/SMB_PLAYLIST_SPEC.md), [docs/VIDEO_EFFECTS_SPEC.md](docs/VIDEO_EFFECTS_SPEC.md), and [docs/LICENSING.md](docs/LICENSING.md).
