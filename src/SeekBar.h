#pragma once
#include <JuceHeader.h>
#include "MultiHandleSliderLogic.h"

class SeekBar : public Component {
public:
    SeekBar();

    void paint(Graphics& g) override;
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
    static constexpr int kBarHeight = 24;

private:
    float getTrackStart() const;
    float getTrackWidth() const;

    MultiHandleSliderLogic logic;

    MultiHandleSliderLogic::HandleType activeHandle
        = MultiHandleSliderLogic::HandleType::None;
    float dragStartMouseX = 0.0f;
    double dragStartLoopMin = 0.0;
    double dragStartLoopMax = 0.0;

    ListenerList<Listener> listeners;
};
