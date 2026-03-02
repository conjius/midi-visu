#pragma once
#include <JuceHeader.h>
#include "MultiHandleSliderLogic.h"
#include "RangeSlider.h"

class SeekBar : public Component {
public:
    SeekBar();

    void paint(Graphics& g) override;
    void paintOverChildren(Graphics& g) override;
    void resized() override;
    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;

    void setMaxValue(double maxSec);
    void setPlayhead(double sec);
    void setLoopStart(double sec);
    void setLoopEnd(double sec);

    double getLoopStart() const;
    double getLoopEnd() const;
    double getPlayhead() const;

    void setLoopEnabled(bool enabled);
    bool isLoopEnabled() const;

    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void seekBarLoopChanged(SeekBar* bar) = 0;
        virtual void seekBarPlayheadDragged(SeekBar* bar) = 0;
    };

    void addListener(Listener* l);
    void removeListener(Listener* l);

    bool isDragging() const;

    static constexpr float kThumbRadius = 8.0f;
    static constexpr int kTopZoneHeight = 8;    // blue drag zone above track
    static constexpr int kTrackAreaHeight = 24;
    static constexpr int kLabelHeight = 16;
    static constexpr int kBarHeight = kTopZoneHeight + kTrackAreaHeight + kLabelHeight;

private:
    float getTrackStart() const;
    float getTrackWidth() const;
    void syncLoopSlider();

    MultiHandleSliderLogic logic;
    bool loopEnabled_ = true;

    // Visual-only JUCE slider that draws the blue loop handle thumbs.
    // Non-interactive — SeekBar handles all mouse events.
    RangeSlider loopSlider_;

    MultiHandleSliderLogic::HandleType activeHandle
        = MultiHandleSliderLogic::HandleType::None;
    float dragStartMouseX = 0.0f;
    double dragStartLoopMin = 0.0;
    double dragStartLoopMax = 0.0;

    ListenerList<Listener> listeners;
};
