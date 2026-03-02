#include <JuceHeader.h>
#include "../src/RangeSliderLogic.h"

class RangeSliderLogicTests : public UnitTest {
public:
    RangeSliderLogicTests() : UnitTest("RangeSliderLogic") {
    }

    void runTest() override {
        beginTest("valueToPixel - extremes");
        {
            // trackStart=10, trackWidth=200, range=[0,100]
            expectEquals(RangeSliderLogic::valueToPixel(0.0, 0.0, 100.0, 10.0f, 200.0f),
                         10.0f);
            expectEquals(RangeSliderLogic::valueToPixel(100.0, 0.0, 100.0, 10.0f, 200.0f),
                         210.0f);
            expectEquals(RangeSliderLogic::valueToPixel(50.0, 0.0, 100.0, 10.0f, 200.0f),
                         110.0f);
        }

        beginTest("pixelToValue - round-trip");
        {
            const double val = 37.5;
            const float px = RangeSliderLogic::valueToPixel(
                val, 5.0, 500.0, 10.0f, 260.0f);
            const double back = RangeSliderLogic::pixelToValue(
                px, 5.0, 500.0, 10.0f, 260.0f);
            // Tolerance reflects float precision in the intermediate pixel representation
            expectWithinAbsoluteError(back, val, 1e-3);
        }

        beginTest("isInMiddleZone - inside");
        {
            // minVal=20, maxVal=80, range=[0,100], trackStart=10, trackWidth=200, thumbRadius=10
            // minPx = 10 + 20/100*200 = 50
            // maxPx = 10 + 80/100*200 = 170
            // zone  = [50+10, 170-10] = [60, 160]
            expect(RangeSliderLogic::isInMiddleZone(
                110.0f, 20.0, 80.0, 0.0, 100.0, 10.0f, 200.0f, 10.0f));
        }

        beginTest("isInMiddleZone - near handles");
        {
            // zone = [60, 160]
            // click at 55 - inside fill but within thumbRadius of min handle
            expect(!RangeSliderLogic::isInMiddleZone(
                55.0f, 20.0, 80.0, 0.0, 100.0, 10.0f, 200.0f, 10.0f));
            // click at 165 - inside fill but within thumbRadius of max handle
            expect(!RangeSliderLogic::isInMiddleZone(
                165.0f, 20.0, 80.0, 0.0, 100.0, 10.0f, 200.0f, 10.0f));
        }

        beginTest("isInMiddleZone - outside fill");
        {
            // zone = [60, 160], fill = [50, 170]
            // click to the left of minHandle
            expect(!RangeSliderLogic::isInMiddleZone(
                30.0f, 20.0, 80.0, 0.0, 100.0, 10.0f, 200.0f, 10.0f));
            // click to the right of maxHandle
            expect(!RangeSliderLogic::isInMiddleZone(
                180.0f, 20.0, 80.0, 0.0, 100.0, 10.0f, 200.0f, 10.0f));
        }

        beginTest("isInMiddleZone - tiny interval");
        {
            // minVal=50, maxVal=51 (very close)
            // minPx = 10 + 50/100*200 = 110
            // maxPx = 10 + 51/100*200 = 112
            // zone  = [120, 102] -- collapsed, always false
            expect(!RangeSliderLogic::isInMiddleZone(
                111.0f, 50.0, 51.0, 0.0, 100.0, 10.0f, 200.0f, 10.0f));
        }

        beginTest("applyDrag - move right, no clamping");
        {
            // initialMin=20, initialMax=80, range=[0,100], trackStart=10, trackWidth=200
            // deltaPixels=20 -> deltaValue=10 -> newMin=30, newMax=90
            auto [newMin, newMax] = RangeSliderLogic::applyDrag(
                20.0, 80.0, 0.0, 100.0, 20.0f, 10.0f, 200.0f);
            expectWithinAbsoluteError(newMin, 30.0, 1e-9);
            expectWithinAbsoluteError(newMax, 90.0, 1e-9);
        }

        beginTest("applyDrag - move left, no clamping");
        {
            // deltaPixels=-20 -> deltaValue=-10 -> newMin=10, newMax=70
            auto [newMin, newMax] = RangeSliderLogic::applyDrag(
                20.0, 80.0, 0.0, 100.0, -20.0f, 10.0f, 200.0f);
            expectWithinAbsoluteError(newMin, 10.0, 1e-9);
            expectWithinAbsoluteError(newMax, 70.0, 1e-9);
        }

        beginTest("applyDrag - clamp at right boundary");
        {
            // deltaPixels=60 -> deltaValue=30 -> newMin=50, newMax=110 -> clamp right
            // interval=60, newMax=100, newMin=40
            auto [newMin, newMax] = RangeSliderLogic::applyDrag(
                20.0, 80.0, 0.0, 100.0, 60.0f, 10.0f, 200.0f);
            expectWithinAbsoluteError(newMax, 100.0, 1e-9);
            expectWithinAbsoluteError(newMin, 40.0, 1e-9);
        }

        beginTest("applyDrag - clamp at left boundary");
        {
            // deltaPixels=-60 -> deltaValue=-30 -> newMin=-10, newMax=50 -> clamp left
            // interval=60, newMin=0, newMax=60
            auto [newMin, newMax] = RangeSliderLogic::applyDrag(
                20.0, 80.0, 0.0, 100.0, -60.0f, 10.0f, 200.0f);
            expectWithinAbsoluteError(newMin, 0.0, 1e-9);
            expectWithinAbsoluteError(newMax, 60.0, 1e-9);
        }
    }
};

static RangeSliderLogicTests rangeSliderLogicTests;