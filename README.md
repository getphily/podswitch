# PodSwitch — The Podcast Automatic Director

> **PodSwitch** is an OBS Studio plugin that acts as an **Automatic Director** for podcasts. It switches cameras based on who's speaking, detects cross-talk, shows reaction shots, and can even generate your entire scene collection with one click.

![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)
![OBS Studio 30+](https://img.shields.io/badge/OBS%20Studio-30%2B-blue.svg)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-lightgrey)

---

## ✨ Features

| Feature | Description |
|---|---|
| **Mic → Camera Mapping** | Link any OBS audio source to any OBS scene |
| **Cross-Talk Detection** | Automatically cuts to the Wide/Split scene when multiple people talk at once |
| **Motion Energy Tracking** | Detects visual reactions (nodding, gesturing) even when someone is silent |
| **Scene Generator** | One-click generation of Solo, Split Screen, and Audio Mix scenes |
| **Nested Audio Mix** | Automatically creates a shared "🎙️ Live Audio Mix" scene nested in every generated scene |
| **Priority Levels** | Low / Medium / High bias — give your host cam priority |
| **Responsiveness Modes** | Relaxed 🐢 / Neutral / Fast ⚡ EMA smoothing |
| **Hold Time** | Minimum time a camera stays active (no flickering) |
| **Noise Gate** | Per-mic dBFS threshold — ignore background noise |
| **Fallback Scene** | Auto-switch to a wide shot when no one speaks |
| **Fade / Cut Transition** | Optional fade with configurable duration |
| **Live Dual Meters** | Real-time Audio (green) + Motion Energy (blue) bars in the dock |
| **One-click Toggle** | Big green ON button in the dock |

---

## ⚠️ Important: Audio Routing

> **PodSwitch only automates visual scene switching.** If you are NOT using the built-in Scene Generator, you MUST set up a separate "Live" global audio mix in OBS. Go to **Settings → Audio** and add your microphones to the global audio devices, or manually nest an audio group scene into every scene. Otherwise, switching away from a camera scene may mute that person's microphone.
>
> If you ARE using the Scene Generator, this is handled automatically — the plugin creates a nested "🎙️ Live Audio Mix" scene containing all your mics and embeds it in every generated scene.

---

## 🎬 How It Works

Every ~10ms, PodSwitch's Automatic Director:
1. Reads the **RMS audio level** of each mapped microphone
2. Applies **Exponential Moving Average** smoothing (controlled by Responsiveness)
3. Adds a **priority bias** (±6 dB) to favour preferred cameras
4. Counts **active speakers** — if 2+ are above threshold, it's cross-talk → switch to Wide/Split
5. If only one speaker is active, switches to their Solo scene — respecting the **hold time**
6. Falls back to a wide shot when everyone goes silent
7. In parallel, the **Motion Detector** compares video frames to detect visual reactions (gestures, nodding), displayed in the dock as blue energy bars

---

## 🎙️ Scene Generator

PodSwitch can automatically create a complete podcast scene collection for you.

### Supported Formats
| Format | Scenes Created |
|---|---|
| **1-on-1 Interview** | Host 1 Solo, Guest 1 Solo, 50/50 Split Screen |
| **Power Dynamic (3-person)** | Host 1 Solo, Guest 1 Solo, Guest 2 Solo, Asymmetrical Split (Host 50% left, Guests stacked right) |

### How to Use
1. Open **⚙ Settings** → **Scene Generator** tab
2. Select your **Podcast Format**
3. Assign your raw camera sources to Host 1, Guest 1 (and Guest 2 if applicable)
4. Click **✨ Generate Podcast Scenes**
5. The plugin creates all scenes with correct crops, scaling, and a nested audio mix

> **Note:** Clicking Generate will delete any previously generated PodSwitch scenes and rebuild them fresh for clean layouts.

---

## 📸 UI Overview

**Dock Panel**
```
┌──────────────────────────────────┐
│  🎥 PodSwitch              [ON] │
├──────────────────────────────────┤
│  Mic 1 → Host   ┤██████░░░│ 🔊  │
│                  ┤██░░░░░░░│ 🏃  │
│  Mic 2 → Guest  ┤███░░░░░░│ 🔊  │
│                  ┤█░░░░░░░░│ 🏃  │
├──────────────────────────────────┤
│  Responsiveness: [Neutral  ▼]   │
│  [⚙ Open Settings]              │
└──────────────────────────────────┘
```
🔊 = Audio level (green)  |  🏃 = Motion energy (blue)

**Settings Dialog** — Two tabs:
- **General**: Map each mic to a scene, set thresholds, pick responsiveness, hold time, fallback scene, and transition type.
- **Scene Generator**: Select podcast format, assign camera sources, and generate scenes with one click.

---

## 🔧 Requirements

- **OBS Studio** ≥ 30.0.0
- A C++17 compiler (MSVC 2019+ / Xcode 14+ / GCC 11+)
- CMake ≥ 3.16
- Qt 6 (or Qt 5 fallback) — bundled with OBS

---

## 🚀 Building

```bash
git clone https://github.com/getphily/podswitch.git
cd podswitch

cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=Release \
  -Dlibobs_DIR=/path/to/obs-studio/build

cmake --build build --config Release
cmake --install build
```

The plugin is auto-installed to your OBS plugins directory.

---

## 🎙️ Quick Start

1. Open OBS → find the **PodSwitch** dock (View → Docks)
2. Click **⚙ Open Settings**
3. *(Optional)* Go to the **Scene Generator** tab and generate your scenes
4. In the **General** tab, add a mapping per speaker:
   - **Audio Source** → microphone
   - **Scene / Camera** → OBS scene to cut to
   - **Priority** → High for close-ups, Low for wide shot
   - **Threshold** → silence gate (default −40 dB)
5. Set a **Fallback Scene** (e.g. Split Screen or wide shot)
6. Hit **● Enable** — PodSwitch takes over 🎉

---

## 📁 Project Structure

```
podswitch/
├── CMakeLists.txt
├── buildspec.json
├── src/
│   ├── plugin-main.cpp            OBS entry point
│   ├── audio-monitor.cpp/h        Audio tap → dBFS callback
│   ├── switch-engine.cpp/h        EMA + priority + cross-talk + hold time
│   ├── motion-detector.cpp/h      Video frame MAD motion energy
│   ├── scene-generator.cpp/h      OBS API scene construction
│   ├── config.cpp/h               JSON persistence
│   ├── utils.h                    dBFS math + EMA helper
│   └── ui/
│       ├── dock-widget.cpp/h      OBS dock panel (dual meters)
│       └── settings-dialog.cpp/h  Settings UI (General + Scene Generator tabs)
└── data/locale/en-US.ini
```

---

## 📄 License

MIT © 2026 getphily

Inspired by the [RØDECaster Video](https://rode.com/en-us/products/rodecaster-video) Auto Switching feature.
