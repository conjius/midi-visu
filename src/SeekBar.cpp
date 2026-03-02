#include "SeekBar.h"
#include "StyleTokens.h"
#include "RangeSliderLogic.h"

SeekBar::SeekBar() {
    // Loop slider: visual only, non-interactive.
    loopSlider_.setSliderStyle(Slider::TwoValueHorizontal);
    loopSlider_.setRange(0.0, 1.0, 0.0);
    loopSlider_.setMinAndMaxValues(0.0, 1.0, dontSendNotification);
    loopSlider_.setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
    loopSlider_.setInterceptsMouseClicks(false, false);
    // Transparent track/background — only thumbs visible.
    loopSlider_.setColour(Slider::backgroundColourId, Colours::transparentBlack);
    loopSlider_.setColour(Slider::trackColourId, Colours::transparentBlack);
    addAndMakeVisible(loopSlider_);
}

void SeekBar::resized() {
    // Offset so the RangeSlider's track (which starts at its own kThumbRadius=10)
    // aligns with our track (which starts at kThumbRadius=8).
    const int off = static_cast<int>(RangeSlider::kThumbRadius - kThumbRadius);
    loopSlider_.setBounds(-off, 0, getWidth() + 2 * off, kTrackAreaHeight);
}

float SeekBar::getTrackStart() const { return kThumbRadius; }

float SeekBar::getTrackWidth() const {
    return static_cast<float>(getWidth()) - 2.0f * kThumbRadius;
}

void SeekBar::setMaxValue(double v) {
    logic.setMaxValue(v);
    syncLoopSlider();
    repaint();
}

void SeekBar::setPlayhead(double v) {
    logic.setPlayhead(v);
    repaint();
}

void SeekBar::setLoopStart(double v) {
    logic.setLoopStart(v);
    syncLoopSlider();
    repaint();
}

void SeekBar::setLoopEnd(double v) {
    logic.setLoopEnd(v);
    syncLoopSlider();
    repaint();
}

double SeekBar::getLoopStart() const { return logic.loopStart(); }
double SeekBar::getLoopEnd() const { return logic.loopEnd(); }
double SeekBar::getPlayhead() const { return logic.playhead(); }

void SeekBar::setLoopEnabled(bool enabled) {
    loopEnabled_ = enabled;
    loopSlider_.setAlpha(enabled ? 1.0f : 0.3f);
    repaint();
}

bool SeekBar::isLoopEnabled() const { return loopEnabled_; }

void SeekBar::addListener(Listener* l) { listeners.add(l); }
void SeekBar::removeListener(Listener* l) { listeners.remove(l); }

bool SeekBar::isDragging() const {
    return activeHandle != MultiHandleSliderLogic::HandleType::None;
}

void SeekBar::syncLoopSlider() {
    const double maxVal = logic.maxValue();
    if (maxVal <= 0.0) return;
    if (loopSlider_.getRange().getEnd() != maxVal)
        loopSlider_.setRange(0.0, maxVal, 0.0);
    loopSlider_.setMinAndMaxValues(logic.loopStart(), logic.loopEnd(),
                                   dontSendNotification);
}

void SeekBar::paint(Graphics& g) {
    const float ts = getTrackStart();
    const float tw = getTrackWidth();
    const float trackCY = static_cast<float>(kTrackAreaHeight) * 0.5f;
    const float trackY = trackCY - 2.0f;
    const float trackH = 4.0f;
    const double maxVal = logic.maxValue();

    // Background track
    g.setColour(Colour(StyleTokens::kSeekBarTrack));
    g.fillRoundedRectangle(ts, trackY, tw, trackH, 2.0f);

    if (maxVal <= 0.0)
        return;

    // Elapsed fill (0 to playhead)
    const float phPx = RangeSliderLogic::valueToPixel(
        logic.playhead(), 0.0, maxVal, ts, tw);
    g.setColour(Colour(StyleTokens::kSeekBarPlayhead).withAlpha(0.45f));
    g.fillRoundedRectangle(ts, trackY, phPx - ts, trackH, 2.0f);

    // Loop region highlight
    const float lsPx = RangeSliderLogic::valueToPixel(
        logic.loopStart(), 0.0, maxVal, ts, tw);
    const float lePx = RangeSliderLogic::valueToPixel(
        logic.loopEnd(), 0.0, maxVal, ts, tw);

    const auto loopCol = Colour(StyleTokens::kSeekBarLoop);
    g.setColour(loopEnabled_ ? loopCol : loopCol.withMultipliedAlpha(0.3f));
    g.fillRoundedRectangle(lsPx, trackY, lePx - lsPx, trackH, 2.0f);

    // Loop handles are drawn by loopSlider_ (JUCE blue thumbs)

    // Playhead handle (white, drawn on top)
    g.setColour(Colour(StyleTokens::kSeekBarPlayhead));
    g.fillEllipse(phPx - kThumbRadius, trackCY - kThumbRadius,
                  kThumbRadius * 2.0f, kThumbRadius * 2.0f);

    // Loop time labels below the track area
    const auto labelCol = Colour(StyleTokens::kSeekBarTime);
    g.setColour(loopEnabled_ ? labelCol : labelCol.withMultipliedAlpha(0.3f));
    g.setFont(Font(FontOptions().withHeight(StyleTokens::kLabelSize)));
    const int labelY = kTrackAreaHeight;
    const auto lsText = String(MultiHandleSliderLogic::formatTime(logic.loopStart()));
    const auto leText = String(MultiHandleSliderLogic::formatTime(logic.loopEnd()));
    g.drawText(lsText, static_cast<int>(lsPx - 18), labelY, 36, kLabelHeight,
               Justification::centred, false);
    g.drawText(leText, static_cast<int>(lePx - 18), labelY, 36, kLabelHeight,
               Justification::centred, false);
}

void SeekBar::mouseDown(const MouseEvent& e) {
    const float mx = static_cast<float>(e.x);
    activeHandle = logic.hitTest(mx, getTrackStart(), getTrackWidth(), kThumbRadius);

    if (activeHandle == MultiHandleSliderLogic::HandleType::MiddleZone) {
        dragStartMouseX = mx;
        dragStartLoopMin = logic.loopStart();
        dragStartLoopMax = logic.loopEnd();
    } else if (activeHandle == MultiHandleSliderLogic::HandleType::None) {
        // Click-to-seek: move playhead to click position
        logic.dragHandle(MultiHandleSliderLogic::HandleType::Playhead,
                         mx, getTrackStart(), getTrackWidth());
        activeHandle = MultiHandleSliderLogic::HandleType::Playhead;
        listeners.call([this](Listener& l) { l.seekBarPlayheadDragged(this); });
        repaint();
    }
}

void SeekBar::mouseDrag(const MouseEvent& e) {
    const float mx = static_cast<float>(e.x);

    if (activeHandle == MultiHandleSliderLogic::HandleType::MiddleZone) {
        const float delta = mx - dragStartMouseX;
        logic.dragMiddleZone(dragStartLoopMin, dragStartLoopMax,
                             delta, getTrackStart(), getTrackWidth());
        syncLoopSlider();
        listeners.call([this](Listener& l) { l.seekBarLoopChanged(this); });
    }
    else if (activeHandle == MultiHandleSliderLogic::HandleType::Playhead) {
        logic.dragHandle(activeHandle, mx, getTrackStart(), getTrackWidth());
        listeners.call([this](Listener& l) { l.seekBarPlayheadDragged(this); });
    }
    else if (activeHandle != MultiHandleSliderLogic::HandleType::None) {
        logic.dragHandle(activeHandle, mx, getTrackStart(), getTrackWidth());
        syncLoopSlider();
        listeners.call([this](Listener& l) { l.seekBarLoopChanged(this); });
    }

    repaint();
}

void SeekBar::mouseUp(const MouseEvent&) {
    activeHandle = MultiHandleSliderLogic::HandleType::None;
}

void SeekBar::mouseWheelMove(const MouseEvent&, const MouseWheelDetails& w) {
    if (!isMouseButtonDown()) return;
    if (logic.maxValue() <= 0.0) return;
    const double interval = logic.maxValue() / 100.0;
    const double delta = interval * w.deltaY * (w.isSmooth ? 1.0 : 10.0);
    logic.setPlayhead(logic.playhead() + delta);
    listeners.call([this](Listener& l) { l.seekBarPlayheadDragged(this); });
    repaint();
}
