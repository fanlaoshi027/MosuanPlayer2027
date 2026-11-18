# Architecture

## Application layers
- **UI (Qt Widgets):** main window, video surface, two playlist panels/tabs, folder favorites, settings.
- **Application services:** playback coordinator, playlist coordinator, favorite service, resume service.
- **Domain:** FolderPlaylist, MediaEntry, FavoriteFolder, PlaybackSession, CropProfile, ExclusionRegion, PlayerPolicy.
- **Infrastructure:** Windows filesystem/UNC access, SQLite persistence, FFmpeg adapter, GPU renderer, policy verifier.
- **Platform boundary:** Windows-specific file identity, shell integration, D3D11 and SMB behavior isolated behind interfaces.

## Suggested modules
- app/: startup and dependency composition
- ui/: MainWindow, VideoSurface, PlaylistView, FavoritesView
- playback/: PlaybackController, IMediaEngine, FfmpegMediaEngine
- media/: MediaScanner, NaturalSort, MediaMetadata
- playlists/: FolderPlaylist, PlaylistRepository, ScanCoordinator
- favorites/: FavoriteRepository
- effects/: InvertEffect, RegionMask, CropTransform
- persistence/: SQLite database and migrations
- platform/windows/: UNC path and D3D11 integration
- security/: policy schema and signature verification (later)
- tests/: unit and integration tests

## Media engine decision
Do not embed libmpv because mpv is GPL-licensed. The proprietary-app route requires an independently audited LGPL-compatible dependency set. First validate FFmpeg LGPL-only DLLs and build a minimal playback pipeline. Use dynamic linking and keep the application code separate. The rendering design should prefer GPU texture processing and avoid CPU frame copies.

## Threading
- UI thread owns Qt widgets.
- Scanner runs on a worker thread and returns an immutable scan result.
- SMB scan must be explicitly requested by opening/reopening the playlist or pressing Refresh.
- Decoder/render thread must never block the UI thread.
- Playlist snapshot is committed atomically only after a successful scan.

## Persistence
Use SQLite for playlists, favorites, resume positions, crop profiles, exclusion regions and settings. Store paths as Unicode strings. Do not store SMB passwords.
