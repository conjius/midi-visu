#include <JuceHeader.h>
#include "../src/VoiceManager.h"

class VoiceManagerTests : public UnitTest {
public:
    VoiceManagerTests() : UnitTest("VoiceManager") {
    }

    void runTest() override {
        beginTest("default channel assignments");
        {
            VoiceManager vm;
            for (int v = 0; v < 4; ++v)
                expectEquals(vm.getDrumChannel(v), 10);
            expectEquals(vm.getMelodicChannel(0), 2);
            expectEquals(vm.getMelodicChannel(1), 3);
            expectEquals(vm.getMelodicChannel(2), 4);
        }

        beginTest("setDrumChannel / getDrumChannel");
        {
            VoiceManager vm;
            vm.setDrumChannel(0, 5);
            expectEquals(vm.getDrumChannel(0), 5);
            expectEquals(vm.getDrumChannel(1), 10); // others unchanged
            vm.setDrumChannel(3, 16);
            expectEquals(vm.getDrumChannel(3), 16);
        }

        beginTest("setMelodicChannel / getMelodicChannel");
        {
            VoiceManager vm;
            vm.setMelodicChannel(2, 9);
            expectEquals(vm.getMelodicChannel(2), 9);
            expectEquals(vm.getMelodicChannel(0), 2); // others unchanged
        }

        beginTest("matchDrumVoice - matches");
        {
            const int drumCh[4] = {10, 10, 10, 10};
            expectEquals(VoiceManager::matchDrumVoice(10, 48, drumCh), 0);
            expectEquals(VoiceManager::matchDrumVoice(10, 50, drumCh), 1);
            expectEquals(VoiceManager::matchDrumVoice(10, 52, drumCh), 2);
            expectEquals(VoiceManager::matchDrumVoice(10, 54, drumCh), 3);
        }

        beginTest("matchDrumVoice -non-matches");
        {
            const int drumCh[4] = {10, 10, 10, 10};
            expectEquals(VoiceManager::matchDrumVoice(9, 48, drumCh), -1);
            // wrong channel
            expectEquals(VoiceManager::matchDrumVoice(10, 60, drumCh), -1); // wrong note
            expectEquals(VoiceManager::matchDrumVoice(1, 60, drumCh), -1); // both wrong
        }

        beginTest("matchDrumVoice -mixed channels");
        {
            const int drumCh[4] = {10, 11, 12, 13};
            expectEquals(VoiceManager::matchDrumVoice(10, 48, drumCh), 0);
            expectEquals(VoiceManager::matchDrumVoice(11, 50, drumCh), 1);
            expectEquals(VoiceManager::matchDrumVoice(10, 50, drumCh), -1);
            // ch correct for v0 but wrong note
            expectEquals(VoiceManager::matchDrumVoice(11, 48, drumCh), -1);
            // note correct for v0 but wrong ch
        }

        beginTest("matchMelodicVoice -matches");
        {
            const int melCh[3] = {2, 3, 4};
            expectEquals(VoiceManager::matchMelodicVoice(2, melCh), 0);
            expectEquals(VoiceManager::matchMelodicVoice(3, melCh), 1);
            expectEquals(VoiceManager::matchMelodicVoice(4, melCh), 2);
        }

        beginTest("matchMelodicVoice -non-matches");
        {
            const int melCh[3] = {2, 3, 4};
            expectEquals(VoiceManager::matchMelodicVoice(1, melCh), -1);
            expectEquals(VoiceManager::matchMelodicVoice(5, melCh), -1);
            expectEquals(VoiceManager::matchMelodicVoice(10, melCh), -1);
        }
    }
};

static VoiceManagerTests voiceManagerTests;