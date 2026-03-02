# midi-visu

[![CI](https://github.com/conjius/midi-visu/actions/workflows/ci.yml/badge.svg)](https://github.com/conjius/midi-visu/actions/workflows/ci.yml)
[![Pages](https://img.shields.io/badge/GitHub%20Pages-live-32cd32)](https://conjius.github.io/midi-visu/)

A real-time MIDI visualizer built as a JUCE audio plugin. Receives MIDI input and renders
animated circles — one per voice — with drum hit-counter animations, video backgrounds,
and a configurable options panel. Runs as a standalone app or as a VST3/AU plugin.

## Prerequisites

- **CMake** 3.22+
- **C++17** compiler (Clang on macOS)
- **JUCE 8** — clone or download to `~/Downloads/JUCE` (the CMake build expects it at
  `../../Downloads/JUCE` relative to the project root)
- **macOS** — required for the AVFoundation video background (Objective-C++ with ARC)

## Build

```bash
# Configure (first time only)
cmake -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug

# Build standalone app
cmake --build cmake-build-debug --target midi-visu_Standalone

# Build & run tests
cmake --build cmake-build-debug --target midi-visu-tests
cd cmake-build-debug && ctest --output-on-failure
```

The standalone app is output to:

```
cmake-build-debug/midi-visu_artefacts/Debug/Standalone/midi-visu.app
```

Other available plugin formats: **VST3**, **AU**.

## Keyboard Shortcuts

| Key   | Action               |
|-------|----------------------|
| `F`   | Toggle fullscreen    |
| `Esc` | Exit fullscreen      |
| `O`   | Toggle options panel |
| `L`   | Toggle log panel     |

Circles can be repositioned by dragging them with the mouse.

## MIDI Channel Mapping

| MIDI Channel | Role             | Visual Column |
|--------------|------------------|---------------|
| 10           | Drums (4 voices) | 0 (leftmost)  |
| 2            | Melodic track 1  | 1             |
| 3            | Melodic track 2  | 2             |
| 4            | Melodic track 3  | 3             |

Channel assignments are configurable via the options panel.

## Project Structure

```
src/
├── PluginProcessor.h/cpp      Audio processor and MIDI routing
├── PluginEditor.h/cpp         UI editor with timer-driven paint loop
├── VoiceManager.h/cpp         Channel assignment and note matching
├── MidiManager.h/cpp          MIDI atomic state and processBlock logic
├── DrawManager.h/cpp          All painting and rendering
├── InteractionManager.h/cpp   Mouse and keyboard input handling
├── StyleManager.h/cpp         JUCE colour/font accessors and control styling
├── StyleTokens.h              Visual style constants (colours, font sizes)
├── AppConstants.h             Shared voice colours and display names
├── VideoBackground.h/mm       AVFoundation video decoder (Obj-C++)
├── VideoListManager.h/cpp     Video file list, selection, and play state
├── RangeSlider.h/cpp          Two-handle horizontal range slider widget
└── RangeSliderLogic.h/cpp     Pure math for range slider hit-testing and dragging
tests/
├── main.cpp                   Test runner entry point
├── VoiceManagerTests.cpp      VoiceManager unit tests
├── MidiManagerTests.cpp       MidiManager unit tests
├── RangeSliderLogicTests.cpp  RangeSliderLogic unit tests
├── StyleTokensTests.cpp       StyleTokens unit tests
└── VideoListManagerTests.cpp  VideoListManager unit tests
```

For detailed class descriptions, dependencies, and implementation patterns, see
[ARCHITECTURE.md](ARCHITECTURE.md).

## License

All rights reserved.
