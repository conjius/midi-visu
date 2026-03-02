#include "MultiHandleSliderLogic.h"
#include "RangeSliderLogic.h"
#include <algorithm>
#include <cmath>

MultiHandleSliderLogic::MultiHandleSliderLogic(double maxValue)
    : loopEnd_(std::max(0.0, maxValue)),
      maxValue_(std::max(0.0, maxValue)) {
}

void MultiHandleSliderLogic::setLoopStart(double v) {
    loopStart_ = std::clamp(v, 0.0, loopEnd_);
}

void MultiHandleSliderLogic::setPlayhead(double v) {
    playhead_ = std::clamp(v, 0.0, maxValue_);
}

void MultiHandleSliderLogic::setLoopEnd(double v) {
    loopEnd_ = std::clamp(v, loopStart_, maxValue_);
}

void MultiHandleSliderLogic::setMaxValue(double v) {
    maxValue_ = std::max(0.0, v);
    loopEnd_ = std::min(loopEnd_, maxValue_);
    loopStart_ = std::min(loopStart_, loopEnd_);
    playhead_ = std::min(playhead_, maxValue_);
}

MultiHandleSliderLogic::HandleType
MultiHandleSliderLogic::hitTest(float mouseX,
                                float trackStart, float trackWidth,
                                float thumbRadius) const {
    if (maxValue_ <= 0.0)
        return HandleType::None;

    const float lsPx = RangeSliderLogic::valueToPixel(loopStart_, 0.0, maxValue_,
                                                      trackStart, trackWidth);
    const float phPx = RangeSliderLogic::valueToPixel(playhead_, 0.0, maxValue_,
                                                      trackStart, trackWidth);
    const float lePx = RangeSliderLogic::valueToPixel(loopEnd_, 0.0, maxValue_,
                                                      trackStart, trackWidth);

    const float dLS = std::abs(mouseX - lsPx);
    const float dPH = std::abs(mouseX - phPx);
    const float dLE = std::abs(mouseX - lePx);

    // Find the closest handle within thumbRadius
    float bestDist = thumbRadius + 1.0f;
    HandleType best = HandleType::None;

    if (dLS <= thumbRadius && dLS < bestDist) {
        bestDist = dLS;
        best = HandleType::LoopStart;
    }
    if (dPH <= thumbRadius && dPH < bestDist) {
        bestDist = dPH;
        best = HandleType::Playhead;
    }
    if (dLE <= thumbRadius && dLE < bestDist) {
        bestDist = dLE;
        best = HandleType::LoopEnd;
    }

    if (best != HandleType::None)
        return best;

    // Check middle zone between loop handles
    if (RangeSliderLogic::isInMiddleZone(mouseX, loopStart_, loopEnd_,
                                         0.0, maxValue_, trackStart, trackWidth,
                                         thumbRadius))
        return HandleType::MiddleZone;

    return HandleType::None;
}

void MultiHandleSliderLogic::dragHandle(HandleType handle, float mouseX,
                                        float trackStart, float trackWidth) {
    if (maxValue_ <= 0.0)
        return;

    const double val = RangeSliderLogic::pixelToValue(mouseX, 0.0, maxValue_, trackStart,
                                                      trackWidth);

    switch (handle) {
    case HandleType::LoopStart:
        loopStart_ = std::clamp(val, 0.0, loopEnd_);
        break;
    case HandleType::Playhead:
        playhead_ = std::clamp(val, 0.0, maxValue_);
        break;
    case HandleType::LoopEnd:
        loopEnd_ = std::clamp(val, loopStart_, maxValue_);
        break;
    default:
        break;
    }
}

void MultiHandleSliderLogic::dragMiddleZone(double initialLoopStart,
                                            double initialLoopEnd,
                                            float deltaPixels,
                                            float trackStart, float trackWidth) {
    auto [newStart, newEnd] = RangeSliderLogic::applyDrag(
        initialLoopStart, initialLoopEnd, 0.0, maxValue_, deltaPixels, trackStart,
        trackWidth);
    loopStart_ = newStart;
    loopEnd_ = newEnd;
}

std::string MultiHandleSliderLogic::formatTime(double seconds) {
    if (seconds < 0.0)
        seconds = 0.0;
    const int totalSec = static_cast<int>(seconds);
    const int mins = totalSec / 60;
    const int secs = totalSec % 60;

    char buf[16];
    std::snprintf(buf, sizeof (buf), "%d:%02d", mins, secs);
    return buf;
}