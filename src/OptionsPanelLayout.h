/*
  ==============================================================================
    OptionsPanelLayout — computes Y positions for the options panel given
    section fold states and scroll offset.
    Pure C++, no JUCE dependency, so it can be unit-tested independently.
  ==============================================================================
*/

#pragma once
#include "StyleTokens.h"

class OptionsPanelLayout {
public:
    enum Section { MidiRouting = 0, Video, Circles, Animation, SectionCount };

    // Fold state
    void setFolded(Section s, bool folded);
    bool isFolded(Section s) const;
    void toggleFolded(Section s);

    // Scroll state
    void setScrollOffset(int offset);
    int scrollOffset() const;
    void scrollBy(int delta);

    // Viewport
    void setViewportHeight(int h);
    int viewportHeight() const;

    // Computed values
    int contentHeight() const;
    int maxScrollOffset() const;

    // Y positions in content space (subtract scrollOffset() for screen Y).
    // Fixed header area (above foldable sections)
    static constexpr int kTitleY = 8;
    static constexpr int kFirstDividerY = 30;
    static constexpr int kMidiSettingsButtonY = 34;

    // Section header Y (accounts for fold state of preceding sections)
    int sectionHeaderY(Section s) const;

    // Section content start Y (header Y + header height)
    int sectionContentY(Section s) const;

    // Convenience accessors for specific widget Y offsets within each section.
    // These return absolute content-space Y values.
    int midiRoutingFirstRowY() const;
    int videoTransportY() const;
    int videoSeekBarY() const;
    int videoTimeLabelY() const;
    int videoCtrlY() const;
    int videoFilesY() const;
    int videoFileListTopY() const;
    int circlesSizeRangeY() const;
    int circlesSliderY() const;
    int circlesHandleLabelsY() const;
    int animFloatToggleY() const;
    int animCollisionToggleY() const;
    int animClockToggleY() const;
    int animClockDivY() const;
    int animClockKickSliderY() const;
    int animFloatIntensitySliderY() const;
    int animFloatSpeedSliderY() const;
    int animWobbleSliderY() const;
    int buttonsY() const;

    // Hit-test: which section header contains this content-space Y?
    // Returns SectionCount if no header hit.
    Section hitTestHeader(int contentY) const;

    // Section content heights (when expanded)
    static constexpr int kHeaderH = 20;
    static constexpr int kSectionGap = 20;
    static constexpr int kMidiRoutingContentH = 7 * 26;     // 182
    static constexpr int kVideoContentH = 288 + StyleTokens::kPadding;
    static constexpr int kCirclesContentH = 80;
    static constexpr int kAnimationContentH = 354;
    static constexpr int kButtonsH = 68;

    // Y where the first foldable section starts
    static constexpr int kFirstSectionY = 66;

private:
    bool folded_[SectionCount] = {false, false, false, false};
    int scrollOffset_ = 0;
    int viewportHeight_ = 1080;

    void clampScroll();
};
