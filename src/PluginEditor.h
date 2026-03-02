/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "VideoBackground.h"

//==============================================================================
class MidiVisuEditor  : public AudioProcessorEditor,
                        private Timer
{
public:
    explicit MidiVisuEditor (MidivisuAudioProcessor&);
    ~MidiVisuEditor() override;

    //==============================================================================
    void paint (Graphics&) override;
    void paintOverChildren (Graphics&) override;
    void resized() override;
    bool keyPressed (const KeyPress& key) override;
    void mouseDown  (const MouseEvent&) override;
    void mouseDrag  (const MouseEvent&) override;
    void mouseUp    (const MouseEvent&) override;
    void mouseWheelMove (const MouseEvent&, const MouseWheelDetails&) override;

private:
    void timerCallback() override;

    MidivisuAudioProcessor& audioProcessor;
    VideoBackground         videoBackground;
    Image             videoFrame;

    // Smoothed radii
    float smoothedRadius[4]     { 0.0f, 0.0f, 0.0f, 0.0f };
    float drumSmoothedRadius[4] { 0.0f, 0.0f, 0.0f, 0.0f };
    int   lastDrumHitCount[4]   { 0, 0, 0, 0 };
    static constexpr float smoothing    = 0.10f;
    static constexpr float drumSmoothing = 0.1f;

    // Clock sync
    int lastClockPulse = 0;

    // Log panel
    bool                      logPanelOpen    = false;
    StringArray         logLines;
    Array<Colour> logColors;
    int                       logScrollOffset = 0;
    int                       lastCh10RawHitCount       = 0;
    int                       lastChannelNoteOnCount[4] = { 0, 0, 0, 0 };
    static constexpr int maxLogLines   = 500;
    static constexpr int logPanelWidth = 300;

    // Circle positions (0-3 = drums, 4-6 = melodic tracks 1-3)
    Point<float> circlePos[7];
    // Brownian float
    Point<float> floatOffset[7];
    Point<float> floatVel[7];
    int                draggedCircle = -1;
    Point<float> dragOffset;

    void initDefaultPositions();
    void writePositionsToFile (const File&);
    void readPositionsFromFile (const File&);
    void savePositions();   // opens "Save as…" dialog
    void loadPositions();   // opens "Load" dialog
    File getAutoSaveFile() const;
    File lastPositionsFile;
    std::unique_ptr<FileChooser> fileChooser;

    // Video file selection
    File lastVideoFile;
    std::unique_ptr<FileChooser> videoChooser;
    void chooseVideoFile();

    // Options panel (O key)
    bool               optionsPanelOpen = false;
    ComboBox     voiceChannelBox[7];
    ComboBox     clockDivisionBox;
    ToggleButton videoToggle { "Video" };
    ToggleButton blurToggle  { "Blur" };
    ToggleButton floatToggle      { "Floating" };
    ToggleButton collisionToggle  { " Collisions" };
    ToggleButton clockKickToggle  { "Clock Sync" };

    // Ball masses — all equal for now, prepared for per-ball tuning.
    float ballMass[7] = { 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f };
    Slider       blurSlider;
    Slider       videoZoomSlider;
    Slider       videoOpacitySlider;
    Slider       ballSizeSlider;          // TwoValueHorizontal range slider
    Slider       floatIntensitySlider;
    Slider       floatSpeedSlider;
    Slider       clockKickIntensitySlider;
    TextButton   saveDefaultButton   { "Save Settings" };
    TextButton   savePositionsButton { "Save Settings to..." };
    TextButton   loadPositionsButton { "Load" };
    TextButton   loadVideoButton     { "Load Video..." };
    TextButton   midiSettingsButton  { "Audio / MIDI Settings" };

    void showJuceAudioSettings();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiVisuEditor)
};
