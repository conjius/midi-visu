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
class MidiVisuEditor  : public juce::AudioProcessorEditor,
                        private juce::Timer
{
public:
    explicit MidiVisuEditor (MidivisuAudioProcessor&);
    ~MidiVisuEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress& key) override;
    void mouseDown  (const juce::MouseEvent&) override;
    void mouseDrag  (const juce::MouseEvent&) override;
    void mouseUp    (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void timerCallback() override;

    MidivisuAudioProcessor& audioProcessor;
    VideoBackground         videoBackground;
    juce::Image             videoFrame;

    // Smoothed radii
    float smoothedRadius[4]     { 0.0f, 0.0f, 0.0f, 0.0f };
    float drumSmoothedRadius[4] { 0.0f, 0.0f, 0.0f, 0.0f };
    int   lastDrumHitCount[4]   { 0, 0, 0, 0 };
    static constexpr float minRadius      = 20.0f;
    static constexpr float maxRadius      = 280.0f;
    static constexpr float smoothing      = 0.10f;
    static constexpr float drumRestRadius = 30.0f;
    static constexpr float drumHitRadius  = 100.0f;
    static constexpr float drumSmoothing  = 0.1f;

    // Log panel
    bool                      logPanelOpen    = false;
    juce::StringArray         logLines;
    juce::Array<juce::Colour> logColors;
    int                       logScrollOffset = 0;
    int                       lastCh10RawHitCount       = 0;
    int                       lastChannelNoteOnCount[4] = { 0, 0, 0, 0 };
    static constexpr int maxLogLines   = 500;
    static constexpr int logPanelWidth = 300;

    // Circle positions (0-3 = drums, 4-6 = melodic tracks 1-3)
    juce::Point<float> circlePos[7];
    int                draggedCircle = -1;
    juce::Point<float> dragOffset;

    void initDefaultPositions();
    void writePositionsToFile (const juce::File&);
    void readPositionsFromFile (const juce::File&);
    void savePositions();   // opens "Save as…" dialog
    void loadPositions();   // opens "Load" dialog
    juce::File getAutoSaveFile() const;
    juce::File lastPositionsFile;
    std::unique_ptr<juce::FileChooser> fileChooser;

    // Video file selection
    juce::File lastVideoFile;
    std::unique_ptr<juce::FileChooser> videoChooser;
    void chooseVideoFile();

    // Options panel (O key)
    bool               optionsPanelOpen = false;
    juce::ComboBox     voiceChannelBox[7];
    juce::ToggleButton videoToggle { "Video" };
    juce::ToggleButton blurToggle  { "Blur" };
    juce::Slider       blurSlider;
    juce::Slider       videoZoomSlider;
    juce::Slider       videoOpacitySlider;
    juce::TextButton   saveDefaultButton   { "Save" };
    juce::TextButton   savePositionsButton { "Save as..." };
    juce::TextButton   loadPositionsButton { "Load" };
    juce::TextButton   loadVideoButton     { "Load Video..." };
    juce::TextButton   midiSettingsButton  { "Settings" };

    void showJuceAudioSettings();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiVisuEditor)
};
