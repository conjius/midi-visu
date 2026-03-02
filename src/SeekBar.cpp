#include "SeekBar.h"
#include "StyleTokens.h"
#include "RangeSliderLogic.h"

SeekBar::SeekBar() {
}

float SeekBar::getTrackStart() const { return kThumbRadius; }

float SeekBar::getTrackWidth() const {
    return static_cast<float>(getWidth()) - 2.0f * kThumbRadius;
}

void SeekBar::setMaxValue(double v) {
    logic.setMaxValue(v);
    repaint();
}

void SeekBar::setPlayhead(double v) {
    logic.setPlayhead(v);
    repaint();
}

void SeekBar::setLoopStart(double v) {
    logic.setLoopStart(v);
    repaint();
}

void SeekBar::setLoopEnd(double v) {
    logic.setLoopEnd(v);
    repaint();
}

double SeekBar::getLoopStart() const { return logic.loopStart(); }
double SeekBar::getLoopEnd() const { return logic.loopEnd(); }
double SeekBar::getPlayhead() const { return logic.playhead(); }

void SeekBar::addListener(Listener* l) { listeners.add(l); }
void SeekBar::removeListener(Listener* l) { listeners.remove(l); }

bool SeekBar::isDragging() const {
    return activeHandle != MultiHandleSliderLogic::HandleType::None;
}

void SeekBar::paint(Graphics& g) {
    const float ts = getTrackStart();
    const float tw = getTrackWidth();
    const float h = static_cast<float>(getHeight());
    const float trackY = h * 0.5f - 2.0f;
    const float trackH = 4.0f;
    const double maxVal = logic.maxValue();

    // Background track
    g.setColour(Colour(StyleTokens::kSeekBarTrack));
    g.fillRoundedRectangle(ts, trackY, tw, trackH, 2.0f);

    if (maxVal <= 0.0)
        return;

    // Loop region highlight
    const float lsPx = RangeSliderLogic::valueToPixel(
        logic.loopStart(), 0.0, maxVal, ts, tw);
    const float lePx = RangeSliderLogic::valueToPixel(
        logic.loopEnd(), 0.0, maxVal, ts, tw);
    g.setColour(Colour(StyleTokens::kSeekBarLoop));
    g.fillRoundedRectangle(lsPx, trackY, lePx - lsPx, trackH, 2.0f);

    // Loop handles (smaller circles)
    const float smallR = kThumbRadius * 0.6f;
    g.setColour(Colour(StyleTokens::kSeekBarHandle));
    g.fillEllipse(lsPx - smallR, h * 0.5f - smallR, smallR * 2.0f, smallR * 2.0f);
    g.fillEllipse(lePx - smallR, h * 0.5f - smallR, smallR * 2.0f, smallR * 2.0f);

    // Playhead handle (larger, brighter)
    const float phPx = RangeSliderLogic::valueToPixel(
        logic.playhead(), 0.0, maxVal, ts, tw);
    g.setColour(Colour(StyleTokens::kSeekBarPlayhead));
    g.fillEllipse(phPx - kThumbRadius, h * 0.5f - kThumbRadius,
                  kThumbRadius * 2.0f, kThumbRadius * 2.0f);

    // Time label is drawn by UiManager below the seekbar
}

void SeekBar::mouseDown(const MouseEvent& e) {
    const float mx = static_cast<float>(e.x);
    activeHandle = logic.hitTest(mx, getTrackStart(), getTrackWidth(), kThumbRadius);

    if (activeHandle == MultiHandleSliderLogic::HandleType::MiddleZone) {
        dragStartMouseX = mx;
        dragStartLoopMin = logic.loopStart();
        dragStartLoopMax = logic.loopEnd();
    }
}

void SeekBar::mouseDrag(const MouseEvent& e) {
    const float mx = static_cast<float>(e.x);

    if (activeHandle == MultiHandleSliderLogic::HandleType::MiddleZone) {
        const float delta = mx - dragStartMouseX;
        logic.dragMiddleZone(dragStartLoopMin, dragStartLoopMax,
                             delta, getTrackStart(), getTrackWidth());
        listeners.call([this](Listener& l) { l.seekBarLoopChanged(this); });
    }
    else if (activeHandle == MultiHandleSliderLogic::HandleType::Playhead) {
        logic.dragHandle(activeHandle, mx, getTrackStart(), getTrackWidth());
        listeners.call([this](Listener& l) { l.seekBarPlayheadDragged(this); });
    }
    else if (activeHandle != MultiHandleSliderLogic::HandleType::None) {
        logic.dragHandle(activeHandle, mx, getTrackStart(), getTrackWidth());
        listeners.call([this](Listener& l) { l.seekBarLoopChanged(this); });
    }

    repaint();
}

void SeekBar::mouseUp(const MouseEvent&) {
    activeHandle = MultiHandleSliderLogic::HandleType::None;
}