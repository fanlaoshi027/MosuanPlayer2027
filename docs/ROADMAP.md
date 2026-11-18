# Roadmap

## P0 — technical feasibility
- Confirm Windows toolchain and Qt 6 installation.
- Build a minimal Qt Widgets window with CMake/MSVC.
- Obtain/build a known FFmpeg LGPL-only shared build; audit exact configuration and dependencies.
- Prototype decode + audio output + video presentation without copying full frames through CPU memory where feasible.
- Benchmark D3D11VA hardware decode and GPU effect pipeline.
- Decide whether to use a maintained LGPL-compatible audio output dependency or Windows WASAPI directly.
- Produce a license manifest and reproducible build notes.

## P1 — usable player shell
- Main window and video surface.
- Playback controls, seek, volume, speed, fullscreen.
- Error reporting and robust shutdown.

## P2 — file management
- Two folder-backed playlists.
- On-open SMB scan only; manual refresh; no periodic SMB scanning.
- Natural sort and supported video filtering.
- Folder favorites.
- SQLite persistence.

## P3 — teaching effects
- CSS-like simple invert.
- Configurable inverted background color.
- Teacher-camera exclusion rectangle with dimming.
- Crop and per-video profiles.

## P4 — student mode integration
- Signed policy format and ACL verification.
- Player-side restrictions only; OS application allowlisting remains the responsibility of MosuanLockdown.

## P5 — packaging
- Windows installer, upgrade/uninstall, crash logs, license notices and source bundle.
