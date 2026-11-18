# Folder-backed playlists and SMB behavior

## Product model
There are exactly two independent folder-backed playlists, Playlist 1 and Playlist 2. Each stores one root folder path and its own scan snapshot, selected item, sort mode, and playback state. A separate Favorites collection stores folder shortcuts; favorites are not playlists.

## Refresh semantics (strict)
- First open of a configured playlist: scan once.
- Close/switch away, then open it again: scan once.
- Explicit Refresh action: scan once.
- No timer, polling loop, QFileSystemWatcher, SMB notification subscription, or scan triggered by playback.
- While the playlist view remains open: retain the snapshot without background SMB refresh.
- On network failure: retain prior snapshot and mark it unavailable/stale; never replace it with an empty list.
- A successful scan replaces the snapshot atomically.
- Opening the same playlist repeatedly while it is already active must not cause repeated scans.
- Optional recursive scanning is configurable; default OFF.
- Include supported video extensions only, configurable list.
- Natural filename sort (2 before 10), deterministic case-insensitive primary comparison with stable tie-breaker.
- Ignore temporary/partial files where practical (e.g. copy-in-progress); do not assume file size stability proves completion.
- Manual Refresh button is always available.

## Playback
- Double-click or Enter opens selected media.
- Sequential next/previous, repeat off / single / playlist.
- Removing a file from the share while playing must not crash or interrupt decoder unexpectedly; report next-item behavior clearly.
- Do not save SMB credentials. Use Windows integrated SMB access and user-provided Windows credentials.
- Paths are UNC paths; no custom SMB protocol implementation in v1.

## Acceptance tests
1. New SMB folder open -> exactly one scan.
2. Wait 30 minutes -> zero additional scans.
3. Play/pause/seek/next -> zero directory scans except loading explicitly selected next file must not rescan.
4. Close playlist and reopen -> exactly one new scan.
5. Manual Refresh -> exactly one new scan.
6. SMB unavailable -> prior snapshot remains visible and marked offline.
7. Reconnect without reopen -> no implicit scan; manual Refresh or reopen updates it.
8. Two playlists maintain independent paths and snapshots.
