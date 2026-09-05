# Contributing to PodSwitch

Thank you for your interest! Contributions are welcome.

## Development Setup

```bash
git clone https://github.com/phil426/switchy-podcast-auto-switcher.git
cd switchy-podcast-auto-switcher

# macOS (Homebrew)
brew install obs-studio cmake ninja

cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

## Pull Request Guidelines

1. Fork the repo and create a feature branch (`git checkout -b feature/my-feature`)
2. Write focused, well-commented C++17
3. Test with at least 2 audio sources mapped to 2 scenes
4. Open a PR with a clear description

## Reporting Bugs

Open an issue with:
- OBS version
- OS + version  
- Exact steps to reproduce
- OBS log (`Help → Log Files → Upload Current Log`)

## Ideas for Future Features

- Face detection mode (OpenCV)
- obs-websocket trigger support
- Per-mapping transition types
- MIDI/controller integration
