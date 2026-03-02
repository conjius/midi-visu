#include <JuceHeader.h>
#include "../src/MultiHandleSliderLogic.h"

class MultiHandleSliderLogicTests : public UnitTest {
public:
    MultiHandleSliderLogicTests() : UnitTest("MultiHandleSliderLogic") {
    }

    void runTest() override {
        beginTest("default state - all values at defaults");
        {
            MultiHandleSliderLogic logic(100.0);
            expectWithinAbsoluteError(logic.loopStart(), 0.0, 1e-9);
            expectWithinAbsoluteError(logic.playhead(), 0.0, 1e-9);
            expectWithinAbsoluteError(logic.loopEnd(), 100.0, 1e-9);
            expectWithinAbsoluteError(logic.maxValue(), 100.0, 1e-9);
        }

        beginTest("default state - zero max");
        {
            MultiHandleSliderLogic logic(0.0);
            expectWithinAbsoluteError(logic.loopStart(), 0.0, 1e-9);
            expectWithinAbsoluteError(logic.loopEnd(), 0.0, 1e-9);
        }

        beginTest("setPlayhead - basic");
        {
            MultiHandleSliderLogic logic(100.0);
            logic.setPlayhead(50.0);
            expectWithinAbsoluteError(logic.playhead(), 50.0, 1e-9);
        }

        beginTest("setPlayhead - clamp below zero");
        {
            MultiHandleSliderLogic logic(100.0);
            logic.setPlayhead(-10.0);
            expectWithinAbsoluteError(logic.playhead(), 0.0, 1e-9);
        }

        beginTest("setPlayhead - clamp above max");
        {
            MultiHandleSliderLogic logic(100.0);
            logic.setPlayhead(200.0);
            expectWithinAbsoluteError(logic.playhead(), 100.0, 1e-9);
        }

        beginTest("setLoopStart - basic");
        {
            MultiHandleSliderLogic logic(100.0);
            logic.setLoopStart(10.0);
            expectWithinAbsoluteError(logic.loopStart(), 10.0, 1e-9);
        }

        beginTest("setLoopStart - clamp below zero");
        {
            MultiHandleSliderLogic logic(100.0);
            logic.setLoopStart(-5.0);
            expectWithinAbsoluteError(logic.loopStart(), 0.0, 1e-9);
        }

        beginTest("setLoopStart - clamp above loopEnd");
        {
            MultiHandleSliderLogic logic(100.0);
            logic.setLoopEnd(60.0);
            logic.setLoopStart(80.0);
            expectWithinAbsoluteError(logic.loopStart(), 60.0, 1e-9);
        }

        beginTest("setLoopEnd - basic");
        {
            MultiHandleSliderLogic logic(100.0);
            logic.setLoopEnd(80.0);
            expectWithinAbsoluteError(logic.loopEnd(), 80.0, 1e-9);
        }

        beginTest("setLoopEnd - clamp below loopStart");
        {
            MultiHandleSliderLogic logic(100.0);
            logic.setLoopStart(30.0);
            logic.setLoopEnd(20.0);
            expectWithinAbsoluteError(logic.loopEnd(), 30.0, 1e-9);
        }

        beginTest("setLoopEnd - clamp above max");
        {
            MultiHandleSliderLogic logic(100.0);
            logic.setLoopEnd(200.0);
            expectWithinAbsoluteError(logic.loopEnd(), 100.0, 1e-9);
        }

        beginTest("setMaxValue - clamps existing values");
        {
            MultiHandleSliderLogic logic(100.0);
            logic.setLoopStart(20.0);
            logic.setPlayhead(80.0);
            logic.setLoopEnd(90.0);
            logic.setMaxValue(50.0);
            expectWithinAbsoluteError(logic.maxValue(), 50.0, 1e-9);
            expectWithinAbsoluteError(logic.loopStart(), 20.0, 1e-9);
            expectWithinAbsoluteError(logic.playhead(), 50.0, 1e-9);
            expectWithinAbsoluteError(logic.loopEnd(), 50.0, 1e-9);
        }

        beginTest("setMaxValue - expands loopEnd from zero");
        {
            MultiHandleSliderLogic logic; // default maxValue=0
            expectWithinAbsoluteError(logic.loopEnd(), 0.0, 1e-9);
            logic.setMaxValue(120.0);
            // loopEnd should expand to the new max
            expectWithinAbsoluteError(logic.loopEnd(), 120.0, 1e-9);
            expectWithinAbsoluteError(logic.loopStart(), 0.0, 1e-9);
        }

        beginTest("setMaxValue - clamps loopStart too");
        {
            MultiHandleSliderLogic logic(100.0);
            logic.setLoopStart(60.0);
            logic.setLoopEnd(80.0);
            logic.setMaxValue(50.0);
            // loopEnd clamps to 50, loopStart clamps to loopEnd=50
            expectWithinAbsoluteError(logic.loopEnd(), 50.0, 1e-9);
            expectWithinAbsoluteError(logic.loopStart(), 50.0, 1e-9);
        }

        // hitTest tests use: trackStart=10, trackWidth=200, range=[0,100], thumbRadius=10
        // loopStart=20 -> px = 10 + 20/100*200 = 50
        // playhead=50  -> px = 10 + 50/100*200 = 110
        // loopEnd=80   -> px = 10 + 80/100*200 = 170

        beginTest("hitTest - closest to loopStart handle");
        {
            MultiHandleSliderLogic logic(100.0);
            logic.setLoopStart(20.0);
            logic.setPlayhead(50.0);
            logic.setLoopEnd(80.0);
            auto result = logic.hitTest(52.0f, 10.0f, 200.0f, 10.0f);
            expect(result == MultiHandleSliderLogic::HandleType::LoopStart);
        }

        beginTest("hitTest - closest to playhead handle");
        {
            MultiHandleSliderLogic logic(100.0);
            logic.setLoopStart(20.0);
            logic.setPlayhead(50.0);
            logic.setLoopEnd(80.0);
            auto result = logic.hitTest(112.0f, 10.0f, 200.0f, 10.0f);
            expect(result == MultiHandleSliderLogic::HandleType::Playhead);
        }

        beginTest("hitTest - closest to loopEnd handle");
        {
            MultiHandleSliderLogic logic(100.0);
            logic.setLoopStart(20.0);
            logic.setPlayhead(50.0);
            logic.setLoopEnd(80.0);
            auto result = logic.hitTest(168.0f, 10.0f, 200.0f, 10.0f);
            expect(result == MultiHandleSliderLogic::HandleType::LoopEnd);
        }

        beginTest("hitTest - middle zone between loop handles");
        {
            MultiHandleSliderLogic logic(100.0);
            logic.setLoopStart(20.0);
            logic.setPlayhead(90.0); // playhead far away
            logic.setLoopEnd(80.0);
            // Middle zone = [50+10, 170-10] = [60, 160]
            // Click at 80 - in middle zone, far from both handles
            auto result = logic.hitTest(80.0f, 10.0f, 200.0f, 10.0f);
            expect(result == MultiHandleSliderLogic::HandleType::MiddleZone);
        }

        beginTest("hitTest - none when outside all handles");
        {
            MultiHandleSliderLogic logic(100.0);
            logic.setLoopStart(40.0);
            logic.setPlayhead(50.0);
            logic.setLoopEnd(60.0);
            // loopStart px=90, playhead px=110, loopEnd px=130
            // Click at 5 - far left, outside all
            auto result = logic.hitTest(5.0f, 10.0f, 200.0f, 10.0f);
            expect(result == MultiHandleSliderLogic::HandleType::None);
        }

        beginTest("dragHandle - move loopStart");
        {
            MultiHandleSliderLogic logic(100.0);
            logic.setLoopStart(20.0);
            logic.setLoopEnd(80.0);
            // Drag to pixel 70 -> value = 0 + (70-10)/200 * 100 = 30
            logic.dragHandle(MultiHandleSliderLogic::HandleType::LoopStart,
                             70.0f, 10.0f, 200.0f);
            expectWithinAbsoluteError(logic.loopStart(), 30.0, 1e-3);
        }

        beginTest("dragHandle - loopStart clamped to loopEnd");
        {
            MultiHandleSliderLogic logic(100.0);
            logic.setLoopStart(20.0);
            logic.setLoopEnd(50.0);
            // Drag to pixel 170 -> value = 80, but loopEnd=50
            logic.dragHandle(MultiHandleSliderLogic::HandleType::LoopStart,
                             170.0f, 10.0f, 200.0f);
            expectWithinAbsoluteError(logic.loopStart(), 50.0, 1e-3);
        }

        beginTest("dragHandle - move playhead");
        {
            MultiHandleSliderLogic logic(100.0);
            logic.setPlayhead(50.0);
            // Drag to pixel 150 -> value = 70
            logic.dragHandle(MultiHandleSliderLogic::HandleType::Playhead,
                             150.0f, 10.0f, 200.0f);
            expectWithinAbsoluteError(logic.playhead(), 70.0, 1e-3);
        }

        beginTest("dragHandle - playhead clamped to range");
        {
            MultiHandleSliderLogic logic(100.0);
            logic.dragHandle(MultiHandleSliderLogic::HandleType::Playhead,
                             -50.0f, 10.0f, 200.0f);
            expectWithinAbsoluteError(logic.playhead(), 0.0, 1e-3);
        }

        beginTest("dragHandle - move loopEnd");
        {
            MultiHandleSliderLogic logic(100.0);
            logic.setLoopStart(20.0);
            logic.setLoopEnd(80.0);
            // Drag to pixel 190 -> value = 90
            logic.dragHandle(MultiHandleSliderLogic::HandleType::LoopEnd,
                             190.0f, 10.0f, 200.0f);
            expectWithinAbsoluteError(logic.loopEnd(), 90.0, 1e-3);
        }

        beginTest("dragHandle - loopEnd clamped to loopStart");
        {
            MultiHandleSliderLogic logic(100.0);
            logic.setLoopStart(40.0);
            logic.setLoopEnd(80.0);
            // Drag to pixel 30 -> value = 10, but loopStart=40
            logic.dragHandle(MultiHandleSliderLogic::HandleType::LoopEnd,
                             30.0f, 10.0f, 200.0f);
            expectWithinAbsoluteError(logic.loopEnd(), 40.0, 1e-3);
        }

        beginTest("dragMiddleZone - move right, no clamping");
        {
            MultiHandleSliderLogic logic(100.0);
            logic.setLoopStart(20.0);
            logic.setLoopEnd(80.0);
            // deltaPixels=20 -> deltaValue=10
            logic.dragMiddleZone(20.0, 80.0, 20.0f, 10.0f, 200.0f);
            expectWithinAbsoluteError(logic.loopStart(), 30.0, 1e-3);
            expectWithinAbsoluteError(logic.loopEnd(), 90.0, 1e-3);
        }

        beginTest("dragMiddleZone - clamp at right boundary");
        {
            MultiHandleSliderLogic logic(100.0);
            logic.setLoopStart(20.0);
            logic.setLoopEnd(80.0);
            // deltaPixels=60 -> deltaValue=30 -> newMax=110 -> clamp
            logic.dragMiddleZone(20.0, 80.0, 60.0f, 10.0f, 200.0f);
            expectWithinAbsoluteError(logic.loopEnd(), 100.0, 1e-3);
            expectWithinAbsoluteError(logic.loopStart(), 40.0, 1e-3);
        }

        beginTest("dragMiddleZone - clamp at left boundary");
        {
            MultiHandleSliderLogic logic(100.0);
            logic.setLoopStart(20.0);
            logic.setLoopEnd(80.0);
            // deltaPixels=-60 -> deltaValue=-30 -> newMin=-10 -> clamp
            logic.dragMiddleZone(20.0, 80.0, -60.0f, 10.0f, 200.0f);
            expectWithinAbsoluteError(logic.loopStart(), 0.0, 1e-3);
            expectWithinAbsoluteError(logic.loopEnd(), 60.0, 1e-3);
        }

        beginTest("formatTime - zero seconds");
        {
            expectEquals(String(MultiHandleSliderLogic::formatTime(0.0)),
                         String("0:00"));
        }

        beginTest("formatTime - 65 seconds");
        {
            expectEquals(String(MultiHandleSliderLogic::formatTime(65.0)),
                         String("1:05"));
        }

        beginTest("formatTime - 3661 seconds");
        {
            expectEquals(String(MultiHandleSliderLogic::formatTime(3661.0)),
                         String("61:01"));
        }

        beginTest("formatTime - fractional seconds rounds down");
        {
            expectEquals(String(MultiHandleSliderLogic::formatTime(59.9)),
                         String("0:59"));
        }
    }
};

static MultiHandleSliderLogicTests multiHandleSliderLogicTests;