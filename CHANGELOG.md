# Changelog

All notable changes to PodSwitch will be documented here.
Format based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Added
- Initial plugin implementation inspired by RØDECaster Video Auto Switching

### Fixed
- Fixed critical bug where opening the Settings Dialog would wipe existing configurations.
- Fixed UI layout memory leak in AutoCamDock where VU meters caused infinite layout stacking.
- Fixed severe memory leak in `get_scene_names()` array processing.
- Fixed application crash on module unload by properly terminating `AutoCamDock` timer.
- Fixed deadlocks and race conditions in `SwitchEngine` transition logic.
- Fixed multithreading TOCTOU bug in `AudioMonitor` source assignment.
- Fixed issue where setting a custom threshold to 0 dBFS reverted to -40 dBFS.
- Audio monitor now correctly checks for maximum RMS across all audio channels (stereo mic support).
- Build system now uses relative include paths and standard CMake installation targets instead of hardcoded iCloud paths.
- Synchronized internal state tracking with manual user scene transitions.
- `AudioMonitor`: per-source OBS audio capture tap with RMS → dBFS conversion
- `SwitchEngine`: EMA-smoothed level tracking with priority bias and hold time
- `Config`: JSON settings persistence via `obs_data` to OBS config directory
- Qt dock panel with live gradient VU meters and green/grey enable toggle
- Full settings dialog with drag-and-drop-style mapping table
- 3 responsiveness modes: Relaxed (α=0.05), Neutral (α=0.15), Fast (α=0.40)
- Priority levels: Low (−6 dB), Medium (0 dB), High (+6 dB)
- Per-mapping noise threshold gate (default −40 dBFS)
- Fallback scene when no speaker is active
- Optional fade transition with configurable duration
- GitHub Actions CI for Windows, macOS, and Linux builds
- Cross-platform CMakeLists.txt (requires OBS Studio ≥ 30.0.0)

## [1.0.0] — 2026-02-19
- First public release 🎙️
