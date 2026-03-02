/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "AppConstants.h"
#include "StyleTokens.h"

#if JUCE_STANDALONE_APPLICATION
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif

//==============================================================================
static String noteName(int n) {
    return MidiMessage::getMidiNoteName(n, true, true, 4);
}

void MidiVisuEditor::appendLog(const String& text, Colour color) {
    logLines.insert(0, text);
    logColors.insert(0, color);
    while (logLines.size() > maxLogLines) {
        logLines.remove(logLines.size() - 1);
        logColors.remove(logColors.size() - 1);
    }
}

//==============================================================================
MidiVisuEditor::MidiVisuEditor(MidivisuAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      interactionManager(*this), uiManager(*this) {
    setWantsKeyboardFocus(true);

    // Voice channel ComboBoxes (hidden until options panel opens).
    for (int i = 0; i < 7; ++i) {
        for (int ch = 1; ch <= 16; ++ch)
            voiceChannelBox[i].addItem(String(ch), ch);

        const int defaultCh = (i < 4) ? kDefaultDrumCh[i] : kDefaultMelodicCh[i - 4];
        voiceChannelBox[i].setSelectedId(defaultCh, dontSendNotification);

        voiceChannelBox[i].onChange = [this, i] {
            const int ch = voiceChannelBox[i].getSelectedId();
            if (i < 4)
                audioProcessor.voiceManager.setDrumChannel(i, ch);
            else
                audioProcessor.voiceManager.setMelodicChannel(i - 4, ch);
            appendLog(voiceNames[i] + " -> ch" + String(ch), styleManager.logInfo());
        };

        addChildComponent(voiceChannelBox[i]);
    }

    videoToggle.setToggleState(true, dontSendNotification);
    addChildComponent(videoToggle);

    blurToggle.setToggleState(true, dontSendNotification);
    blurSlider.setRange(0.0, 40.0, 0.5);
    blurSlider.setValue(12.0, dontSendNotification);
    blurSlider.setSliderStyle(Slider::LinearHorizontal);
    blurSlider.setTextBoxStyle(Slider::TextBoxRight, false, 46, 20);
    addChildComponent(blurToggle);
    addChildComponent(blurSlider);

    videoZoomSlider.setRange(1.0, 8.0, 0.1);
    videoZoomSlider.setValue(2.0, dontSendNotification);
    videoZoomSlider.setSliderStyle(Slider::LinearHorizontal);
    videoZoomSlider.setTextBoxStyle(Slider::TextBoxRight, false, 46, 20);
    addChildComponent(videoZoomSlider);

    videoOpacitySlider.setRange(0.0, 1.0, 0.01);
    videoOpacitySlider.setValue(0.5, dontSendNotification);
    videoOpacitySlider.setSliderStyle(Slider::LinearHorizontal);
    videoOpacitySlider.setTextBoxStyle(Slider::TextBoxRight, false, 46, 20);
    addChildComponent(videoOpacitySlider);

    // Clock division: id == pulse count per division (24 PPQ)
    clockDivisionBox.addItem("1    (whole)", 96);
    clockDivisionBox.addItem("1/2  (half)", 48);
    clockDivisionBox.addItem("1/4  (quarter)", 24);
    clockDivisionBox.addItem("1/8  (eighth)", 12);
    clockDivisionBox.addItem("1/16 (sixteenth)", 6);
    clockDivisionBox.addItem("1/32 (32nd)", 3);
    clockDivisionBox.setSelectedId(24, dontSendNotification);
    addChildComponent(clockDivisionBox);

    ballSizeSlider.setRange(5.0, 500.0, 1.0);
    ballSizeSlider.setMinAndMaxValues(20.0, 280.0, dontSendNotification);
    ballSizeSlider.setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
    addChildComponent(ballSizeSlider);

    clockKickIntensitySlider.setRange(0.0, 2.0, 0.01);
    clockKickIntensitySlider.setValue(1.0, dontSendNotification);
    clockKickIntensitySlider.setSliderStyle(Slider::LinearHorizontal);
    clockKickIntensitySlider.setTextBoxStyle(Slider::TextBoxRight, false, 46, 20);
    addChildComponent(clockKickIntensitySlider);

    floatToggle.setToggleState(true, dontSendNotification);
    collisionToggle.setToggleState(true, dontSendNotification);
    clockKickToggle.setToggleState(true, dontSendNotification);
    addChildComponent(collisionToggle);
    addChildComponent(clockKickToggle);
    floatIntensitySlider.setRange(0.0, 1.0, 0.01);
    floatIntensitySlider.setValue(0.5, dontSendNotification);
    floatIntensitySlider.setSliderStyle(Slider::LinearHorizontal);
    floatIntensitySlider.setTextBoxStyle(Slider::TextBoxRight, false, 46, 20);
    floatSpeedSlider.setRange(0.0, 1.0, 0.01);
    floatSpeedSlider.setValue(0.3, dontSendNotification);
    floatSpeedSlider.setSliderStyle(Slider::LinearHorizontal);
    floatSpeedSlider.setTextBoxStyle(Slider::TextBoxRight, false, 46, 20);
    addChildComponent(floatToggle);
    addChildComponent(floatIntensitySlider);
    addChildComponent(floatSpeedSlider);

    videoToggle.onClick = [this] {
        appendLog(videoToggle.getToggleState() ? "Video: ON" : "Video: OFF",
                  styleManager.logInfo());
    };
    blurToggle.onClick = [this] {
        appendLog(blurToggle.getToggleState() ? "Blur: ON" : "Blur: OFF",
                  styleManager.logInfo());
    };
    floatToggle.onClick = [this] {
        appendLog(floatToggle.getToggleState() ? "Floating: ON" : "Floating: OFF",
                  styleManager.logInfo());
    };
    collisionToggle.onClick = [this] {
        appendLog(collisionToggle.getToggleState() ? "Collisions: ON" : "Collisions: OFF",
                  styleManager.logInfo());
    };
    clockKickToggle.onClick = [this] {
        appendLog(clockKickToggle.getToggleState() ? "Clock sync: ON" : "Clock sync: OFF",
                  styleManager.logInfo());
    };

    clockDivisionBox.onChange = [this] {
        appendLog("Clock div: " + clockDivisionBox.getText(), styleManager.logInfo());
    };

    saveDefaultButton.onClick = [this] {
        writePositionsToFile(getAutoSaveFile());
        appendLog("Save Settings", styleManager.logInfo());
    };
    savePositionsButton.onClick = [this] {
        savePositions();
        appendLog("Save Settings to...", styleManager.logInfo());
    };
    loadPositionsButton.onClick = [this] {
        loadPositions();
        appendLog("Load positions", styleManager.logInfo());
    };
    midiSettingsButton.onClick = [this] {
        showJuceAudioSettings();
        appendLog("Audio/MIDI settings", styleManager.logInfo());
    };
    addChildComponent(saveDefaultButton);
    addChildComponent(savePositionsButton);
    addChildComponent(loadPositionsButton);
    addChildComponent(midiSettingsButton);

    // Video playback controls
    seekBar.addListener(this);
    addChildComponent(seekBar);

    videoPlayPauseButton.onClick = [this] {
        if (videoListManager.isPlaying()) {
            videoBackground.pause();
            videoListManager.setPlayState(VideoListManager::PlayState::Paused);
            videoPlayPauseButton.setButtonText(CharPointer_UTF8("\xe2\x96\xb6"));
            appendLog("Video: paused", styleManager.logInfo());
        }
        else {
            videoBackground.play();
            videoListManager.setPlayState(VideoListManager::PlayState::Playing);
            videoPlayPauseButton.setButtonText(CharPointer_UTF8("\xe2\x8f\xb8"));
            appendLog("Video: playing", styleManager.logInfo());
        }
        repaint();
    };
    videoStopButton.onClick = [this] {
        videoBackground.stop();
        videoListManager.setPlayState(VideoListManager::PlayState::Stopped);
        videoPlayPauseButton.setButtonText(CharPointer_UTF8("\xe2\x96\xb6"));
        appendLog("Video: stopped", styleManager.logInfo());
        repaint();
    };
    addChildComponent(videoPlayPauseButton);
    addChildComponent(videoStopButton);

    loopButton.onClick = [this] {
        const bool nowOn = !seekBar.isLoopEnabled();
        seekBar.setLoopEnabled(nowOn);
        loopButton.setColour(TextButton::buttonColourId,
                             Colour(nowOn ? StyleTokens::kButtonBgOn
                                         : StyleTokens::kButtonBg));
        if (nowOn)
            videoBackground.setLoopPoints(seekBar.getLoopStart(),
                                          seekBar.getLoopEnd());
        else
            videoBackground.setLoopPoints(0.0, videoBackground.duration());
        writePositionsToFile(getAutoSaveFile());
        repaint();
    };
    // Start with loop enabled — highlight the button
    loopButton.setColour(TextButton::buttonColourId,
                         Colour(StyleTokens::kButtonBgOn));
    addChildComponent(loopButton);

    // Disable scroll-wheel on all sliders so events bubble to the panel scroller.
    for (auto* s : {
             &blurSlider, &videoZoomSlider, &videoOpacitySlider,
             &clockKickIntensitySlider, &floatIntensitySlider, &floatSpeedSlider
         }) {
        s->setScrollWheelEnabled(false);
        styleManager.applyToSlider(*s);
    }
    ballSizeSlider.setScrollWheelEnabled(false);
    styleManager.applyToSlider(ballSizeSlider);

    for (auto* b : {
             &videoToggle, &blurToggle, &floatToggle, &collisionToggle, &clockKickToggle
         })
        styleManager.applyToToggleButton(*b);

    for (auto* b : {&logMidiNotesToggle, &logMidiClockToggle}) {
        styleManager.applyToToggleButton(*b);
        b->setToggleState(true, dontSendNotification);
        addChildComponent(*b);
    }

    clearLogButton.onClick = [this] {
        logLines.clear();
        logColors.clear();
        repaint();
    };
    styleManager.applyToTextButton(clearLogButton);
    addChildComponent(clearLogButton);

    for (auto* b : {
             &saveDefaultButton, &savePositionsButton, &loadPositionsButton,
             &videoPlayPauseButton, &videoStopButton, &loopButton, &midiSettingsButton
         })
        styleManager.applyToTextButton(*b);

    for (int i = 0; i < 7; ++i)
        styleManager.applyToComboBox(voiceChannelBox[i]);
    styleManager.applyToComboBox(clockDivisionBox);

    // Mutable voice names (initialised from compile-time defaults).
    for (int i = 0; i < 7; ++i)
        voiceNames[i] = kVoiceNames[i];

    // Inline-editable voice name labels (double-click to rename).
    for (int i = 0; i < 7; ++i) {
        voiceNameLabel[i].setText(voiceNames[i], dontSendNotification);
        voiceNameLabel[i].setFont(styleManager.headingFont());
        voiceNameLabel[i].setJustificationType(Justification::left);
        voiceNameLabel[i].setColour(Label::textColourId, styleManager.voiceName());
        voiceNameLabel[i].setColour(Label::backgroundColourId, Colours::transparentBlack);
        voiceNameLabel[i].setColour(Label::outlineColourId, Colours::transparentBlack);
        voiceNameLabel[i].setColour(Label::textWhenEditingColourId,
                                    styleManager.voiceName());
        voiceNameLabel[i].setColour(Label::backgroundWhenEditingColourId,
                                    Colour(StyleTokens::kSliderBg));
        voiceNameLabel[i].setColour(Label::outlineWhenEditingColourId,
                                    styleManager.border());
        voiceNameLabel[i].setEditable(false, true, false);
        voiceNameLabel[i].onTextChange = [this, i] {
            const String oldName = voiceNames[i];
            voiceNames[i] = voiceNameLabel[i].getText();
            appendLog(oldName + " -> " + voiceNames[i], styleManager.logInfo());
            };
        addChildComponent(voiceNameLabel[i]);
    }

    // Scan assets/video/ and populate the video list
    assetsVideoDir = File(String(MIDI_VISU_ASSETS_DIR)).getChildFile("video");
    {
        Array<File> videoFiles;
        assetsVideoDir.findChildFiles(videoFiles, File::findFiles, false,
                                      "*.mp4;*.mov;*.m4v");
        videoFiles.sort();
        std::vector<VideoListManager::FileEntry> entries;
        for (const auto& f : videoFiles) {
            const double dur = VideoBackground::fileDuration(
                f.getFullPathName().toRawUTF8());
            entries.push_back({f.getFileName().toStdString(), dur});
        }
        videoListManager.setFiles(entries);
    }

    setSize(1920, 1080);
    initDefaultPositions();
    optionsPanelOpen = true;   // default for first run (overridden by saved state)
    readPositionsFromFile(getAutoSaveFile());
    resized();

    startTimerHz(60);
}

MidiVisuEditor::~MidiVisuEditor() {
    stopTimer();
}

//==============================================================================
// Delegate paint and input to manager classes.

void MidiVisuEditor::paint(Graphics& g) {
    uiManager.paint(g);
}

void MidiVisuEditor::paintOverChildren(Graphics& g) {
    uiManager.paintOverChildren(g);
}

bool MidiVisuEditor::keyPressed(const KeyPress& key) {
    return interactionManager.keyPressed(key);
}

void MidiVisuEditor::mouseDown(const MouseEvent& e) {
    interactionManager.mouseDown(e);
}

void MidiVisuEditor::mouseDrag(const MouseEvent& e) {
    interactionManager.mouseDrag(e);
}

void MidiVisuEditor::mouseUp(const MouseEvent& e) {
    interactionManager.mouseUp(e);
}

void MidiVisuEditor::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& w) {
    interactionManager.mouseWheelMove(e, w);
}

//==============================================================================
// Circle position helpers

void MidiVisuEditor::initDefaultPositions() {
    const float w = static_cast<float>(getWidth());
    const float h = static_cast<float>(getHeight());
    const float slotW = w / 4.0f;
    const float rowH = h / 4.0f;

    for (int v = 0; v < 4; ++v)
        circlePos[v] = {slotW * 0.5f, rowH * (3 - v) + rowH * 0.5f};

    for (int i = 1; i <= 3; ++i)
        circlePos[3 + i] = {slotW * i + slotW * 0.5f, h * 0.5f};
}

File MidiVisuEditor::getAutoSaveFile() const {
    auto dir = File::getSpecialLocation(File::userApplicationDataDirectory)
                   .getChildFile("MidiVisu");
    auto newFile = dir.getChildFile("settings.json");
    if (!newFile.existsAsFile()) {
        auto oldFile = dir.getChildFile("circle_positions.json");
        if (oldFile.existsAsFile())
            oldFile.moveFileTo(newFile);
    }
    return newFile;
}

void MidiVisuEditor::writePositionsToFile(const File& file) const {
    Array<var> arr;
    for (int i = 0; i < 7; ++i) {
        auto* entry = new DynamicObject();
        entry->setProperty("name", voiceNames[i]);
        entry->setProperty("x", (double)circlePos[i].x);
        entry->setProperty("y", (double)circlePos[i].y);
        arr.add(entry);
    }
    auto* settings = new DynamicObject();
    settings->setProperty("ballSizeMin", ballSizeSlider.getMinValue());
    settings->setProperty("ballSizeMax", ballSizeSlider.getMaxValue());
    settings->setProperty("videoEnabled", videoToggle.getToggleState());
    settings->setProperty("blurEnabled", blurToggle.getToggleState());
    settings->setProperty("blurRadius", blurSlider.getValue());
    settings->setProperty("videoZoom", videoZoomSlider.getValue());
    settings->setProperty("videoOpacity", videoOpacitySlider.getValue());
    settings->setProperty("floatEnabled", floatToggle.getToggleState());
    settings->setProperty("collisionEnabled", collisionToggle.getToggleState());
    settings->setProperty("floatIntensity", floatIntensitySlider.getValue());
    settings->setProperty("floatSpeed", floatSpeedSlider.getValue());
    settings->setProperty("clockDiv", clockDivisionBox.getSelectedId());
    settings->setProperty("clockKick", clockKickToggle.getToggleState());
    settings->setProperty("clockKickIntensity", clockKickIntensitySlider.getValue());
    settings->setProperty("logMidiNotes", logMidiNotesToggle.getToggleState());
    settings->setProperty("logMidiClock", logMidiClockToggle.getToggleState());
    settings->setProperty("loopStart", seekBar.getLoopStart());
    settings->setProperty("loopEnd", seekBar.getLoopEnd());
    settings->setProperty("loopEnabled", seekBar.isLoopEnabled());
    settings->setProperty("optionsPanelOpen", optionsPanelOpen);
    settings->setProperty("logPanelOpen", logPanelOpen);

    Array<var> foldArr;
    for (int s = 0; s < OptionsPanelLayout::SectionCount; ++s)
        foldArr.add(optionsLayout.isFolded(
            static_cast<OptionsPanelLayout::Section>(s)));
    settings->setProperty("foldStates", foldArr);

    Array<var> midiChArr;
    for (int i = 0; i < 7; ++i) midiChArr.add(voiceChannelBox[i].getSelectedId());
    settings->setProperty("voiceChannels", midiChArr);

    Array<var> namesArr;
    for (int i = 0; i < 7; ++i) namesArr.add(voiceNames[i]);
    settings->setProperty("voiceNames", namesArr);

    auto* root = new DynamicObject();
    root->setProperty("circles", arr);
    root->setProperty("settings", var(settings));
    settings->setProperty("selectedVideoIndex", videoListManager.selectedIndex());
    if (lastVideoFile.existsAsFile())
        root->setProperty("videoPath", lastVideoFile.getFullPathName());

#if JUCE_STANDALONE_APPLICATION
    if (auto* holder = StandalonePluginHolder::getInstance())
        if (auto xml = holder->deviceManager.createStateXml())
            root->setProperty("audioDeviceState", xml->toString());
#endif

    file.getParentDirectory().createDirectory();
    file.replaceWithText(JSON::toString(var(root)));
}

void MidiVisuEditor::readPositionsFromFile(const File& file) {
    if (!file.existsAsFile()) return;

    const auto json = JSON::parse(file.loadFileAsString());
    auto* root = json.getDynamicObject();
    if (root == nullptr) return;

    auto* arr = root->getProperty("circles").getArray();
    if (arr != nullptr)
        for (int j = 0; j < jmin(7, arr->size()); ++j)
            if (auto* obj = arr->getReference(j).getDynamicObject()) {
                circlePos[j].x = static_cast<float>((double)obj->getProperty("x"));
                circlePos[j].y = static_cast<float>((double)obj->getProperty("y"));
            }

    if (auto* s = root->getProperty("settings").getDynamicObject()) {
        if (s->hasProperty("ballSizeMin") && s->hasProperty("ballSizeMax"))
            ballSizeSlider.setMinAndMaxValues((double)s->getProperty("ballSizeMin"),
                                              (double)s->getProperty("ballSizeMax"),
                                              dontSendNotification);
        if (s->hasProperty("videoEnabled"))
            videoToggle.setToggleState((bool)s->getProperty("videoEnabled"),
                                       dontSendNotification);
        if (s->hasProperty("blurEnabled"))
            blurToggle.setToggleState((bool)s->getProperty("blurEnabled"),
                                      dontSendNotification);
        if (s->hasProperty("blurRadius"))
            blurSlider.setValue((double)s->getProperty("blurRadius"),
                                dontSendNotification);
        if (s->hasProperty("videoZoom"))
            videoZoomSlider.setValue((double)s->getProperty("videoZoom"),
                                     dontSendNotification);
        if (s->hasProperty("videoOpacity"))
            videoOpacitySlider.setValue((double)s->getProperty("videoOpacity"),
                                        dontSendNotification);
        if (s->hasProperty("floatEnabled"))
            floatToggle.setToggleState((bool)s->getProperty("floatEnabled"),
                                       dontSendNotification);
        if (s->hasProperty("collisionEnabled"))
            collisionToggle.setToggleState((bool)s->getProperty("collisionEnabled"),
                                           dontSendNotification);
        if (s->hasProperty("floatIntensity"))
            floatIntensitySlider.setValue((double)s->getProperty("floatIntensity"),
                                          dontSendNotification);
        if (s->hasProperty("floatSpeed"))
            floatSpeedSlider.setValue((double)s->getProperty("floatSpeed"),
                                      dontSendNotification);
        if (s->hasProperty("clockDiv"))
            clockDivisionBox.setSelectedId((int)s->getProperty("clockDiv"),
                                           dontSendNotification);
        if (s->hasProperty("clockKick"))
            clockKickToggle.setToggleState((bool)s->getProperty("clockKick"),
                                           dontSendNotification);
        if (s->hasProperty("clockKickIntensity"))
            clockKickIntensitySlider.setValue(
                (double)s->getProperty("clockKickIntensity"),
                dontSendNotification);
        if (s->hasProperty("logMidiNotes"))
            logMidiNotesToggle.setToggleState((bool)s->getProperty("logMidiNotes"),
                                              dontSendNotification);
        if (s->hasProperty("logMidiClock"))
            logMidiClockToggle.setToggleState((bool)s->getProperty("logMidiClock"),
                                              dontSendNotification);
        if (s->hasProperty("loopStart"))
            pendingLoopStart_ = (double)s->getProperty("loopStart");
        if (s->hasProperty("loopEnd"))
            pendingLoopEnd_ = (double)s->getProperty("loopEnd");
        if (s->hasProperty("loopEnabled")) {
            const bool on = (bool)s->getProperty("loopEnabled");
            seekBar.setLoopEnabled(on);
            loopButton.setColour(TextButton::buttonColourId,
                                 Colour(on ? StyleTokens::kButtonBgOn
                                           : StyleTokens::kButtonBg));
        }
        if (s->hasProperty("optionsPanelOpen"))
            optionsPanelOpen = (bool)s->getProperty("optionsPanelOpen");
        if (s->hasProperty("logPanelOpen"))
            logPanelOpen = (bool)s->getProperty("logPanelOpen");

        if (auto* fArr = s->getProperty("foldStates").getArray())
            for (int i = 0;
                 i < jmin(static_cast<int>(OptionsPanelLayout::SectionCount),
                          fArr->size());
                 ++i)
                optionsLayout.setFolded(
                    static_cast<OptionsPanelLayout::Section>(i),
                    (bool)fArr->getReference(i));

        if (auto* chArr = s->getProperty("voiceChannels").getArray())
            for (int i = 0; i < jmin(7, chArr->size()); ++i) {
                const int ch = (int)chArr->getReference(i);
                voiceChannelBox[i].setSelectedId(ch, dontSendNotification);
                if (i < 4) audioProcessor.voiceManager.setDrumChannel(i, ch);
                else audioProcessor.voiceManager.setMelodicChannel(i - 4, ch);
            }

        if (auto* nmArr = s->getProperty("voiceNames").getArray())
            for (int i = 0; i < jmin(7, nmArr->size()); ++i) {
                voiceNames[i] = nmArr->getReference(i).toString();
                voiceNameLabel[i].setText(voiceNames[i], dontSendNotification);
            }
    }

    // Restore selected video — prefer index-based lookup, fall back to videoPath.
    bool videoRestored = false;
    if (auto* s = root->getProperty("settings").getDynamicObject()) {
        if (s->hasProperty("selectedVideoIndex")) {
            const int idx = (int)s->getProperty("selectedVideoIndex");
            if (idx >= 0 && idx < videoListManager.fileCount()) {
                videoListManager.setSelectedIndex(idx);
                const File f = assetsVideoDir.getChildFile(
                    String(videoListManager.filename(idx)));
                if (f.existsAsFile()) {
                    lastVideoFile = f;
                    MessageManager::callAsync([this, path = f.getFullPathName()]() {
                        if (seekBar.isLoopEnabled())
                            videoBackground.setLoopPoints(seekBar.getLoopStart(),
                                                          seekBar.getLoopEnd());
                        videoBackground.loadFile(path.toRawUTF8());
                        videoListManager.setPlayState(
                            VideoListManager::PlayState::Playing);
                        videoPlayPauseButton.setButtonText(CharPointer_UTF8("\xe2\x8f\xb8"));
                    });
                    videoRestored = true;
                }
            }
        }
    }

    if (!videoRestored) {
        const String videoPath = root->getProperty("videoPath").toString();
        if (videoPath.isNotEmpty()) {
            const File vf(videoPath);
            if (vf.existsAsFile()) {
                lastVideoFile = vf;
                MessageManager::callAsync([this, path = vf.getFullPathName()]() {
                    if (seekBar.isLoopEnabled())
                        videoBackground.setLoopPoints(seekBar.getLoopStart(),
                                                      seekBar.getLoopEnd());
                    videoBackground.loadFile(path.toRawUTF8());
                    videoListManager.setPlayState(VideoListManager::PlayState::Playing);
                    videoPlayPauseButton.setButtonText(CharPointer_UTF8("\xe2\x8f\xb8"));
                });
            }
        }
    }
}

void MidiVisuEditor::savePositions() {
    const File startDir = lastPositionsFile.existsAsFile()
                              ? lastPositionsFile.getParentDirectory()
                              : getAutoSaveFile().getParentDirectory();
    fileChooser = std::make_unique<
        FileChooser>("Save Settings to...", startDir, "*.json");
    fileChooser->launchAsync(FileBrowserComponent::saveMode
                             | FileBrowserComponent::canSelectFiles
                             | FileBrowserComponent::warnAboutOverwriting,
                             [this](const FileChooser& fc) {
                                 const File f = fc.getResult();
                                 if (f != File{}) {
                                     writePositionsToFile(f);
                                     lastPositionsFile = f;
                                 }
                             });
}

void MidiVisuEditor::loadPositions() {
    const File startDir = lastPositionsFile.existsAsFile()
                              ? lastPositionsFile.getParentDirectory()
                              : getAutoSaveFile().getParentDirectory();
    fileChooser = std::make_unique<FileChooser>("Load Settings", startDir, "*.json");
    fileChooser->launchAsync(FileBrowserComponent::openMode
                             | FileBrowserComponent::canSelectFiles,
                             [this](const FileChooser& fc) {
                                 const File f = fc.getResult();
                                 if (f != File{}) {
                                     readPositionsFromFile(f);
                                     lastPositionsFile = f;
                                     repaint();
                                 }
                             });
}


//==============================================================================
void MidiVisuEditor::timerCallback() {
    const float minR = static_cast<float>(ballSizeSlider.getMinValue());
    const float maxR = static_cast<float>(ballSizeSlider.getMaxValue());

    // --- Drum voice animation (per-voice): instant snap up, smooth decay ---
    for (int v = 0; v < 4; ++v) {
        const int count = audioProcessor.midiManager.drumVoiceHitCount[v].load(
            std::memory_order_relaxed);
        if (count != lastDrumHitCount[v]) {
            lastDrumHitCount[v] = count;
            drumSmoothedRadius[v] = maxR; // instant peak
        }
        drumSmoothedRadius[v] += (minR - drumSmoothedRadius[v]) * drumSmoothing;
    }
    {
        const int count = audioProcessor.midiManager.ch10RawHitCount.load(
            std::memory_order_relaxed);
        const int newHits = count - lastCh10RawHitCount;
        if (newHits > 0) {
            lastCh10RawHitCount = count;
            const int note = audioProcessor.midiManager.ch10RawHitNote.load(
                std::memory_order_relaxed);
            int matchedVoice = -1;
            for (int v = 0; v < 4; ++v)
                if (note == VoiceManager::kDrumNotes[v]) {
                    matchedVoice = v;
                    break;
                }
            const String label = (matchedVoice >= 0)
                                     ? voiceNames[matchedVoice]
                                     : ("ch10  " + noteName(note));
            const Colour color = (matchedVoice >= 0)
                                     ? drumColours[matchedVoice]
                                     : Colours::limegreen;
            if (logMidiNotesToggle.getToggleState())
                for (int h = 0; h < newHits; ++h)
                    appendLog(label, color);
        }
    }

    // --- Melodic channels animation + log ---
    for (int i = 1; i < 4; ++i) {
        const int note = audioProcessor.midiManager.channelHighestNote[i].load();
        const float target = (note >= 0)
                                 ? (minR + (note / 127.0f) * (maxR - minR))
                                 : minR;
        smoothedRadius[i] += (target - smoothedRadius[i]) * smoothing;

        const int count = audioProcessor.midiManager.channelNoteOnCount[i].load(
            std::memory_order_relaxed);
        const int newHits = count - lastChannelNoteOnCount[i];
        if (newHits > 0) {
            lastChannelNoteOnCount[i] = count;
            const int n = audioProcessor.midiManager.channelNoteOnNote[i].load(
                std::memory_order_relaxed);
            const int ch = audioProcessor.voiceManager.getMelodicChannel(i - 1);
            const String label = ("ch" + String(ch) + "   " + noteName(n));
            if (logMidiNotesToggle.getToggleState())
                for (int h = 0; h < newHits; ++h)
                    appendLog(label, kChColours[i]);
        }
    }

    // MIDI clock kick — independent of float toggle.
    static Random rng;
    const bool floatActive = floatToggle.getToggleState();
    const bool kickActive = clockKickToggle.getToggleState();

    const int clockNow = audioProcessor.midiManager.midiClockPulse.load(
        std::memory_order_relaxed);
    const int div = clockDivisionBox.getSelectedId();
    if (kickActive && div > 0 && clockNow / div > lastClockPulse / div) {
        const float kickStr = 8.0f * static_cast<float>(clockKickIntensitySlider.
            getValue());
        for (int i = 0; i < 7; ++i) {
            const float angle = rng.nextFloat() * MathConstants<float>::twoPi;
            floatVel[i].x += std::cos(angle) * kickStr;
            floatVel[i].y += std::sin(angle) * kickStr;
        }
        if (logMidiClockToggle.getToggleState())
            appendLog("CLOCK", styleManager.logClock());
    }
    lastClockPulse = clockNow;

    const bool collisionActive = collisionToggle.getToggleState();

    if (floatActive || kickActive || collisionActive) {
        const float intensity = floatActive
                                    ? static_cast<float>(floatIntensitySlider.getValue())
                                    : 0.0f;
        const float kForce = 5.4f * intensity;
        const float kDamping = 0.99f - static_cast<float>(floatSpeedSlider.getValue()) *
            0.16f;

        for (int i = 0; i < 7; ++i) {
            if (intensity > 0.0f) {
                floatVel[i].x += (rng.nextFloat() * 2.0f - 1.0f) * kForce;
                floatVel[i].y += (rng.nextFloat() * 2.0f - 1.0f) * kForce;
            }
            floatVel[i] *= kDamping;
            floatOffset[i] += floatVel[i];
        }

        // Elastic collisions — radii match current visual size.
        if (collisionActive) {
            float cr[7];
            for (int v = 0; v < 4; ++v) cr[v] = jmax(minR, drumSmoothedRadius[v]);
            for (int i = 1; i < 4; ++i) cr[3 + i] = jmax(minR, smoothedRadius[i]);

            const float W = static_cast<float>(getWidth());
            const float H = static_cast<float>(getHeight());

            // Ball-wall
            for (int i = 0; i < 7; ++i) {
                const float cx = circlePos[i].x + floatOffset[i].x;
                const float cy = circlePos[i].y + floatOffset[i].y;
                if (cx - cr[i] < 0.f) {
                    floatOffset[i].x += cr[i] - cx;
                    floatVel[i].x = std::abs(floatVel[i].x);
                }
                else if (cx + cr[i] > W) {
                    floatOffset[i].x -= cx + cr[i] - W;
                    floatVel[i].x = -std::abs(floatVel[i].x);
                }
                if (cy - cr[i] < 0.f) {
                    floatOffset[i].y += cr[i] - cy;
                    floatVel[i].y = std::abs(floatVel[i].y);
                }
                else if (cy + cr[i] > H) {
                    floatOffset[i].y -= cy + cr[i] - H;
                    floatVel[i].y = -std::abs(floatVel[i].y);
                }
            }

            // Ball-ball (general elastic, prepared for unequal masses)
            for (int i = 0; i < 7; ++i)
                for (int j = i + 1; j < 7; ++j) {
                    const float dx = (circlePos[j].x + floatOffset[j].x)
                        - (circlePos[i].x + floatOffset[i].x);
                    const float dy = (circlePos[j].y + floatOffset[j].y)
                        - (circlePos[i].y + floatOffset[i].y);
                    const float distSq = dx * dx + dy * dy;
                    const float minDist = cr[i] + cr[j];
                    if (distSq >= minDist * minDist || distSq < 0.0001f) continue;

                    const float dist = std::sqrt(distSq);
                    const float nx = dx / dist, ny = dy / dist;

                    const float dvn = (floatVel[i].x - floatVel[j].x) * nx
                        + (floatVel[i].y - floatVel[j].y) * ny;
                    if (dvn > 0.f) {
                        const float mi = ballMass[i], mj = ballMass[j];
                        const float imp = 2.f * dvn / (mi + mj);
                        floatVel[i].x -= imp * mj * nx;
                        floatVel[i].y -= imp * mj * ny;
                        floatVel[j].x += imp * mi * nx;
                        floatVel[j].y += imp * mi * ny;
                    }

                    const float half = (minDist - dist) * 0.5f;
                    floatOffset[i].x -= nx * half;
                    floatOffset[i].y -= ny * half;
                    floatOffset[j].x += nx * half;
                    floatOffset[j].y += ny * half;
                }
        }
    }
    else {
        for (int i = 0; i < 7; ++i)
            floatVel[i] = floatOffset[i] = {};
    }

    // Sync seekbar playhead with video current time (skip while user is dragging)
    if (videoToggle.getToggleState() && !seekBar.isDragging()) {
        const double dur = videoBackground.duration();
        if (dur > 0.0) {
            seekBar.setMaxValue(dur);
            if (pendingLoopStart_ >= 0.0) {
                seekBar.setLoopStart(pendingLoopStart_);
                seekBar.setLoopEnd(pendingLoopEnd_);
                if (seekBar.isLoopEnabled()) {
                    videoBackground.setLoopPoints(pendingLoopStart_,
                                                  pendingLoopEnd_);
                    videoBackground.seek(pendingLoopStart_);
                }
                pendingLoopStart_ = -1.0;
                pendingLoopEnd_ = -1.0;
            }
            seekBar.setPlayhead(videoBackground.currentTime());
        }
    }

    repaint();
}

//==============================================================================
void MidiVisuEditor::resized() {
    // ── Log panel widgets ────────────────────────────────────────────────────
    if (logPanelOpen) {
        const int lpad = 10;
        logMidiNotesToggle.setBounds(lpad, 54, logPanelWidth - lpad * 2, 24);
        logMidiNotesToggle.setVisible(true);
        logMidiClockToggle.setBounds(lpad, 80, logPanelWidth - lpad * 2, 24);
        logMidiClockToggle.setVisible(true);
        clearLogButton.setBounds(lpad, 112, logPanelWidth - lpad * 2, 22);
        clearLogButton.setVisible(true);
    } else {
        logMidiNotesToggle.setVisible(false);
        logMidiClockToggle.setVisible(false);
        clearLogButton.setVisible(false);
    }

    // ── Options panel widgets ────────────────────────────────────────────────
    if (!optionsPanelOpen) {
        for (auto& box : voiceChannelBox) box.setVisible(false);
        for (auto& lbl : voiceNameLabel) lbl.setVisible(false);
        midiSettingsButton.setVisible(false);
        seekBar.setVisible(false);
        videoPlayPauseButton.setVisible(false);
        videoStopButton.setVisible(false);
        loopButton.setVisible(false);
        videoToggle.setVisible(false);
        blurToggle.setVisible(false);
        blurSlider.setVisible(false);
        videoZoomSlider.setVisible(false);
        videoOpacitySlider.setVisible(false);
        ballSizeSlider.setVisible(false);
        floatToggle.setVisible(false);
        collisionToggle.setVisible(false);
        clockKickToggle.setVisible(false);
        clockDivisionBox.setVisible(false);
        clockKickIntensitySlider.setVisible(false);
        floatIntensitySlider.setVisible(false);
        floatSpeedSlider.setVisible(false);
        saveDefaultButton.setVisible(false);
        savePositionsButton.setVisible(false);
        loadPositionsButton.setVisible(false);
        return;
    }

    optionsLayout.setViewportHeight(getHeight());
    const int scroll = optionsLayout.scrollOffset();
    const int px = getWidth() - logPanelWidth;
    const int pad = 10;
    const int panelW = logPanelWidth - pad * 2;
    const int h = getHeight();

    // Helper: position a component and set visible only if within viewport
    auto place = [&](Component& c, int x, int y, int w, int ch) {
        c.setBounds(x, y, w, ch);
        c.setVisible(y + ch > 0 && y < h);
    };

    // Fixed header area (scrolls with content)
    place(midiSettingsButton, px + pad,
          OptionsPanelLayout::kMidiSettingsButtonY - scroll, panelW, 26);

    // ── MIDI ROUTING ─────────────────────────────────────────────────────────
    if (!optionsLayout.isFolded(OptionsPanelLayout::MidiRouting)) {
        const int firstY = optionsLayout.midiRoutingFirstRowY() - scroll;
        const int boxX = px + 110;
        const int boxW = logPanelWidth - 110 - pad;
        const int rowH = 26;
        for (int i = 0; i < 7; ++i) {
            place(voiceChannelBox[i], boxX, firstY + i * rowH + 2, boxW, 22);
            place(voiceNameLabel[i], px + pad + 14, firstY + i * rowH + 2, 82, 20);
        }
    } else {
        for (int i = 0; i < 7; ++i) {
            voiceChannelBox[i].setVisible(false);
            voiceNameLabel[i].setVisible(false);
        }
    }

    // ── VIDEO ────────────────────────────────────────────────────────────────
    if (!optionsLayout.isFolded(OptionsPanelLayout::Video)) {
        const int seekY = optionsLayout.videoSeekBarY() - scroll;
        const int btnH = SeekBar::kTrackAreaHeight;
        const int btnGap = 2;
        const int seekGap = 4;
        const int btnsW = btnH * 3 + btnGap * 2 + seekGap;
        place(videoPlayPauseButton, px + pad, seekY, btnH, btnH);
        place(videoStopButton, px + pad + btnH + btnGap, seekY, btnH, btnH);
        place(loopButton, px + pad + (btnH + btnGap) * 2, seekY, btnH, btnH);
        place(seekBar, px + pad + btnsW, seekY, panelW - btnsW, SeekBar::kBarHeight);

        const int ctrlY = optionsLayout.videoCtrlY() - scroll;
        place(videoToggle, px + pad, ctrlY, panelW, StyleTokens::kRowHeight);
        place(blurToggle, px + pad, ctrlY + 26, panelW, StyleTokens::kRowHeight);
        place(blurSlider, px + pad, ctrlY + 70, panelW, StyleTokens::kSliderHeight);
        place(videoZoomSlider, px + pad, ctrlY + 124, panelW,
              StyleTokens::kSliderHeight);
        place(videoOpacitySlider, px + pad, ctrlY + 178, panelW,
              StyleTokens::kSliderHeight);
    } else {
        seekBar.setVisible(false);
        videoPlayPauseButton.setVisible(false);
        videoStopButton.setVisible(false);
        loopButton.setVisible(false);
        videoToggle.setVisible(false);
        blurToggle.setVisible(false);
        blurSlider.setVisible(false);
        videoZoomSlider.setVisible(false);
        videoOpacitySlider.setVisible(false);
    }

    // ── CIRCLES ──────────────────────────────────────────────────────────────
    if (!optionsLayout.isFolded(OptionsPanelLayout::Circles)) {
        place(ballSizeSlider, px + pad,
              optionsLayout.circlesSliderY() - scroll, panelW,
              StyleTokens::kSliderHeight);
    } else {
        ballSizeSlider.setVisible(false);
    }

    // ── ANIMATION ────────────────────────────────────────────────────────────
    if (!optionsLayout.isFolded(OptionsPanelLayout::Animation)) {
        place(floatToggle, px + pad,
              optionsLayout.animFloatToggleY() - scroll, panelW,
              StyleTokens::kRowHeight);
        place(collisionToggle, px + pad,
              optionsLayout.animCollisionToggleY() - scroll, panelW,
              StyleTokens::kRowHeight);
        place(clockKickToggle, px + pad,
              optionsLayout.animClockToggleY() - scroll, panelW,
              StyleTokens::kRowHeight);
        place(clockDivisionBox, px + pad,
              optionsLayout.animClockDivY() - scroll, panelW, 24);
        place(clockKickIntensitySlider, px + pad,
              optionsLayout.animClockKickSliderY() - scroll, panelW,
              StyleTokens::kSliderHeight);
        place(floatIntensitySlider, px + pad,
              optionsLayout.animFloatIntensitySliderY() - scroll, panelW,
              StyleTokens::kSliderHeight);
        place(floatSpeedSlider, px + pad,
              optionsLayout.animFloatSpeedSliderY() - scroll, panelW,
              StyleTokens::kSliderHeight);
    } else {
        floatToggle.setVisible(false);
        collisionToggle.setVisible(false);
        clockKickToggle.setVisible(false);
        clockDivisionBox.setVisible(false);
        clockKickIntensitySlider.setVisible(false);
        floatIntensitySlider.setVisible(false);
        floatSpeedSlider.setVisible(false);
    }

    // ── BUTTONS (always visible) ─────────────────────────────────────────────
    const int btnsY = optionsLayout.buttonsY() - scroll;
    const int btnW = (logPanelWidth - pad * 3) / 2;
    place(saveDefaultButton, px + pad, btnsY, panelW, StyleTokens::kSliderHeight);
    place(savePositionsButton, px + pad, btnsY + 34, btnW, StyleTokens::kSliderHeight);
    place(loadPositionsButton, px + pad * 2 + btnW, btnsY + 34, btnW,
          StyleTokens::kSliderHeight);
}

//==============================================================================
void MidiVisuEditor::seekBarLoopChanged(SeekBar* bar) {
    if (bar->isLoopEnabled())
        videoBackground.setLoopPoints(bar->getLoopStart(), bar->getLoopEnd());
    else
        videoBackground.setLoopPoints(0.0, videoBackground.duration());
}

void MidiVisuEditor::seekBarPlayheadDragged(SeekBar* bar) {
    videoBackground.seek(bar->getPlayhead());
}

//==============================================================================
void MidiVisuEditor::showJuceAudioSettings() {
#if JUCE_STANDALONE_APPLICATION
    if (auto* holder = StandalonePluginHolder::getInstance()) {
        auto* selector = new AudioDeviceSelectorComponent(
            holder->deviceManager,
            0, 0, 0, 0,
            true, false, false, false);
        selector->setSize(500, 400);

        DialogWindow::LaunchOptions opts;
        opts.content.setOwned(selector);
        opts.dialogTitle = "Audio / MIDI Settings";
        opts.dialogBackgroundColour = styleManager.dialogBackground();
        opts.escapeKeyTriggersCloseButton = true;
        opts.useNativeTitleBar = true;
        opts.resizable = true;
        opts.launchAsync();
    }
#endif
}