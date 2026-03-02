#include <JuceHeader.h>
#include "../src/VoiceManager.h"
#include "../src/MidiManager.h"

class MidiManagerTests : public UnitTest {
public:
    MidiManagerTests() : UnitTest("MidiManager") {
    }

    void runTest() override {
        beginTest("MIDI clock counting");
        {
            VoiceManager vm;
            MidiManager mm(vm);
            MidiBuffer buf;
            buf.addEvent(MidiMessage::midiClock(), 0);
            buf.addEvent(MidiMessage::midiClock(), 1);
            buf.addEvent(MidiMessage::midiClock(), 2);
            mm.processBlock(buf);
            expectEquals(mm.midiClockPulse.load(), 3);

            // Clock accumulates across calls.
            mm.processBlock(buf);
            expectEquals(mm.midiClockPulse.load(), 6);
        }

        beginTest("drum voice hit counting");
        {
            VoiceManager vm;
            MidiManager mm(vm);
            MidiBuffer buf;
            // Voice 0: C3 (48) on ch10
            buf.addEvent(MidiMessage::noteOn(10, 48, (uint8)100), 0);
            mm.processBlock(buf);
            expectEquals(mm.drumVoiceHitCount[0].load(), 1);
            expectEquals(mm.ch10RawHitCount.load(), 1);
            expectEquals(mm.ch10RawHitNote.load(), 48);
            // Other voices untouched
            expectEquals(mm.drumVoiceHitCount[1].load(), 0);
        }

        beginTest("all drum voices matched");
        {
            VoiceManager vm;
            MidiManager mm(vm);
            const int notes[4] = {48, 50, 52, 54};
            for (int v = 0; v < 4; ++v) {
                MidiBuffer buf;
                buf.addEvent(MidiMessage::noteOn(10, notes[v], (uint8)100),
                             0);
                mm.processBlock(buf);
                expectEquals(mm.drumVoiceHitCount[v].load(), 1);
            }
            expectEquals(mm.ch10RawHitCount.load(), 4);
        }

        beginTest("melodic note-on tracking");
        {
            VoiceManager vm;
            MidiManager mm(vm);
            // ch2 → track index 0 → internal slot 1
            MidiBuffer buf;
            buf.addEvent(MidiMessage::noteOn(2, 60, (uint8)100), 0);
            mm.processBlock(buf);
            expectEquals(mm.channelHighestNote[1].load(), 60);
            expectEquals(mm.channelNoteOnNote[1].load(), 60);
            expectEquals(mm.channelNoteOnCount[1].load(), 1);
        }

        beginTest("melodic note-off clears highest note");
        {
            VoiceManager vm;
            MidiManager mm(vm);
            MidiBuffer buf;
            buf.addEvent(MidiMessage::noteOn(2, 60, (uint8)100), 0);
            mm.processBlock(buf);
            expectEquals(mm.channelHighestNote[1].load(), 60);

            MidiBuffer buf2;
            buf2.addEvent(MidiMessage::noteOff(2, 60), 0);
            mm.processBlock(buf2);
            expectEquals(mm.channelHighestNote[1].load(), -1);
        }

        beginTest("melodic polyphony -highest note wins");
        {
            VoiceManager vm;
            MidiManager mm(vm);
            MidiBuffer buf;
            buf.addEvent(MidiMessage::noteOn(2, 60, (uint8)100), 0);
            buf.addEvent(MidiMessage::noteOn(2, 72, (uint8)100), 1);
            mm.processBlock(buf);
            expectEquals(mm.channelHighestNote[1].load(), 72);

            // Release the higher note; lower should now be highest.
            MidiBuffer buf2;
            buf2.addEvent(MidiMessage::noteOff(2, 72), 0);
            mm.processBlock(buf2);
            expectEquals(mm.channelHighestNote[1].load(), 60);
        }

        beginTest("unmatched MIDI channel ignored");
        {
            VoiceManager vm; // default: drum ch10, melodic ch2/3/4
            MidiManager mm(vm);
            MidiBuffer buf;
            // ch5 is not assigned to anything
            buf.addEvent(MidiMessage::noteOn(5, 60, (uint8)100), 0);
            mm.processBlock(buf);
            for (int i = 0; i < 4; ++i) {
                expectEquals(mm.channelHighestNote[i].load(), -1);
                expectEquals(mm.drumVoiceHitCount[i].load(), 0);
            }
        }
    }
};

static MidiManagerTests midiManagerTests;