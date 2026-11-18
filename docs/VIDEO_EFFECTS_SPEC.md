# Video effects

## Core principle
Apply effects to the decoded video image in the GPU presentation path. Do not capture the window, process screenshots, copy every frame through CPU memory, or re-encode the source video.

## Inversion modes
1. Original: no effect.
2. Simple invert: equivalent in intent to CSS `filter: invert(1)`, applied to video pixels only.
3. Invert with configurable resulting background color: user selects a target color for pixels that would otherwise become white/light background. Keep algorithm simple; color fidelity is not the priority.
4. Optional selective/smart invert may be evaluated later; do not delay simple invert for it.

## Teacher-camera exclusion region
- User toggles region-edit mode and drags a rectangle on the displayed video.
- Rectangle uses normalized video coordinates [0..1], not window coordinates.
- Region is excluded from inversion and receives configurable brightness multiplier (default 0.70; adjustable).
- Outside region: selected inversion effect applies.
- Support move, resize, enable/disable, reset and per-video persistence.
- v1 supports one rectangle; architecture can later allow multiple.
- Correct mapping must account for letterboxing, aspect ratio, crop, resize and fullscreen.
- No face recognition or AI required.

## Rendering validation
- Verify video-only effects; Qt controls and subtitles/UI overlays must not accidentally be inverted.
- Verify 1080p60 and 4K playback with effects enabled.
- Compare GPU utilization, dropped frames and latency against effects-off baseline.
- Avoid claims of zero overhead; measure actual cost.
