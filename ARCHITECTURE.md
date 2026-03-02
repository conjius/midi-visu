# midi-visu — Architecture Notes

## Project Structure

```
src/
├── PluginProcessor.h/cpp          Audio processor and MIDI routing
├── PluginEditor.h/cpp             UI editor with timer-driven paint loop
├── VoiceManager.h/cpp             Channel assignment and note matching
├── MidiManager.h/cpp              MIDI atomic state and processBlock logic
├── UiManager.h/cpp                All painting and rendering
├── InteractionManager.h/cpp       Mouse and keyboard input handling
├── StyleManager.h/cpp             JUCE colour/font accessors and control styling
├── StyleTokens.h                  Visual style constants (colours, font sizes)
├── AppConstants.h                 Shared voice colours and display names
├── VideoBackground.h/mm           AVFoundation video decoder (Obj-C++)
├── VideoListManager.h/cpp         Video file list, selection, and play state
├── RangeSlider.h/cpp              Two-handle horizontal range slider widget
├── RangeSliderLogic.h/cpp         Pure math for range slider hit-testing and dragging
├── SeekBar.h/cpp                  Video timeline with loop and playhead handles
├── MultiHandleSliderLogic.h/cpp   Pure math for three-handle slider
└── OptionsPanelLayout.h/cpp       Pure math for options panel section folding and scrolling
tests/
├── main.cpp                       Test runner entry point
├── VoiceManagerTests.cpp          VoiceManager unit tests
├── MidiManagerTests.cpp           MidiManager unit tests
├── RangeSliderLogicTests.cpp      RangeSliderLogic unit tests
├── StyleTokensTests.cpp           StyleTokens unit tests
├── VideoListManagerTests.cpp      VideoListManager unit tests
├── MultiHandleSliderLogicTests.cpp MultiHandleSliderLogic unit tests
└── OptionsPanelLayoutTests.cpp    OptionsPanelLayout unit tests
```

## Class Descriptions and Dependencies

### Audio Thread

**`MidivisuAudioProcessor`** — Top-level JUCE audio processor that owns the voice and MIDI
managers and delegates `processBlock` to `MidiManager`.

- Owns: `VoiceManager`, `MidiManager`

**`MidiManager`** — Processes incoming MIDI messages on the audio thread, updating atomic
counters for drum hits, melodic note tracking, and clock pulses.

- Uses: `VoiceManager` (to resolve channel/note → voice index)

**`VoiceManager`** — Pure C++ class that stores MIDI channel assignments and provides
static methods to match a channel/note pair to a drum or melodic voice index.

- Dependencies: none (no JUCE)

### UI Thread

**`MidiVisuEditor`** — Main editor component that runs a 60 Hz timer, owns all UI widgets
and state, and delegates painting and input to dedicated managers.

- Owns: `UiManager`, `InteractionManager`, `StyleManager`, `VideoBackground`,
  `VideoListManager`, `RangeSlider`, `SeekBar`, `OptionsPanelLayout`
- Reads from: `MidivisuAudioProcessor` (via `audioProcessor` reference to access
  `MidiManager` / `VoiceManager` atomics)

**`UiManager`** — Renders all visual elements (circles, log panel, options panel, video
frame) by reading editor state as a friend class.

- Uses: `MidiVisuEditor` (friend access), `AppConstants`, `StyleManager`

**`InteractionManager`** — Handles all keyboard shortcuts, mouse dragging of circles, and
scroll events by mutating editor state as a friend class.

- Uses: `MidiVisuEditor` (friend access)

**`StyleManager`** — JUCE-aware wrapper that converts `StyleTokens` constants into
`Colour` and `Font` objects and applies dark styling to JUCE controls.

- Uses: `StyleTokens`

**`StyleTokens`** — Header-only namespace containing all visual style constants (ARGB
colours, font sizes) with no JUCE dependency.

- Dependencies: none (no JUCE)

**`AppConstants`** — Header-only file defining shared voice colours, channel colours,
display names, and default channel assignments.

- Dependencies: JUCE (`Colour`)

### Video

**`VideoBackground`** — Decodes video frames via AVFoundation (Objective-C++) and exposes
raw BGRA pixel buffers, deliberately avoiding any JUCE headers to prevent Objective-C type
conflicts.

- Dependencies: AVFoundation (macOS system framework)

**`VideoListManager`** — Pure C++ class managing a list of video filenames, the selected
index, and play/pause/stop state.

- Dependencies: none (no JUCE)

### Widgets

**`RangeSlider`** — Custom JUCE slider subclass implementing a two-handle horizontal range
control for setting min/max ball sizes.

- Uses: `RangeSliderLogic`

**`RangeSliderLogic`** — Pure C++ math utilities for value-to-pixel conversion,
middle-zone hit testing, and clamped drag translation in a range slider.

- Dependencies: none (no JUCE)

**`SeekBar`** — Custom JUCE Component implementing a video timeline with playhead and
loop handles. Features:

- **Click-to-seek**: clicking empty track area moves playhead to that position.
- **Elapsed fill**: brighter track fill from 0 to the playhead position.
- **Loop handles**: drawn as full-size edge markers (matching RangeSlider style) with
  timestamp labels below. Draggable regardless of loop enabled state.
- **Loop toggle**: `setLoopEnabled(bool)` controls visual appearance — when disabled,
  loop handles and region are drawn greyed out but remain interactive.
- `kBarHeight = 40` (24px track area + 16px for loop time labels below).

- Uses: `MultiHandleSliderLogic`

**`MultiHandleSliderLogic`** — Pure C++ math for a three-handle slider: hit testing,
handle dragging, middle-zone dragging, and time formatting.

- Dependencies: none (no JUCE)

**`OptionsPanelLayout`** — Pure C++ layout engine that computes Y positions for all
options panel sections given fold states and scroll offset.

- Dependencies: none (no JUCE)

## Options panel layout

Layout Y positions for the options panel are computed by `OptionsPanelLayout`
(pure C++, no JUCE). Sections can be folded/expanded; the panel supports vertical
scrolling with a thin scrollbar on the right side.

**Sections** (in order): MIDI ROUTING, VIDEO, CIRCLES, ANIMATION.

- Clicking a section header toggles fold state; a triangle indicator shows state
  (▼ expanded, ▶ collapsed).
- When folded, section content (both painted elements and JUCE Component widgets) is
  hidden. Subsequent sections shift up to fill the gap.
- Mouse wheel scrolls the entire panel; scroll offset is clamped to `[0, maxScroll]`.
- `resized()` and `UiManager::paint()` both query `OptionsPanelLayout` for Y positions,
  ensuring widget bounds and painted elements stay synchronized.
- JUCE child Components are repositioned via `setBounds()` on each scroll/fold change;
  components scrolled outside the viewport are set invisible.

## Log panel

- Toggle with `L` key
- 300px overlay on left side, newest entry first
- `mouseWheelMove` for scrolling (deltaY * 30 lines)
- Entries added continuously to `logLines` (StringArray, max 500) regardless of panel
  open state
- Scrollbar on the left side of the panel

## JUCE

- Location: `~/Downloads/JUCE` (added via `add_subdirectory` in CMakeLists.txt)
- Version: JUCE 8.0.12 — use modern APIs:
    - `Font(FontOptions().withName(...).withHeight(...))` — old `Font(name, size, style)`
      constructor is deprecated
    - Mouse wheel override is `mouseWheelMove` (not `mouseWheelMoved`)

## Drum kit (channel 10)

Column 0 shows 4 stacked circles, one per voice, kick at bottom:

| Voice | Name  | MIDI note | Color               |
|-------|-------|-----------|---------------------|
| 0     | Kick  | C3 (48)   | `#32cd32` limegreen |
| 1     | Noise | D3 (50)   | `#27a427`           |
| 2     | Snare | E3 (52)   | `#1d7b1d`           |
| 3     | HH    | F#3 (54)  | `#125212`           |

## Drum animation

Uses **hit-counter** approach (not sustained note state):

- `drumVoiceHitCount[4]` atomics in processor — incremented on each note-on
- Editor compares counter to `lastDrumHitCount[v]` each frame; any delta → snap
  `drumSmoothedRadius[v]` to max radius, then decay at `drumSmoothing = 0.1f/frame`
- Reason: drum note-off arrives in the same audio block as note-on, so sustained-state
  approach always reads -1 at 60Hz

## Friend-class pattern

- `InteractionManager` and `UiManager` are friends of `MidiVisuEditor`
- Their headers forward-declare `class MidiVisuEditor;` — no circular includes
- Their `.cpp` files include `PluginEditor.h` to get the full type

## Atomic access from UI thread

- `audioProcessor.midiManager.drumVoiceHitCount[v]`
- `audioProcessor.voiceManager.getMelodicChannel(i)`

## Testing

- Framework: JUCE `UnitTest` with static registration
- Test names must use ASCII dashes (not em-dashes) to avoid JUCE String assertions
- Pure-C++ classes are tested: VoiceManager, MidiManager, RangeSliderLogic, StyleTokens,
  VideoListManager, MultiHandleSliderLogic, OptionsPanelLayout

## Known quirks

- IDE (clangd) shows many false-positive errors (`JuceHeader.h` not found, `juce::`
  undeclared) — these are clangd config issues, not real errors. Trust the cmake build
  output.
- `juce_MidiMessageCollector.cpp:71` assertion suppressed (commented out) — fires in
  standalone when MIDI source sends messages with timestamp 0.0
