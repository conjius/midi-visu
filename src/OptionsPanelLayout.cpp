#include "OptionsPanelLayout.h"

#include <algorithm>

// ── Fold state ───────────────────────────────────────────────────────────────

void OptionsPanelLayout::setFolded(Section s, bool folded) {
    if (s >= 0 && s < SectionCount) {
        folded_[s] = folded;
        clampScroll();
    }
}

bool OptionsPanelLayout::isFolded(Section s) const {
    return s >= 0 && s < SectionCount && folded_[s];
}

void OptionsPanelLayout::toggleFolded(Section s) {
    if (s >= 0 && s < SectionCount) {
        folded_[s] = !folded_[s];
        clampScroll();
    }
}

// ── Scroll state ─────────────────────────────────────────────────────────────

void OptionsPanelLayout::setScrollOffset(int offset) {
    scrollOffset_ = offset;
    clampScroll();
}

int OptionsPanelLayout::scrollOffset() const {
    return scrollOffset_;
}

void OptionsPanelLayout::scrollBy(int delta) {
    scrollOffset_ += delta;
    clampScroll();
}

// ── Viewport ─────────────────────────────────────────────────────────────────

void OptionsPanelLayout::setViewportHeight(int h) {
    viewportHeight_ = h;
    clampScroll();
}

int OptionsPanelLayout::viewportHeight() const {
    return viewportHeight_;
}

// ── Computed values ──────────────────────────────────────────────────────────

int OptionsPanelLayout::contentHeight() const {
    // Walk sections to compute total height
    int y = kFirstSectionY;
    for (int s = 0; s < SectionCount; ++s) {
        y += kHeaderH + kSectionGap; // header + gap before next section
        if (!folded_[s]) {
            switch (static_cast<Section>(s)) {
                case MidiRouting: y += kMidiRoutingContentH; break;
                case Video:       y += kVideoContentH; break;
                case Circles:     y += kCirclesContentH; break;
                case Animation:   y += kAnimationContentH; break;
                default: break;
            }
        }
    }
    y += kButtonsH; // save/load buttons always visible
    return y;
}

int OptionsPanelLayout::maxScrollOffset() const {
    return std::max(0, contentHeight() - viewportHeight_);
}

// ── Y positions ──────────────────────────────────────────────────────────────

int OptionsPanelLayout::sectionHeaderY(Section s) const {
    int y = kFirstSectionY;
    for (int i = 0; i < s; ++i) {
        y += kHeaderH + kSectionGap;
        if (!folded_[i]) {
            switch (static_cast<Section>(i)) {
                case MidiRouting: y += kMidiRoutingContentH; break;
                case Video:       y += kVideoContentH; break;
                case Circles:     y += kCirclesContentH; break;
                case Animation:   y += kAnimationContentH; break;
                default: break;
            }
        }
    }
    return y;
}

int OptionsPanelLayout::sectionContentY(Section s) const {
    return sectionHeaderY(s) + kHeaderH;
}

// MIDI ROUTING content: voice rows start 2px below header bottom (88 = 66 + 22)
int OptionsPanelLayout::midiRoutingFirstRowY() const {
    return sectionContentY(MidiRouting) + 2;
}

// VIDEO content offsets (relative to VIDEO header Y)
// Transport buttons (play/pause, stop, loop, eye, blur) on first line.
int OptionsPanelLayout::videoTransportY() const {
    return sectionHeaderY(Video) + 18;
}

// Seekbar on its own line below transport buttons.
int OptionsPanelLayout::videoSeekBarY() const {
    return sectionHeaderY(Video) + 46;
}

int OptionsPanelLayout::videoTimeLabelY() const {
    return sectionHeaderY(Video) + 88;
}

int OptionsPanelLayout::videoCtrlY() const {
    return sectionHeaderY(Video) + 106;
}

int OptionsPanelLayout::videoFilesY() const {
    return videoCtrlY() + 134;
}

int OptionsPanelLayout::videoFileListTopY() const {
    return videoFilesY() + 16;
}

// CIRCLES content offsets (relative to CIRCLES header Y)
int OptionsPanelLayout::circlesSizeRangeY() const {
    return sectionHeaderY(Circles) + 18;
}

int OptionsPanelLayout::circlesSliderY() const {
    return sectionHeaderY(Circles) + 34;
}

int OptionsPanelLayout::circlesHandleLabelsY() const {
    return sectionHeaderY(Circles) + 66;
}

// ANIMATION content offsets (relative to ANIMATION header Y)
int OptionsPanelLayout::animFloatToggleY() const {
    return sectionHeaderY(Animation) + 20;
}

int OptionsPanelLayout::animCollisionToggleY() const {
    return sectionHeaderY(Animation) + 46;
}

int OptionsPanelLayout::animClockToggleY() const {
    return sectionHeaderY(Animation) + 72;
}

int OptionsPanelLayout::animClockDivY() const {
    return sectionHeaderY(Animation) + 118;
}

int OptionsPanelLayout::animClockKickSliderY() const {
    return sectionHeaderY(Animation) + 168;
}

int OptionsPanelLayout::animFloatIntensitySliderY() const {
    return sectionHeaderY(Animation) + 218;
}

int OptionsPanelLayout::animFloatSpeedSliderY() const {
    return sectionHeaderY(Animation) + 268;
}

int OptionsPanelLayout::buttonsY() const {
    int y = kFirstSectionY;
    for (int i = 0; i < SectionCount; ++i) {
        y += kHeaderH + kSectionGap;
        if (!folded_[i]) {
            switch (static_cast<Section>(i)) {
                case MidiRouting: y += kMidiRoutingContentH; break;
                case Video:       y += kVideoContentH; break;
                case Circles:     y += kCirclesContentH; break;
                case Animation:   y += kAnimationContentH; break;
                default: break;
            }
        }
    }
    return y;
}

// ── Hit-test ─────────────────────────────────────────────────────────────────

OptionsPanelLayout::Section OptionsPanelLayout::hitTestHeader(int contentY) const {
    for (int s = 0; s < SectionCount; ++s) {
        const int hy = sectionHeaderY(static_cast<Section>(s));
        if (contentY >= hy && contentY < hy + kHeaderH)
            return static_cast<Section>(s);
    }
    return SectionCount;
}

// ── Internal ─────────────────────────────────────────────────────────────────

void OptionsPanelLayout::clampScroll() {
    const int max = maxScrollOffset();
    if (scrollOffset_ < 0) scrollOffset_ = 0;
    if (scrollOffset_ > max) scrollOffset_ = max;
}
