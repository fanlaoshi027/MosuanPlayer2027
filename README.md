# MosuanPlayer2027

Windows-first teaching video player. Proprietary application code; third-party dependencies retain their own licenses.

## Product priorities
1. Two independent folder-backed dynamic playlists and folder favorites.
2. SMB folders scan once at startup and only on manual refresh; no periodic polling or watcher.
3. Smooth GPU video playback through dynamically loaded libmpv.
4. Teaching-friendly inversion, configurable inverted background, camera exclusion rectangle and dimming.
5. Crop, playback speed, timeline seek, volume, previous/next and sequential playback.

## Technology
- C++20, Qt 6 Widgets, CMake, MSVC x64.
- Playback: dynamically loaded libmpv-2.dll.
- Pinned runtime: mpv v0.42.0 official x86_64-w64-mingw32 LGPL release asset, SHA-256 verified by CI.
- Distribution target: closed-source app with an audited LGPL media runtime. Keep corresponding source, notices, build metadata and replaceability obligations with each release.

## Implemented
- Single-pass GPU GLSL inversion at mpv's OUTPUT stage; no CPU frame readback or resampling is introduced by the inversion pass.
- Inversion toggle and persistent background presets: 90% gray-black, charcoal, deep blue and deep green. Near-white source pixels are smoothly mapped to the selected dark background to keep the board readable.
- Native Windows video surface and libmpv embedding.
- Double-click playback from either playlist.
- Pause/resume and speed presets 1.0x, 1.25x, 1.5x, 1.75x and 2.0x.
- Playback position/duration polling, seek slider and elapsed/total time.
- Volume slider.
- Previous/next playlist navigation and automatic next-item playback at end of file.
- Folder-backed playlists, favorites and SMB startup/manual refresh contract.

## Not yet implemented
- Camera exclusion rectangle with independent dimming.
- Crop UI and persistent crop presets.
- Fullscreen mode and configurable hotkeys.
- Resume position persistence and playlist queue behavior.
- Automated runtime playback tests on physical Windows hardware.

## Build and licensing
See `docs/LICENSING.md` and `docs/ADR-MEDIA-ENGINE.md`. The current CI artifact is a technical test build, not a commercially audited release. Verify the exact DLL dependency closure, source correspondence, notices and LGPL obligations before commercial distribution.
