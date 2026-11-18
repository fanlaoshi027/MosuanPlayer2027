# Folder-backed playlists and SMB behavior

## Product model
There are exactly two independent folder-backed playlists. Each stores one root folder path and its own persisted scan snapshot. A separate Favorites collection stores folder shortcuts; favorites are not playlists.

## SMB refresh semantics (strict)
- At application startup, scan each configured SMB playlist exactly once.
- Selecting a folder, switching playlist panels, opening/revisiting a playlist panel, and playback actions must not trigger a scan.
- No timer, polling loop, QFileSystemWatcher, SMB notification subscription, or background SMB scan.
- An explicit user click on **Manual Refresh** triggers exactly one scan.
- Changing a playlist root folder only saves the path; it does not scan immediately. The user can manually refresh or restart the application.
- On network failure, retain the last successful persisted snapshot and mark the path offline; never replace the snapshot with an empty list.
- A successful enumeration atomically replaces the snapshot.
- Folder favorites are shortcuts. Double-clicking a favorite assigns it to Playlist 1 without scanning; startup or Manual Refresh updates it.
- Recursive scanning is OFF in v1.
- Supported video extensions are filtered and filenames use natural numeric sorting (2 before 10).
- SMB credentials are never stored by the application; Windows manages access credentials.

## Playback
- Double-click selects a media item; actual playback is implemented by the media-engine phase.
- Sequential next/previous and repeat modes will operate on the current snapshot and must not rescan folders.
- If a network file disappears during playback, report the playback error without automatically rescanning.

## Acceptance tests
1. App startup with two configured SMB folders -> one scan per folder.
2. Wait 30 minutes -> zero additional scans.
3. Open/switch/revisit playlist panels -> zero scans.
4. Play/pause/seek/next -> zero directory scans.
5. Change a playlist folder -> zero immediate scans.
6. Manual Refresh -> exactly one scan for that playlist.
7. Restart application -> exactly one scan per configured playlist.
8. SMB unavailable at startup/refresh -> previous persisted snapshot remains visible and path marked offline.
9. Two playlists keep independent paths and snapshots.
