#include "InteractionManager.h"
#include "PluginEditor.h"
#include "OptionsPanelLayout.h"

#if JUCE_STANDALONE_APPLICATION
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif

InteractionManager::InteractionManager(MidiVisuEditor& e)
    : editor(e) {
}

// ── Mouse ─────────────────────────────────────────────────────────────────────

void InteractionManager::mouseDown(const MouseEvent& e) {
    draggedCircle = -1;

    if (editor.optionsPanelOpen) {
        const int pxPanel = editor.getWidth() - editor.logPanelWidth;
        const int pad = 10;

        if (e.x >= pxPanel && e.x < editor.getWidth()) {
            const auto& layout = editor.optionsLayout;

            // Section header click — toggle fold
            const int contentY = e.y + layout.scrollOffset();
            auto section = layout.hitTestHeader(contentY);
            if (section != OptionsPanelLayout::SectionCount) {
                editor.optionsLayout.toggleFolded(section);

                editor.resized();
                editor.repaint();
                return;
            }

            // Video file list click
            if (!layout.isFolded(OptionsPanelLayout::Video)) {
                const int listTop = layout.videoFileListTopY() - layout.scrollOffset();
                const int listH = 2 * 22;
                const int listL = pxPanel + pad;
                const int listW = editor.logPanelWidth - pad * 2;

                if (e.x >= listL && e.x < listL + listW
                    && e.y >= listTop && e.y < listTop + listH) {
                    const int row = (e.y - listTop) / 22;
                    const int fileIdx = editor.videoListScrollOffset + row;
                    if (fileIdx < editor.videoListManager.fileCount()) {
                        editor.videoListManager.setSelectedIndex(fileIdx);
                        const File f = editor.assetsVideoDir.getChildFile(
                            String(editor.videoListManager.filename(fileIdx)));
                        if (f.existsAsFile()) {
                            if (editor.seekBar.isLoopEnabled())
                                editor.videoBackground.setLoopPoints(
                                    editor.seekBar.getLoopStart(),
                                    editor.seekBar.getLoopEnd());
                            editor.videoBackground.loadFile(
                                f.getFullPathName().toRawUTF8());
                            editor.lastVideoFile = f;
                            editor.videoListManager.setPlayState(
                                VideoListManager::PlayState::Playing);
                            editor.videoPlayPauseButton.setButtonText(CharPointer_UTF8("\xe2\x8f\xb8"));
                            editor.writePositionsToFile(editor.getAutoSaveFile());
                        }
                        editor.repaint();
                    }
                    return;
                }
            }
        }
    }

    const float minR_ = static_cast<float>(editor.ballSizeSlider.getMinValue());

    for (int i = 6; i >= 0; --i) {
        const float r = (i < 4)
                            ? jmax(minR_, editor.drumSmoothedRadius[i])
                            : jmax(minR_, editor.smoothedRadius[i - 3]);
        const float hitR = jmax(r, 30.0f);
        if (e.position.getDistanceFrom(editor.circlePos[i] + editor.floatOffset[i]) <=
            hitR) {
            draggedCircle = i;
            dragOffset = editor.circlePos[i] - e.position;
            break;
        }
    }
}

void InteractionManager::mouseDrag(const MouseEvent& e) const {
    if (draggedCircle < 0) return;
    editor.circlePos[draggedCircle] = e.position + dragOffset;
    editor.repaint();
}

void InteractionManager::mouseUp(const MouseEvent& /*e*/) {
    if (draggedCircle >= 0)
        editor.writePositionsToFile(editor.getAutoSaveFile());
    draggedCircle = -1;
}

void InteractionManager::mouseWheelMove(const MouseEvent& e,
                                        const MouseWheelDetails& w) const {
    if (editor.optionsPanelOpen) {
        const int pxPanel = editor.getWidth() - editor.logPanelWidth;
        const int pad = 10;

        if (e.x >= pxPanel && e.x < editor.getWidth()) {
            const auto& layout = editor.optionsLayout;

            // Video file list scroll (only when Video section is expanded)
            if (!layout.isFolded(OptionsPanelLayout::Video)) {
                const int listTop = layout.videoFileListTopY() - layout.scrollOffset();
                const int listH = 2 * 22;
                const int listL = pxPanel + pad;
                const int listW = editor.logPanelWidth - pad * 2;

                if (e.x >= listL && e.x < listL + listW
                    && e.y >= listTop && e.y < listTop + listH) {
                    const int maxOff = jmax(0, editor.videoListManager.fileCount() - 2);
                    editor.videoListScrollOffset -= roundToInt(w.deltaY * 3.0f);
                    editor.videoListScrollOffset =
                        jlimit(0, maxOff, editor.videoListScrollOffset);
                    editor.repaint();
                    return;
                }
            }

            // Options panel scroll
            editor.optionsLayout.scrollBy(-roundToInt(w.deltaY * 200.0f));
            editor.resized();
            editor.repaint();
            return;
        }
    }

    if (!editor.logPanelOpen) return;
    if (e.x > editor.logPanelWidth) return;

    const int logAreaTop = 142;
    const int lineH = 18;
    const int visibleLines = jmax(1, (editor.getHeight() - logAreaTop) / lineH);
    const int maxOff = jmax(0, editor.logLines.size() - visibleLines);

    editor.logScrollOffset -= roundToInt(w.deltaY * 30.0f);
    editor.logScrollOffset = jlimit(0, maxOff, editor.logScrollOffset);
    editor.repaint();
}

// ── Keyboard ──────────────────────────────────────────────────────────────────

bool InteractionManager::keyPressed(const KeyPress& key) const {
    if (key == KeyPress::escapeKey) {
        // Exit fullscreen if active
        if (auto* peer = editor.getPeer())
            if (peer->isFullScreen()) {
                peer->setFullScreen(false);
                editor.setSize(1920, 1080);
                editor.repaint();
                return true;
            }

        const bool anyOpen = editor.optionsPanelOpen || editor.logPanelOpen;

        // Close all panels
        editor.optionsPanelOpen = false;
        editor.logPanelOpen = false;

        // If nothing was open, open both panels
        if (!anyOpen) {
            editor.optionsPanelOpen = true;
            editor.logPanelOpen = true;
            editor.logScrollOffset = 0;
        }


        editor.resized();

        editor.repaint();
        return true;
    }

    if (key == KeyPress('f', ModifierKeys::noModifiers, 0)) {
        if (auto* peer = editor.getPeer())
            peer->setFullScreen(!peer->isFullScreen());
        return true;
    }

    if (key == KeyPress('b', ModifierKeys::noModifiers, 0)) {
        editor.blurToggleButton.setToggleState(!editor.blurToggleButton.getToggleState(),
                                         dontSendNotification);

        editor.repaint();
        return true;
    }

    if (key == KeyPress('o', ModifierKeys::noModifiers, 0)) {
        editor.optionsPanelOpen = !editor.optionsPanelOpen;

        editor.resized();
        editor.repaint();
        return true;
    }

    if (key == KeyPress('l', ModifierKeys::noModifiers, 0)) {
        editor.logPanelOpen = !editor.logPanelOpen;
        editor.logScrollOffset = 0;

        editor.resized();
        editor.repaint();
        return true;
    }

    if (key == KeyPress('s', ModifierKeys::commandModifier, 0)) {
        editor.writePositionsToFile(editor.getAutoSaveFile());
        editor.appendLog("Settings saved", editor.styleManager.logInfo());
        editor.repaint();
        return true;
    }

    if (key == KeyPress('s', ModifierKeys::commandModifier
                             | ModifierKeys::shiftModifier, 0)) {
        editor.savePositions();
        return true;
    }

    if (key == KeyPress('l', ModifierKeys::commandModifier, 0)
        || key == KeyPress('o', ModifierKeys::commandModifier, 0)) {
        editor.loadPositions();
        return true;
    }

    return false;
}