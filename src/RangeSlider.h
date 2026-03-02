#pragma once

#include <JuceHeader.h>
#include "RangeSliderLogic.h"

class RangeSlider : public juce::Slider {
public:
    RangeSlider();

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e,
                        const juce::MouseWheelDetails& w) override;

    static constexpr float kThumbRadius = 10.0f;

private:
    float getTrackStart() const { return kThumbRadius; }

    float getTrackWidth() const {
        return static_cast<float>(getWidth()) - 2.0f * kThumbRadius;
    }

    bool isDraggingMiddle = false;
    float dragStartMouseX = 0.0f;
    double dragStartMin = 0.0;
    double dragStartMax = 0.0;
};