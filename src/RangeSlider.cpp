#include "RangeSlider.h"

RangeSlider::RangeSlider() {
    setSliderStyle(juce::Slider::TwoValueHorizontal);
    setScrollWheelEnabled(false);
}

void RangeSlider::mouseDown(const juce::MouseEvent& e) {
    const float mx = static_cast<float>(e.x);

    isDraggingMiddle = RangeSliderLogic::isInMiddleZone(
        mx,
        getMinValue(), getMaxValue(),
        getRange().getStart(), getRange().getEnd(),
        getTrackStart(), getTrackWidth(),
        kThumbRadius);

    if (isDraggingMiddle) {
        dragStartMouseX = mx;
        dragStartMin = getMinValue();
        dragStartMax = getMaxValue();
    }
    else {
        juce::Slider::mouseDown(e);
    }
}

void RangeSlider::mouseDrag(const juce::MouseEvent& e) {
    if (isDraggingMiddle) {
        const float delta = static_cast<float>(e.x) - dragStartMouseX;
        const auto [newMin, newMax] = RangeSliderLogic::applyDrag(
            dragStartMin, dragStartMax,
            getRange().getStart(), getRange().getEnd(),
            delta, getTrackStart(), getTrackWidth());
        setMinAndMaxValues(newMin, newMax, juce::sendNotificationSync);
    }
    else {
        juce::Slider::mouseDrag(e);
    }
}

void RangeSlider::mouseUp(const juce::MouseEvent& e) {
    if (isDraggingMiddle) {
        isDraggingMiddle = false;
    }
    else {
        juce::Slider::mouseUp(e);
    }
}