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
├── StyleTokens.h                  Visual style constants (colours, font sizes, layout spacing)
├── AppConstants.h                 Shared voice colours and display names
├── VideoBackground.h/mm           AVFoundation video decoder (Obj-C++)
├── VideoListManager.h/cpp         Video file list, selection, and play state
├── RangeSlider.h/cpp              Two-handle horizontal range slider widget
├── RangeSliderLogic.h/cpp         Pure math for range slider hit-testing and dragging
├── SeekBar.h/cpp                  Video timeline with loop and playhead handles
├── MultiHandleSliderLogic.h/cpp   Pure math for three-handle slider
├── OptionsPanelLayout.h/cpp       Pure math for options panel section folding and scrolling
├── SvgWobbleLogic.h/cpp           Pure math for wobble deformation of vertex arrays
└── SvgShapeManager.h/cpp          SVG shape loading, vertex extraction, deformed path rendering
assets/
└── svg/                           SVG shape files (shape_0.svg..shape_6.svg); optional
tests/
├── main.cpp                       Test runner entry point
├── VoiceManagerTests.cpp          VoiceManager unit tests
├── MidiManagerTests.cpp           MidiManager unit tests
├── RangeSliderLogicTests.cpp      RangeSliderLogic unit tests
├── StyleTokensTests.cpp           StyleTokens unit tests
├── VideoListManagerTests.cpp      VideoListManager unit tests
├── MultiHandleSliderLogicTests.cpp MultiHandleSliderLogic unit tests
├── OptionsPanelLayoutTests.cpp    OptionsPanelLayout unit tests
└── SvgWobbleLogicTests.cpp        SvgWobbleLogic unit tests
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
and state, and delegates painting and input to dedicated managers. Also implements
`SeekBar::Listener` for video seek/loop callbacks.

- Owns: `UiManager`, `InteractionManager`, `StyleManager`, `VideoBackground`,
  `VideoListManager`, `RangeSlider`, `SeekBar`, `OptionsPanelLayout`
- Reads from: `MidivisuAudioProcessor` (via `audioProcessor` reference to access
  `MidiManager` / `VoiceManager` atomics)
- Defines: `PassthroughComboBox` — `ComboBox` subclass that forwards mouse wheel events
  to the parent instead of capturing them

**`UiManager`** — Renders all visual elements (circles, log panel, options panel, video
frame) by reading editor state as a friend class.

- Uses: `MidiVisuEditor` (friend access), `AppConstants`, `StyleManager`,
  `OptionsPanelLayout`, `StyleTokens`

**`InteractionManager`** — Handles all keyboard shortcuts, mouse dragging of circles, and
scroll events by mutating editor state as a friend class.

- Uses: `MidiVisuEditor` (friend access), `OptionsPanelLayout`

**`StyleManager`** — JUCE-aware wrapper that converts `StyleTokens` constants into
`Colour` and `Font` objects and applies dark styling to JUCE controls.

- Uses: `StyleTokens`

**`StyleTokens`** — Header-only namespace containing all visual style constants: ARGB
colours for panels, sliders, buttons, combo boxes, seek bar, and log; font sizes; and
layout spacing constants (`kPadding`, `kRowHeight`, `kListRowHeight`, `kSliderHeight`,
`kButtonHeight`, `kScrollbarW`, etc.).

- Dependencies: none (no JUCE)

**`AppConstants`** — Header-only file defining shared voice colours, channel colours,
display names, and default channel assignments.

- Dependencies: JUCE (`Colour`)

### Video

**`VideoBackground`** — Decodes video frames via AVFoundation (Objective-C++) and exposes
raw BGRA pixel buffers, deliberately avoiding any JUCE headers to prevent Objective-C type
conflicts. Provides playback control (`play`, `pause`, `stop`, `seek`), loop points
(`setLoopPoints`), time queries (`currentTime`, `duration`), and a static `fileDuration`
probe for files not yet loaded.

- Dependencies: AVFoundation (macOS system framework)

**`VideoListManager`** — Pure C++ class managing a list of video file entries (filename +
duration), the selected index, and play/pause/stop state.

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
- **Loop handles**: drawn as full-size edge markers with timestamp labels below.
  Draggable regardless of loop enabled state.
- **Loop toggle**: `setLoopEnabled(bool)` controls visual appearance — when disabled,
  loop handles and region are drawn greyed out but remain interactive.
- **Listener interface**: `SeekBar::Listener` with `seekBarLoopChanged` and
  `seekBarPlayheadDragged` callbacks.
- Uses an internal `RangeSlider` as a visual-only element for loop handle rendering.
- `kBarHeight = 40` (24px track area + 16px for loop time labels below).

- Uses: `MultiHandleSliderLogic`, `RangeSlider`

**`MultiHandleSliderLogic`** — Pure C++ math for a three-handle slider (loop start,
playhead, loop end): hit testing, handle dragging, middle-zone dragging, and time
formatting.

- Dependencies: none (no JUCE)

**`SvgWobbleLogic`** — Pure C++ class that computes radial wobble deformation of
vertex arrays. Stores per-voice state (phase, amplitude, frequency) and deforms
vertices using 3 sine harmonics for organic shape distortion.

- Dependencies: none (no JUCE)
- `Vertex` struct: `origX, origY, angle, radius` (polar coords from centroid)
- `VoiceState` struct: `phase, amplitude, targetAmplitude, frequency`
- `buildVertices`: compute centroid + polar coords from flat xy array
- `deformVertices`: radial displacement using 3 sine harmonics
- `advanceState`: increment phase by `frequency * 2π * dt`, wrap at 2π
- `triggerDrumHit`: snap amplitude to 1.0, target to 0.0
- `updateMelodicWobble`: map MIDI note 0-127 to frequency [2Hz..8Hz]
- `decayAmplitude`: smooth-step amplitude toward target

**`SvgShapeManager`** — JUCE-dependent class that loads SVG shape files, extracts
vertex arrays, and renders deformed paths using `SvgWobbleLogic`.

- Uses: `SvgWobbleLogic`
- `loadShapes`: load `shape_0.svg`..`shape_6.svg` from `assets/svg/`, fallback to
  32-vertex circle polygon if missing
- `drawShape`: deform vertices → build Path → `g.fillPath()`
- `extractVertices`: walk `Path::Iterator`, record lineTo endpoints, subdivide
  cubicTo curves (~4 segments each)
- Pre-allocated `deformBuffer_` avoids per-frame heap allocation

**`OptionsPanelLayout`** — Pure C++ layout engine that computes Y positions for all
options panel sections given fold states and scroll offset.

- Uses: `StyleTokens` (for `kPadding`)

## Options panel layout

Layout Y positions for the options panel are computed by `OptionsPanelLayout`
(pure C++). Sections can be folded/expanded; the panel supports vertical scrolling
with a thin scrollbar on the right side.

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

## Panel layout

- **Log panel**: 300px overlay on the **left** side of the window. Scrollbar on the left
  edge.
- **Options panel**: 300px overlay on the **right** side of the window. Scrollbar on the
  right edge.

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

## SVG wobble deformation

Each voice renders as a filled SVG shape (or fallback circle polygon) that deforms
organically in response to MIDI input.

**Wobble algorithm** — per-vertex pseudo-noise radial displacement:

Each vertex has a unique `noiseSeed` (derived from its index using the golden ratio)
and is displaced radially by a noise function built from 4 sine waves with
irrational frequency ratios:

```
noise(seed, phase, slowPhase) =
    sin(seed * 1.0  + phase * 0.7  + slowPhase * 0.3)  * 1.0
  + sin(seed * 1.618 + phase * 1.3 + slowPhase * 0.7) * 0.7
  + sin(seed * 2.732 + phase * 0.4 + slowPhase * 1.1) * 0.5
  + sin(seed * 0.577 + phase * 2.1 + slowPhase * 0.2) * 0.3
  / 2.5 (normalised)

displacement = wobbleAmount * amplitude * radius * kMaxDisplacement * noise
```

- `wobbleAmount` — global intensity from the "Wobble" slider (0..1)
- `amplitude` — per-voice, decays toward `targetAmplitude` each frame
- `radius` — distance from centroid to original vertex position
- `kMaxDisplacement` — 0.4 (40% max radial displacement)
- `phase` — advances at `frequency` Hz; `slowPhase` always drifts at 0.13 Hz
- The irrational ratios (phi, sqrt(e), 1/sqrt(3)) prevent periodicity

**Melodic voices** — wobble speed tied to playing note's pitch:
- Note 0 → 0.5 Hz, Note 127 → 2.5 Hz (slow, organic)
- Amplitude → 1.0 while note is active, decays to 0.0 when released

**Drum voices** — wobble on hit and decay:
- On hit: amplitude snaps to 1.0, target set to 0.0 (decays immediately)

**SVG loading** — `shape_0.svg`..`shape_6.svg` from `assets/svg/`. Missing files
fall back to a 32-vertex regular polygon (circle approximation). Vertices are
extracted once at load time; deformation happens per-frame via `SvgWobbleLogic`.

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
