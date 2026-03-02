#include "UiManager.h"
#include "PluginEditor.h"
#include "AppConstants.h"
#include "RangeSliderLogic.h"
#include "MultiHandleSliderLogic.h"
#include "StyleTokens.h"

UiManager::UiManager(MidiVisuEditor& e)
    : editor(e) {
}

void UiManager::paint(Graphics& g) const {
    g.fillAll(Colours::black);

    // ── Video ──────────────────────────────────────────────────────────────────
    int srcW = 0, srcH = 0, srcStride = 0;
    const uint8_t* srcPixels = nullptr;
    if (editor.videoToggle.getToggleState()
        && editor.videoBackground.getLatestFrame(srcW, srcH, srcStride, srcPixels)) {
        if (!editor.videoFrame.isValid()
            || editor.videoFrame.getWidth() != srcW
            || editor.videoFrame.getHeight() != srcH)
            editor.videoFrame = Image(Image::ARGB, srcW, srcH, false);

        {
            Image::BitmapData bmp(editor.videoFrame, Image::BitmapData::writeOnly);
            for (int row = 0; row < srcH; ++row) {
                const uint8_t* src = srcPixels + row * srcStride;
                uint8_t* dst = bmp.getLinePointer(row);
                for (int col = 0; col < srcW; ++col) {
                    // Source is BGRA; JUCE Image::ARGB same byte layout on LE macOS.
                    const uint8_t pb = src[col * 4 + 0];
                    const uint8_t pg = src[col * 4 + 1];
                    const uint8_t pr = src[col * 4 + 2];
                    const uint8_t pa = src[col * 4 + 3];
                    // BT.601 luma (integer arithmetic, no FP per pixel)
                    const uint8_t y = (uint8_t)((77 * pr + 150 * pg + 29 * pb) >> 8);
                    dst[col * 4 + 0] = y;
                    dst[col * 4 + 1] = y;
                    dst[col * 4 + 2] = y;
                    dst[col * 4 + 3] = pa;
                }
            }
        }

        if (editor.blurToggle.getToggleState()) {
            const float r = (float)editor.blurSlider.getValue();
            if (r > 0.0f)
                editor.videoFrame.getPixelData()->applyGaussianBlurEffect(r);
        }

        const float videoZoom = (float)editor.videoZoomSlider.getValue();
        const int destW = editor.getWidth(), destH = editor.getHeight();
        const int cropW = roundToInt((float)srcW / videoZoom);
        const int cropH = roundToInt((float)srcW * destH / ((float)destW * videoZoom));
        const int cropX = (srcW - cropW) / 2, cropY = (srcH - cropH) / 2;

        g.setOpacity((float)editor.videoOpacitySlider.getValue());
        g.drawImage(editor.videoFrame, 0, 0, destW, destH, cropX, cropY, cropW, cropH);
    }

    // ── Circles ────────────────────────────────────────────────────────────────
    g.setOpacity(1.0f);

    for (int v = 0; v < 4; ++v) {
        const float r = editor.drumSmoothedRadius[v];
        const float cx = editor.circlePos[v].x + editor.floatOffset[v].x;
        const float cy = editor.circlePos[v].y + editor.floatOffset[v].y;
        g.setColour(drumColours[v]);
        g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
    }

    for (int i = 1; i < 4; ++i) {
        const float r = editor.smoothedRadius[i];
        const float cx = editor.circlePos[3 + i].x + editor.floatOffset[3 + i].x;
        const float cy = editor.circlePos[3 + i].y + editor.floatOffset[3 + i].y;
        g.setColour(kChColours[i]);
        g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
    }

    // ── Log panel background (left side, drawn over circles) ───────────────────
    if (editor.logPanelOpen) {
        g.setColour(editor.styleManager.logBackground());
        g.fillRect(0, 0, editor.logPanelWidth, editor.getHeight());
        g.setColour(editor.styleManager.border());
        g.drawVerticalLine(editor.logPanelWidth, 0.0f, (float)editor.getHeight());
    }

    // ── Options panel ──────────────────────────────────────────────────────────
    if (editor.optionsPanelOpen) {
        const int px = editor.getWidth() - editor.logPanelWidth;
        const int pad = 10;
        const int rowH = 26;
        const auto monoFont = editor.styleManager.monoFont();
        const auto labelFont = editor.styleManager.headingFont();

        // Background + left border
        g.setColour(editor.styleManager.panelBackground());
        g.fillRect(px, 0, editor.logPanelWidth, editor.getHeight());
        g.setColour(editor.styleManager.border());
        g.drawVerticalLine(px, 0.0f, (float)editor.getHeight());

        // "OPTIONS" header
        g.setFont(labelFont);
        g.setColour(editor.styleManager.panelTitle());
        g.drawText("OPTIONS", px + pad, 8, editor.logPanelWidth - pad * 2, 18,
                   Justification::left);
        g.setColour(editor.styleManager.divider());
        g.drawHorizontalLine(30, (float)(px + pad), (float)(editor.getWidth() - pad));

        // "MIDI ROUTING" section
        g.setFont(monoFont);
        g.setColour(editor.styleManager.sectionHeading());
        g.drawText("MIDI ROUTING", px + pad, 66, editor.logPanelWidth - pad * 2, 16,
                   Justification::left);

        const int firstY = 88;
        for (int i = 0; i < 7; ++i) {
            const int rowY = firstY + i * rowH;
            g.setColour(kVoiceColours[i]);
            g.fillEllipse((float)(px + pad), (float)(rowY + 6), 10.0f, 10.0f);
            g.setColour(editor.styleManager.secondary());
            g.setFont(monoFont);
            g.drawText("ch", px + 92, rowY + 5, 18, 16, Justification::left);
        }

        // Divider + VIDEO section header
        const int videoY = firstY + 7 * rowH + StyleTokens::kSectionGap;
        g.setColour(editor.styleManager.divider());
        g.drawHorizontalLine(videoY - StyleTokens::kDividerGap, (float)(px + pad),
                             (float)(editor.getWidth() - pad));
        g.setFont(monoFont);
        g.setColour(editor.styleManager.sectionHeading());
        g.drawText("VIDEO", px + pad, videoY, editor.logPanelWidth - pad * 2, 16,
                   Justification::left);

        // Seekbar time label: "MM:SS / MM:SS"
        {
            const int labelY = videoY + 42;
            const auto curText = String(
                MultiHandleSliderLogic::formatTime(editor.seekBar.getPlayhead()));
            const double maxVal = editor.videoBackground.duration();
            const auto totText = String(
                MultiHandleSliderLogic::formatTime(maxVal > 0.0 ? maxVal : 0.0));
            g.setFont(monoFont);
            g.setColour(Colour(StyleTokens::kSeekBarTime));
            g.drawText(curText + " / " + totText,
                       px + pad, labelY, editor.logPanelWidth - pad * 2, 14,
                       Justification::centred);
        }

        // Labels for toggles/sliders below buttons
        const int ctrlY = videoY + 96;
        g.setFont(monoFont);
        g.setColour(editor.styleManager.label());
        g.drawText("Blur radius", px + pad, ctrlY + 56, editor.logPanelWidth - pad * 2,
                   14,
                   Justification::left);
        g.drawText("Zoom", px + pad, ctrlY + 110, editor.logPanelWidth - pad * 2, 14,
                   Justification::left);
        g.drawText("Opacity", px + pad, ctrlY + 164, editor.logPanelWidth - pad * 2, 14,
                   Justification::left);

        // Divider + FILES sub-section
        const int filesY = ctrlY + 210;
        g.setColour(editor.styleManager.divider());
        g.drawHorizontalLine(filesY - StyleTokens::kDividerGap, (float)(px + pad),
                             (float)(editor.getWidth() - pad));
        g.setFont(monoFont);
        g.setColour(editor.styleManager.label());
        g.drawText("FILES", px + pad, filesY, editor.logPanelWidth - pad * 2, 12,
                   Justification::left);

        // Video file list (2 visible rows)
        {
            const int listTop = filesY + 16;
            const int listRows = 2;
            const int rowHList = StyleTokens::kListRowHeight;
            const int listH = listRows * rowHList;
            const int listL = px + pad;
            const int listW = editor.logPanelWidth - pad * 2;
            const int sbW = StyleTokens::kScrollbarW;
            const int scrollOff = editor.videoListScrollOffset;

            g.setColour(editor.styleManager.border());
            g.drawRect(listL, listTop, listW, listH);

            g.setFont(monoFont);
            for (int row = 0; row < listRows; ++row) {
                const int fileIdx = scrollOff + row;
                if (fileIdx >= editor.videoListManager.fileCount()) break;
                const int rowY2 = listTop + row * rowHList;
                const bool sel = (fileIdx == editor.videoListManager.selectedIndex());
                if (sel) {
                    g.setColour(editor.styleManager.listRowSelected());
                    g.fillRect(listL + 1, rowY2 + 1, listW - 2, rowHList - 1);
                }
                g.setColour(sel
                                ? editor.styleManager.value()
                                : editor.styleManager.secondary());

                // Filename on the left
                const auto fname = String(editor.videoListManager.filename(fileIdx));
                const int durW = 40;
                g.drawText(fname,
                           listL + 4, rowY2 + 4, listW - sbW - durW - 8, rowHList - 4,
                           Justification::left, true);
                // Duration on the right
                const auto durText = String(MultiHandleSliderLogic::formatTime(
                    editor.videoListManager.duration(fileIdx)));
                g.drawText(durText,
                           listL + listW - sbW - durW - 4, rowY2 + 4, durW, rowHList - 4,
                           Justification::right);
            }

            if (editor.videoListManager.fileCount() == 0) {
                g.setColour(editor.styleManager.label());
                g.drawText("(no videos in assets/video/)", listL, listTop, listW, listH,
                           Justification::centred);
            }

            if (editor.videoListManager.fileCount() > listRows) {
                const float sbX = (float)(listL + listW - sbW);
                const float totalH = (float)listH;
                const int maxOff = editor.videoListManager.fileCount() - listRows;
                const float thumbFrac = (float)listRows / (float)editor.videoListManager.
                    fileCount();
                const float thumbH = jmax(12.0f, thumbFrac * totalH);
                const float thumbY = (float)listTop + ((maxOff > 0)
                                                           ? ((float)scrollOff / maxOff) *
                                                           (totalH - thumbH)
                                                           : 0.0f);
                g.setColour(editor.styleManager.scrollTrack());
                g.fillRect((int)sbX, listTop, sbW, listH);
                g.setColour(editor.styleManager.secondary());
                g.fillRoundedRectangle(sbX, thumbY, (float)sbW, thumbH, 2.0f);
            }
        }

        // Divider + CIRCLES section header
        const int circlesY = filesY + 76;
        g.setColour(editor.styleManager.divider());
        g.drawHorizontalLine(circlesY - 10, (float)(px + pad),
                             (float)(editor.getWidth() - pad));
        g.setFont(monoFont);
        g.setColour(editor.styleManager.sectionHeading());
        g.drawText("CIRCLES", px + pad, circlesY, editor.logPanelWidth - pad * 2, 16,
                   Justification::left);
        g.setFont(monoFont);
        g.setColour(editor.styleManager.label());
        g.drawText("Size range", px + pad, circlesY + 18, editor.logPanelWidth - pad * 2,
                   14,
                   Justification::left);

        // Handle position labels for the TwoValueHorizontal slider
        {
            const float sliderX = (float)(px + pad);
            const float sliderW = (float)(editor.logPanelWidth - pad * 2);
            const float thumbR = 10.f;
            const float trackW = sliderW - 2.f * thumbR;
            const float sMin = 5.f, sMax = 500.f;
            const float minVal = (float)editor.ballSizeSlider.getMinValue();
            const float maxVal = (float)editor.ballSizeSlider.getMaxValue();
            const float trackStart = sliderX + thumbR;
            const float minX = RangeSliderLogic::valueToPixel(
                minVal, sMin, sMax, trackStart, trackW);
            const float maxX = RangeSliderLogic::valueToPixel(
                maxVal, sMin, sMax, trackStart, trackW);
            g.setFont(monoFont);
            g.setColour(editor.styleManager.value());
            g.drawText(String((int)minVal), (int)(minX - 15.f), circlesY + 66, 30, 14,
                       Justification::centred, false);
            g.drawText(String((int)maxVal), (int)(maxX - 15.f), circlesY + 66, 30, 14,
                       Justification::centred, false);
        }

        // Divider + ANIMATION section header
        const int animY = circlesY + 90;
        g.setColour(editor.styleManager.divider());
        g.drawHorizontalLine(animY - 10, (float)(px + pad),
                             (float)(editor.getWidth() - pad));
        g.setFont(monoFont);
        g.setColour(editor.styleManager.sectionHeading());
        g.drawText("ANIMATION", px + pad, animY, editor.logPanelWidth - pad * 2, 16,
                   Justification::left);

        g.setFont(monoFont);
        g.setColour(editor.styleManager.label());
        g.drawText("Clock sync", px + pad, animY + 104, editor.logPanelWidth - pad * 2,
                   14,
                   Justification::left);
        g.drawText("Kick amount", px + pad, animY + 154, editor.logPanelWidth - pad * 2,
                   14,
                   Justification::left);
        g.drawText("Intensity", px + pad, animY + 204, editor.logPanelWidth - pad * 2, 14,
                   Justification::left);
        g.drawText("Speed", px + pad, animY + 254, editor.logPanelWidth - pad * 2, 14,
                   Justification::left);

        // Divider above save/load buttons
        const int btnsY = animY + 304;
        g.setColour(editor.styleManager.divider());
        g.drawHorizontalLine(btnsY - 10, (float)(px + pad),
                             (float)(editor.getWidth() - pad));
    }
}

void UiManager::paintOverChildren(Graphics& g) const {
    if (!editor.logPanelOpen) return;

    const int panelX = 0;
    const int panelW = editor.logPanelWidth;
    const int panelH = editor.getHeight();
    const int sbW = 6;
    const int pad = 8;
    const int logAreaTop = 142;
    const int lineH = 18;
    const int visibleLines = jmax(1, (panelH - logAreaTop) / lineH);
    const int maxOff = jmax(0, editor.logLines.size() - visibleLines);
    const int offset = jlimit(0, maxOff, editor.logScrollOffset);
    const auto monoFont = editor.styleManager.monoFont();
    const auto labelFont = editor.styleManager.headingFont();

    // "LOGGER" header
    g.setFont(labelFont);
    g.setColour(editor.styleManager.panelTitle());
    g.drawText("LOGGER", panelX + pad, 8, panelW - pad * 2, 18, Justification::left);
    g.setColour(editor.styleManager.divider());
    g.drawHorizontalLine(30, (float)(panelX + pad), (float)(panelX + panelW - pad));

    // "FILTERS" section label
    g.setFont(monoFont);
    g.setColour(editor.styleManager.sectionHeading());
    g.drawText("FILTERS", panelX + pad, 36, panelW - pad * 2, 14, Justification::left);

    // Divider below filter toggles
    g.setColour(editor.styleManager.divider());
    g.drawHorizontalLine(108, (float)(panelX + pad), (float)(panelX + panelW - pad));

    // Divider above log area
    g.drawHorizontalLine(138, (float)(panelX + pad), (float)(panelX + panelW - pad));

    // Log lines (latest first, coloured by instrument)
    g.setFont(editor.styleManager.logFont());
    for (int i = 0; i < visibleLines; ++i) {
        const int idx = offset + i;
        if (idx >= editor.logLines.size()) break;
        g.setColour(idx < editor.logColors.size()
                        ? editor.logColors[idx]
                        : editor.styleManager.logFallback());
        g.drawText(editor.logLines[idx],
                   panelX + pad, logAreaTop + i * lineH,
                   panelW - sbW - pad * 2, lineH,
                   Justification::left, false);
    }

    // Scrollbar
    if (editor.logLines.size() > visibleLines) {
        const float sbX = (float)(panelX + panelW - sbW);
        const float totalH = (float)(panelH - logAreaTop);
        const float thumbFrac = (float)visibleLines / (float)editor.logLines.size();
        const float thumbH = jmax(20.0f, thumbFrac * totalH);
        const float thumbY = logAreaTop + ((maxOff > 0)
                                               ? ((float)offset / maxOff) * (totalH -
                                                   thumbH)
                                               : 0.0f);

        g.setColour(editor.styleManager.scrollTrack());
        g.fillRect((int)sbX, logAreaTop, sbW, panelH - logAreaTop);

        g.setColour(editor.styleManager.secondary());
        g.fillRoundedRectangle(sbX, thumbY, (float)sbW, thumbH, 3.0f);
    }
}