/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#if JUCE_STANDALONE_APPLICATION
 #include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif

static const Colour channelColours[4] =
{
    Colours::limegreen, // track 0 (ch 10) - not used directly, drums use drumColours
    Colours::orange,    // track 1 (ch 2)
    Colours::yellow,    // track 2 (ch 3)
    Colour(0xffb03060), // track 3 (ch 4) - reddish-purple
};

// Ch10 drum voices: progressively darker lime green. Voice 0 = kick (lightest).
static const Colour drumColours[4] =
{
    Colour(0xff32cd32), // Voice 1 Kick  - limegreen
    Colour(0xff27a427), // Voice 2 Clap  - ~80%
    Colour(0xff1d7b1d), // Voice 3 Snare - ~60%
    Colour(0xff125212), // Voice 4 HH    - ~40%
};

//==============================================================================
static constexpr int kDefaultDrumCh[4]    = { 10, 10, 10, 10 };
static constexpr int kDefaultMelodicCh[3] = { 2, 3, 4 };

static const char* kVoiceNames[7]    = { "KICK", "NOISE", "SNARE", "HH", "K-2", "EREBUS",
    "HYDRA" };
static const Colour kVoiceColours[7] = {
    Colour(0xff32cd32), Colour(0xff27a427), Colour(0xff1d7b1d), Colour(0xff125212),
    Colours::orange, Colours::yellow, Colour(0xffb03060)
};

MidiVisuEditor::MidiVisuEditor(MidivisuAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setWantsKeyboardFocus(true);

    // Voice channel ComboBoxes (hidden until options panel opens).
    for (int i = 0; i < 7; ++i)
    {
        for (int ch = 1; ch <= 16; ++ch)
            voiceChannelBox[i].addItem(String(ch), ch);

        const int defaultCh = (i < 4) ? kDefaultDrumCh[i] : kDefaultMelodicCh[i - 4];
        voiceChannelBox[i].setSelectedId(defaultCh, dontSendNotification);

        voiceChannelBox[i].onChange = [this, i]
        {
            const int ch = voiceChannelBox[i].getSelectedId();
            if (i < 4)
                audioProcessor.drumVoiceMidiChannel[i].store(ch, std::memory_order_relaxed);
            else
                audioProcessor.melodicMidiChannel[i - 4].store(ch, std::memory_order_relaxed);
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

    saveDefaultButton.onClick   = [this] { writePositionsToFile(getAutoSaveFile()); };
    savePositionsButton.onClick = [this] { savePositions(); };
    loadPositionsButton.onClick = [this] { loadPositions(); };
    loadVideoButton.onClick     = [this] { chooseVideoFile(); };
    midiSettingsButton.onClick  = [this] { showJuceAudioSettings(); };
    addChildComponent(saveDefaultButton);
    addChildComponent(savePositionsButton);
    addChildComponent(loadPositionsButton);
    addChildComponent(loadVideoButton);
    addChildComponent(midiSettingsButton);

    setSize(1920, 1080);
    initDefaultPositions();
    readPositionsFromFile(getAutoSaveFile());
    startTimerHz(60);
}

MidiVisuEditor::~MidiVisuEditor()
{
    stopTimer();
}

//==============================================================================
static const Colour kChColours[4] = {
    Colours::limegreen,   // ch10 (fallback for unmatched notes)
    Colours::orange,      // ch2
    Colours::yellow,      // ch3
    Colour(0xffb03060),   // ch4
};

static String noteName(int n)
{
    return MidiMessage::getMidiNoteName(n, true, true, 4);
}

static void pushLog(StringArray& lines, Array<Colour>& colors, const String& text, Colour color)
{
    lines.insert (0, text);
    colors.insert(0, color);
}

// ── Circle position helpers ────────────────────────────────────────────────────
void MidiVisuEditor::initDefaultPositions()
{
    const float w     = (float)getWidth();
    const float h     = (float)getHeight();
    const float slotW = w / 4.0f;
    const float rowH  = h / 4.0f;

    for (int v = 0; v < 4; ++v)
        circlePos[v] = { slotW * 0.5f, rowH * (3 - v) + rowH * 0.5f };

    for (int i = 1; i <= 3; ++i)
        circlePos[3 + i] = { slotW * i + slotW * 0.5f, h * 0.5f };
}

File MidiVisuEditor::getAutoSaveFile() const
{
    return File::getSpecialLocation(File::userApplicationDataDirectory)
               .getChildFile("MidiVisu")
               .getChildFile("circle_positions.json");
}

void MidiVisuEditor::writePositionsToFile(const File& file)
{
    Array<var> arr;
    for (int i = 0; i < 7; ++i)
    {
        auto* entry = new DynamicObject();
        entry->setProperty("name", String(kVoiceNames[i]));
        entry->setProperty("x",   (double)circlePos[i].x);
        entry->setProperty("y",   (double)circlePos[i].y);
        arr.add(entry);
    }
    auto* settings = new DynamicObject();
    settings->setProperty("videoEnabled",  videoToggle.getToggleState());
    settings->setProperty("blurEnabled",   blurToggle.getToggleState());
    settings->setProperty("blurRadius",    blurSlider.getValue());
    settings->setProperty("videoZoom",     videoZoomSlider.getValue());
    settings->setProperty("videoOpacity",  videoOpacitySlider.getValue());

    auto* root = new DynamicObject();
    root->setProperty("circles",  arr);
    root->setProperty("settings", var(settings));
    if (lastVideoFile.existsAsFile())
        root->setProperty("videoPath", lastVideoFile.getFullPathName());
    file.getParentDirectory().createDirectory();
    file.replaceWithText(JSON::toString(var(root)));
}

void MidiVisuEditor::readPositionsFromFile(const File& file)
{
    if (! file.existsAsFile()) return;

    const auto json = JSON::parse(file.loadFileAsString());
    auto* root = json.getDynamicObject();
    if (root == nullptr) return;

    auto* arr = root->getProperty("circles").getArray();
    if (arr != nullptr)
    {
        for (const auto& entry : *arr)
        {
            auto* obj = entry.getDynamicObject();
            if (obj == nullptr) continue;

            const String name = obj->getProperty("name").toString();
            for (int j = 0; j < 7; ++j)
            {
                if (name == kVoiceNames[j])
                {
                    circlePos[j].x = (float)(double)obj->getProperty("x");
                    circlePos[j].y = (float)(double)obj->getProperty("y");
                    break;
                }
            }
        }
    }

    if (auto* s = root->getProperty("settings").getDynamicObject())
    {
        if (s->hasProperty("videoEnabled"))
            videoToggle.setToggleState((bool)s->getProperty("videoEnabled"), dontSendNotification);
        if (s->hasProperty("blurEnabled"))
            blurToggle.setToggleState((bool)s->getProperty("blurEnabled"), dontSendNotification);
        if (s->hasProperty("blurRadius"))
            blurSlider.setValue((double)s->getProperty("blurRadius"), dontSendNotification);
        if (s->hasProperty("videoZoom"))
            videoZoomSlider.setValue((double)s->getProperty("videoZoom"), dontSendNotification);
        if (s->hasProperty("videoOpacity"))
            videoOpacitySlider.setValue((double)s->getProperty("videoOpacity"), dontSendNotification);
    }

    const String videoPath = root->getProperty("videoPath").toString();
    if (videoPath.isNotEmpty())
    {
        const File vf(videoPath);
        if (vf.existsAsFile())
        {
            lastVideoFile = vf;
            // Defer the actual AVFoundation load until the message loop is running;
            // calling it directly from the constructor (before the run loop spins)
            // leaves AVFoundation's async seek with no run-loop to complete on.
            MessageManager::callAsync([this, path = vf.getFullPathName()]()
            {
                videoBackground.loadFile(path.toRawUTF8());
            });
        }
    }
}

void MidiVisuEditor::savePositions()
{
    const File startDir = lastPositionsFile.existsAsFile()
                              ? lastPositionsFile.getParentDirectory()
                              : getAutoSaveFile().getParentDirectory();
    fileChooser = std::make_unique<FileChooser>("Save Circle Positions", startDir, "*.json");
    fileChooser->launchAsync(FileBrowserComponent::saveMode
                                 | FileBrowserComponent::canSelectFiles
                                 | FileBrowserComponent::warnAboutOverwriting,
        [this](const FileChooser& fc)
        {
            const File f = fc.getResult();
            if (f != File{})
            {
                writePositionsToFile(f);
                lastPositionsFile = f;
            }
        });
}

void MidiVisuEditor::loadPositions()
{
    const File startDir = lastPositionsFile.existsAsFile()
                              ? lastPositionsFile.getParentDirectory()
                              : getAutoSaveFile().getParentDirectory();
    fileChooser = std::make_unique<FileChooser>("Load Circle Positions", startDir, "*.json");
    fileChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
        [this](const FileChooser& fc)
        {
            const File f = fc.getResult();
            if (f != File{})
            {
                readPositionsFromFile(f);
                lastPositionsFile = f;
                repaint();
            }
        });
}

void MidiVisuEditor::chooseVideoFile()
{
    const File startDir = lastVideoFile.existsAsFile()
                              ? lastVideoFile.getParentDirectory()
                              : File::getSpecialLocation(File::userHomeDirectory);
    videoChooser = std::make_unique<FileChooser>("Load Video", startDir, "*.mp4;*.mov;*.m4v");
    videoChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
        [this](const FileChooser& fc)
        {
            const File f = fc.getResult();
            if (f != File{})
            {
                videoBackground.loadFile(f.getFullPathName().toRawUTF8());
                lastVideoFile = f;
                writePositionsToFile(getAutoSaveFile());
            }
        });
}

void MidiVisuEditor::timerCallback()
{
    static const String voiceNames[4] = { "Kick", "Clap", "Snare", "HH" };
    static constexpr int kDrumNotes[4] = { 48, 50, 52, 54 }; // C3, D3, E3, F#3 (JUCE octave numbering)

    // --- ch10: animation (per-voice) — instant snap up, smooth decay ---
    for (int v = 0; v < 4; ++v)
    {
        const int count = audioProcessor.drumVoiceHitCount[v].load(std::memory_order_relaxed);
        if (count != lastDrumHitCount[v])
        {
            lastDrumHitCount[v]   = count;
            drumSmoothedRadius[v] = drumHitRadius; // instant peak
        }
        drumSmoothedRadius[v] += (drumRestRadius - drumSmoothedRadius[v]) * drumSmoothing;
    }
    {
        const int count   = audioProcessor.ch10RawHitCount.load(std::memory_order_relaxed);
        const int newHits = count - lastCh10RawHitCount;
        if (newHits > 0)
        {
            lastCh10RawHitCount = count;
            const int note      = audioProcessor.ch10RawHitNote.load(std::memory_order_relaxed);
            int matchedVoice = -1;
            for (int v = 0; v < 4; ++v)
                if (note == kDrumNotes[v]) { matchedVoice = v; break; }
            const String label = (matchedVoice >= 0)
                                     ? voiceNames[matchedVoice]
                                     : ("ch10  " + noteName(note));
            const Colour color = (matchedVoice >= 0)
                                     ? drumColours[matchedVoice]
                                     : Colours::limegreen;
            for (int h = 0; h < newHits; ++h)
                pushLog(logLines, logColors, label, color);
        }
    }

    // --- ch2/3/4: animation + log ---
    for (int i = 1; i < 4; ++i)
    {
        const int note = audioProcessor.channelHighestNote[i].load();
        const float target = (note >= 0)
                                 ? (minRadius + (note / 127.0f) * (maxRadius - minRadius))
                                 : minRadius;
        smoothedRadius[i] += (target - smoothedRadius[i]) * smoothing;

        const int count   = audioProcessor.channelNoteOnCount[i].load(std::memory_order_relaxed);
        const int newHits = count - lastChannelNoteOnCount[i];
        if (newHits > 0)
        {
            lastChannelNoteOnCount[i] = count;
            const int n  = audioProcessor.channelNoteOnNote[i].load(std::memory_order_relaxed);
            const int ch = audioProcessor.melodicMidiChannel[i - 1].load(std::memory_order_relaxed);
            const String label = ("ch" + String(ch) + "   " + noteName(n));
            for (int h = 0; h < newHits; ++h)
                pushLog(logLines, logColors, label, kChColours[i]);
        }
    }

    while (logLines.size() > maxLogLines)
    {
        logLines.remove (logLines.size()  - 1);
        logColors.remove(logColors.size() - 1);
    }

    repaint();
}

void MidiVisuEditor::paint(Graphics& g)
{
    g.fillAll(Colours::black);

    // ── Video ──────────────────────────────────────────────────────────────────
    int srcW = 0, srcH = 0, srcStride = 0;
    const uint8_t* srcPixels = nullptr;
    if (videoToggle.getToggleState()
            && videoBackground.getLatestFrame(srcW, srcH, srcStride, srcPixels))
    {
        if (! videoFrame.isValid() || videoFrame.getWidth() != srcW || videoFrame.getHeight() != srcH)
            videoFrame = Image(Image::ARGB, srcW, srcH, false);

        {
            Image::BitmapData bmp(videoFrame, Image::BitmapData::writeOnly);
            for (int row = 0; row < srcH; ++row)
            {
                const uint8_t* src = srcPixels + row * srcStride;
                uint8_t*       dst = bmp.getLinePointer(row);
                for (int col = 0; col < srcW; ++col)
                {
                    // Source is BGRA; JUCE Image::ARGB has the same byte layout on LE macOS.
                    const uint8_t pb = src[col * 4 + 0];
                    const uint8_t pg = src[col * 4 + 1];
                    const uint8_t pr = src[col * 4 + 2];
                    const uint8_t pa = src[col * 4 + 3];
                    // BT.601 luma (integer, no FP per pixel)
                    const uint8_t y = (uint8_t)((77 * pr + 150 * pg + 29 * pb) >> 8);
                    dst[col * 4 + 0] = y;
                    dst[col * 4 + 1] = y;
                    dst[col * 4 + 2] = y;
                    dst[col * 4 + 3] = pa;
                }
            }
        }

        if (blurToggle.getToggleState())
        {
            const float r = (float)blurSlider.getValue();
            if (r > 0.0f)
                videoFrame.getPixelData()->applyGaussianBlurEffect(r);
        }

        const float videoZoom = (float)videoZoomSlider.getValue();
        const int destW = getWidth(), destH = getHeight();
        const int cropW = roundToInt((float)srcW / videoZoom);
        const int cropH = roundToInt((float)srcW * destH / ((float)destW * videoZoom));
        const int cropX = (srcW - cropW) / 2, cropY = (srcH - cropH) / 2;

        g.setOpacity((float)videoOpacitySlider.getValue());
        g.drawImage(videoFrame, 0, 0, destW, destH, cropX, cropY, cropW, cropH);
    }

    // ── Circles (positions draggable) ──────────────────────────────────────────
    g.setOpacity(1.0f);

    for (int v = 0; v < 4; ++v)
    {
        const float r  = drumSmoothedRadius[v];
        const float cx = circlePos[v].x, cy = circlePos[v].y;
        g.setColour(drumColours[v]);
        g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
    }

    for (int i = 1; i < 4; ++i)
    {
        const float r  = smoothedRadius[i];
        const float cx = circlePos[3 + i].x, cy = circlePos[3 + i].y;
        g.setColour(channelColours[i]);
        g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
    }

    // ── Options panel background + labels (drawn above circles, below child components) ──
    if (optionsPanelOpen)
    {
        const int px   = getWidth() - logPanelWidth;
        const int pad  = 10;
        const int rowH = 26;
        const Font monoFont (FontOptions().withName(Font::getDefaultMonospacedFontName()).withHeight(12.0f));
        const Font labelFont (FontOptions().withHeight(13.0f));

        // Background + left border
        g.setColour(Colour(0xf0101010));
        g.fillRect(px, 0, logPanelWidth, getHeight());
        g.setColour(Colour(0x50ffffff));
        g.drawVerticalLine(px, 0.0f, (float)getHeight());

        // "OPTIONS" header
        g.setFont(labelFont);
        g.setColour(Colours::white.withAlpha(0.9f));
        g.drawText("OPTIONS", px + pad, 8, logPanelWidth - pad * 2, 18, Justification::left);
        g.setColour(Colour(0x30ffffff));
        g.drawHorizontalLine(30, (float)(px + pad), (float)(getWidth() - pad));

        // "MIDI ROUTING" section
        g.setFont(monoFont);
        g.setColour(Colour(0x80ffffff));
        g.drawText("MIDI ROUTING", px + pad, 66, logPanelWidth - pad * 2, 16, Justification::left);

        const int firstY = 88;
        for (int i = 0; i < 7; ++i)
        {
            const int rowY = firstY + i * rowH;
            // Coloured dot
            g.setColour(kVoiceColours[i]);
            g.fillEllipse((float)(px + pad), (float)(rowY + 6), 10.0f, 10.0f);
            // Voice name
            g.setColour(Colours::white.withAlpha(0.85f));
            g.setFont(labelFont);
            g.drawText(kVoiceNames[i], px + pad + 14, rowY + 2, 82, 20, Justification::left);
            // "ch" prefix before ComboBox
            g.setColour(Colour(0x70ffffff));
            g.setFont(monoFont);
            g.drawText("ch", px + 92, rowY + 5, 18, 16, Justification::left);
        }

        // Divider + VIDEO section header
        const int videoY = firstY + 7 * rowH + 20;
        g.setColour(Colour(0x30ffffff));
        g.drawHorizontalLine(videoY - 10, (float)(px + pad), (float)(getWidth() - pad));
        g.setFont(monoFont);
        g.setColour(Colour(0x80ffffff));
        g.drawText("VIDEO", px + pad, videoY, logPanelWidth - pad * 2, 16, Justification::left);

        // Sub-labels for sliders
        g.setFont(monoFont);
        g.setColour(Colour(0x60ffffff));
        g.drawText("Blur radius", px + pad, videoY + 72, logPanelWidth - pad * 2, 14, Justification::left);
        g.drawText("Zoom",        px + pad, videoY + 122, logPanelWidth - pad * 2, 14, Justification::left);
        g.drawText("Opacity",     px + pad, videoY + 172, logPanelWidth - pad * 2, 14, Justification::left);

        // Divider + CIRCLE POSITIONS section header
        const int posY = videoY + 244;
        g.setColour(Colour(0x30ffffff));
        g.drawHorizontalLine(posY - 10, (float)(px + pad), (float)(getWidth() - pad));
        g.setFont(monoFont);
        g.setColour(Colour(0x80ffffff));
        g.drawText("CIRCLE POSITIONS", px + pad, posY, logPanelWidth - pad * 2, 16, Justification::left);
    }
}

void MidiVisuEditor::paintOverChildren(Graphics& g)
{
    // Log panel overlay (toggled with L)
    if (logPanelOpen)
    {
        const int panelX = getWidth() - logPanelWidth;
        const int panelH = getHeight();
        const int sbW    = 6;
        const int pad    = 8;
        const int lineH  = 18;
        const int visibleLines = jmax(1, (panelH - pad) / lineH);
        const int maxOff       = jmax(0, logLines.size() - visibleLines);
        const int offset       = jlimit(0, maxOff, logScrollOffset);

        // Background
        g.setColour(Colour(0xe8101010));
        g.fillRect(panelX, 0, logPanelWidth, panelH);

        // Left border
        g.setColour(Colour(0x50ffffff));
        g.drawVerticalLine(panelX, 0.0f, (float)panelH);

        // Log lines (latest first, coloured by instrument)
        g.setFont(Font(FontOptions().withName(Font::getDefaultMonospacedFontName()).withHeight(13.0f)));
        for (int i = 0; i < visibleLines; ++i)
        {
            const int idx = offset + i;
            if (idx >= logLines.size()) break;
            g.setColour(idx < logColors.size() ? logColors[idx] : Colour(0xffcccccc));
            g.drawText(logLines[idx],
                       panelX + pad, pad + i * lineH,
                       logPanelWidth - sbW - pad * 2, lineH,
                       Justification::left, false);
        }

        // Scrollbar
        if (logLines.size() > visibleLines)
        {
            const float totalH    = (float)panelH;
            const float thumbFrac = (float)visibleLines / (float)logLines.size();
            const float thumbH    = jmax(20.0f, thumbFrac * totalH);
            const float thumbY    = (maxOff > 0)
                                        ? ((float)offset / maxOff) * (totalH - thumbH)
                                        : 0.0f;

            g.setColour(Colour(0x20ffffff));
            g.fillRect(panelX + logPanelWidth - sbW, 0, sbW, panelH);

            g.setColour(Colour(0x70ffffff));
            g.fillRoundedRectangle((float)(panelX + logPanelWidth - sbW),
                                   thumbY, (float)sbW, thumbH, 3.0f);
        }
    }
}

void MidiVisuEditor::resized()
{
    if (! optionsPanelOpen) return;

    const int px     = getWidth() - logPanelWidth;
    const int pad    = 10;

    midiSettingsButton.setBounds(px + pad, 34, logPanelWidth - pad * 2, 26);

    const int boxX   = px + 110;
    const int boxW   = logPanelWidth - 110 - pad;
    const int rowH   = 26;
    const int firstY = 88;

    for (int i = 0; i < 7; ++i)
        voiceChannelBox[i].setBounds(boxX, firstY + i * rowH + 2, boxW, 22);

    const int videoY = firstY + 7 * rowH + 20;
    videoToggle.setBounds      (px + pad, videoY + 20,  logPanelWidth - pad * 2, 26);
    blurToggle.setBounds       (px + pad, videoY + 46,  logPanelWidth - pad * 2, 26);
    blurSlider.setBounds       (px + pad, videoY + 86,  logPanelWidth - pad * 2, 28);
    videoZoomSlider.setBounds  (px + pad, videoY + 136, logPanelWidth - pad * 2, 28);
    videoOpacitySlider.setBounds(px + pad, videoY + 186, logPanelWidth - pad * 2, 28);
    loadVideoButton.setBounds  (px + pad, videoY + 214, logPanelWidth - pad * 2, 28);

    const int posY = videoY + 244;
    const int btnW = (logPanelWidth - pad * 3) / 2;
    saveDefaultButton.setBounds  (px + pad,            posY + 22, logPanelWidth - pad * 2, 28);
    savePositionsButton.setBounds(px + pad,            posY + 54, btnW, 28);
    loadPositionsButton.setBounds(px + pad * 2 + btnW, posY + 54, btnW, 28);
}

bool MidiVisuEditor::keyPressed(const KeyPress& key)
{
    if (key == KeyPress::escapeKey)
    {
        if (auto* peer = getPeer())
        {
            peer->setFullScreen(false);
            setSize(1920, 1080);
        }
        return true;
    }

    if (key == KeyPress('f', ModifierKeys::noModifiers, 0))
    {
        if (auto* peer = getPeer())
            peer->setFullScreen(!peer->isFullScreen());
        return true;
    }

    if (key == KeyPress('b', ModifierKeys::noModifiers, 0))
    {
        blurToggle.setToggleState(!blurToggle.getToggleState(), dontSendNotification);
        repaint();
        return true;
    }

    if (key == KeyPress('o', ModifierKeys::noModifiers, 0))
    {
        optionsPanelOpen = !optionsPanelOpen;
        for (auto& box : voiceChannelBox) box.setVisible(optionsPanelOpen);
        videoToggle.setVisible(optionsPanelOpen);
        blurToggle.setVisible(optionsPanelOpen);
        blurSlider.setVisible(optionsPanelOpen);
        videoZoomSlider.setVisible(optionsPanelOpen);
        videoOpacitySlider.setVisible(optionsPanelOpen);
        saveDefaultButton.setVisible(optionsPanelOpen);
        savePositionsButton.setVisible(optionsPanelOpen);
        loadPositionsButton.setVisible(optionsPanelOpen);
        loadVideoButton.setVisible(optionsPanelOpen);
        midiSettingsButton.setVisible(optionsPanelOpen);
        if (optionsPanelOpen)
            resized();
        repaint();
        return true;
    }

    if (key == KeyPress('l', ModifierKeys::noModifiers, 0))
    {
        logPanelOpen    = !logPanelOpen;
        logScrollOffset = 0;
        repaint();
        return true;
    }

    return false;
}

void MidiVisuEditor::showJuceAudioSettings()
{
#if JUCE_STANDALONE_APPLICATION
    if (auto* holder = StandalonePluginHolder::getInstance())
    {
        auto* selector = new AudioDeviceSelectorComponent(
            holder->deviceManager,
            0, 0, 0, 0,
            true, false, false, false);
        selector->setSize(500, 400);

        DialogWindow::LaunchOptions opts;
        opts.content.setOwned(selector);
        opts.dialogTitle                  = "Audio / MIDI Settings";
        opts.dialogBackgroundColour       = Colour(0xff1a1a1a);
        opts.escapeKeyTriggersCloseButton = true;
        opts.useNativeTitleBar            = true;
        opts.resizable                    = true;
        opts.launchAsync();
    }
#endif
}

// ── Circle drag ───────────────────────────────────────────────────────────────
void MidiVisuEditor::mouseDown(const MouseEvent& e)
{
    draggedCircle = -1;
    for (int i = 6; i >= 0; --i)
    {
        const float r    = (i < 4) ? jmax(drumRestRadius, drumSmoothedRadius[i])
                                   : jmax(minRadius,      smoothedRadius[i - 3]);
        const float hitR = jmax(r, 30.0f);
        if (e.position.getDistanceFrom(circlePos[i]) <= hitR)
        {
            draggedCircle = i;
            dragOffset    = circlePos[i] - e.position;
            break;
        }
    }
}

void MidiVisuEditor::mouseDrag(const MouseEvent& e)
{
    if (draggedCircle < 0) return;
    circlePos[draggedCircle] = e.position + dragOffset;
    repaint();
}

void MidiVisuEditor::mouseUp(const MouseEvent& /*e*/)
{
    if (draggedCircle >= 0)
        writePositionsToFile(getAutoSaveFile());
    draggedCircle = -1;
}

void MidiVisuEditor::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& w)
{
    if (! logPanelOpen) return;
    if (e.x < getWidth() - logPanelWidth) return;

    const int lineH        = 18;
    const int visibleLines = jmax(1, getHeight() / lineH);
    const int maxOff       = jmax(0, logLines.size() - visibleLines);

    logScrollOffset -= roundToInt(w.deltaY * 30.0f);
    logScrollOffset  = jlimit(0, maxOff, logScrollOffset);
    repaint();
}
