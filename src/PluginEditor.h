/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "VideoBackground.h"
#include "StyleManager.h"
#include "InteractionManager.h"
#include "UiManager.h"
#include "RangeSlider.h"
#include "VideoListManager.h"
#include "SeekBar.h"
#include "OptionsPanelLayout.h"

class InvertedSlider : public Slider {
public:
    using Slider::Slider;
    void mouseWheelMove(const MouseEvent& e,
                        const MouseWheelDetails& w) override {
        if (!isMouseButtonDown()) return;
        auto reversed = w;
        reversed.deltaY = -reversed.deltaY;
        Slider::mouseWheelMove(e, reversed);
    }
};

//==============================================================================
class MidiVisuEditor : public AudioProcessorEditor,
                       private Timer,
                       public SeekBar::Listener {
public:
    explicit MidiVisuEditor(MidivisuAudioProcessor&);
    ~MidiVisuEditor() override;

    //==============================================================================
    void paint(Graphics&) override;
    void paintOverChildren(Graphics&) override;
    void resized() override;
    bool keyPressed(const KeyPress& key) override;
    void mouseDown(const MouseEvent&) override;
    void mouseDrag(const MouseEvent&) override;
    void mouseUp(const MouseEvent&) override;
    void mouseWheelMove(const MouseEvent&, const MouseWheelDetails&) override;

private:
    void timerCallback() override;

    MidivisuAudioProcessor& audioProcessor;
    VideoBackground videoBackground;
    Image videoFrame;

    // Smoothed radii
    float smoothedRadius[4]{0.0f, 0.0f, 0.0f, 0.0f};
    float drumSmoothedRadius[4]{0.0f, 0.0f, 0.0f, 0.0f};
    int lastDrumHitCount[4]{0, 0, 0, 0};
    static constexpr float smoothing = 0.10f;
    static constexpr float drumSmoothing = 0.1f;

    // Clock sync
    int lastClockPulse = 0;

    // Log panel
    bool logPanelOpen = false;
    StringArray logLines;
    Array<Colour> logColors;
    int logScrollOffset = 0;
    int lastCh10RawHitCount = 0;
    int lastChannelNoteOnCount[4] = {0, 0, 0, 0};
    static constexpr int maxLogLines = 500;
    static constexpr int logPanelWidth = 300;

    // Circle positions (0-3 = drums, 4-6 = melodic tracks 1-3)
    Point<float> circlePos[7];
    // Brownian float
    Point<float> floatOffset[7];
    Point<float> floatVel[7];

    // Ball masses — all equal for now, prepared for per-ball tuning.
    float ballMass[7] = {1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f};

    void initDefaultPositions();
    void writePositionsToFile(const File&) const;
    void readPositionsFromFile(const File&);
    void savePositions(); // opens "Save as…" dialog
    void loadPositions(); // opens "Load" dialog
    File getAutoSaveFile() const;
    File lastPositionsFile;
    std::unique_ptr<FileChooser> fileChooser;

    // Video file selection
    File lastVideoFile;
    File assetsVideoDir;
    VideoListManager videoListManager;
    int videoListScrollOffset = 0;

    void appendLog(const String& text, Colour color);

    // Options panel (O key)
    bool optionsPanelOpen = false;
    String voiceNames[7];
    Label voiceNameLabel[7];
    ComboBox voiceChannelBox[7];
    ComboBox clockDivisionBox;
    ToggleButton videoToggle{"Video"};
    ToggleButton blurToggle{"Blur"};
    ToggleButton floatToggle{"Floating"};
    ToggleButton collisionToggle{" Collisions"};
    ToggleButton clockKickToggle{"Clock Sync"};
    ToggleButton logMidiNotesToggle{"MIDI notes"};
    ToggleButton logMidiClockToggle{"MIDI clock"};
    TextButton clearLogButton{"Clear"};

    InvertedSlider blurSlider;
    InvertedSlider videoZoomSlider;
    InvertedSlider videoOpacitySlider;
    RangeSlider ballSizeSlider;
    InvertedSlider floatIntensitySlider;
    InvertedSlider floatSpeedSlider;
    InvertedSlider clockKickIntensitySlider;
    TextButton saveDefaultButton{"Save Settings"};
    TextButton savePositionsButton{"Save Settings to..."};
    TextButton loadPositionsButton{"Load"};
    TextButton videoPlayPauseButton{CharPointer_UTF8("\xe2\x96\xb6")};       // ▶
    TextButton videoStopButton{CharPointer_UTF8("\xe2\x96\xa0")};           // ■
    TextButton loopButton{CharPointer_UTF8("\xe2\x86\xbb")};               // ↻
    TextButton midiSettingsButton{"Audio / MIDI Settings"};
    SeekBar seekBar;

    void seekBarLoopChanged(SeekBar* bar) override;
    void seekBarPlayheadDragged(SeekBar* bar) override;

    void showJuceAudioSettings();

    // Options panel layout — fold state and scroll offset.
    OptionsPanelLayout optionsLayout;

    // Style — declared before interaction/draw managers so they can access it.
    StyleManager styleManager;

    // Interaction and drawing managers — declared after all state they reference.
    InteractionManager interactionManager;
    UiManager uiManager;

    // Pending loop values from settings, applied after maxValue becomes positive.
    double pendingLoopStart_ = -1.0;
    double pendingLoopEnd_ = -1.0;

    friend class InteractionManager;
    friend class UiManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiVisuEditor)
};