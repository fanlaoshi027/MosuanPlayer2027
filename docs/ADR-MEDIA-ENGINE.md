# Architecture decision: media engine

Status: Accepted

## Decision
Use Qt 6 Widgets + dynamically loaded libmpv on Windows x64. Distribute only an audited LGPL build of libmpv, configured with mpv's -Dgpl=false mode and linked against an LGPL-only FFmpeg build.

## Rationale
- mpv provides mature demuxing, decoding, audio/video synchronization, subtitles, seeking, speed control and hardware decode selection.
- Dynamic loading keeps the application independent of the import library and preserves a replaceable shared-library boundary.
- Qt owns the teaching-player interface, folder playlists, favorites and policy integration.
- The player uses a native child HWND as mpv's video target. Custom teaching effects will be evaluated after baseline playback is stable.

## Constraints
- Do not use a default GPL mpv build.
- Do not enable GPL/nonfree FFmpeg components.
- Pin the exact libmpv build and corresponding source before bundling.
- Validate the Windows D3D11VA/hwdec path on representative Intel/AMD/NVIDIA hardware.
- The current dynamic wrapper is an initial integration, not yet a verified playback release.
