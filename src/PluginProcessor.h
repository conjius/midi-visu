/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <set>

//==============================================================================
/**
*/
class MidivisuAudioProcessor  : public AudioProcessor
{
public:
    //==============================================================================
    MidivisuAudioProcessor();
    ~MidivisuAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Highest active MIDI note per channel (indices 1-3 → MIDI ch 2-4), -1 = no notes held
    std::atomic<int> channelHighestNote[4] { -1, -1, -1, -1 };

    // Per-voice hit counter for drums: incremented on each note-on
    std::atomic<int> drumVoiceHitCount[4] { 0, 0, 0, 0 };

    // Raw drum tracking: fires on ANY matched drum note-on, for logging
    std::atomic<int> ch10RawHitCount { 0 };
    std::atomic<int> ch10RawHitNote  { -1 };

    // Note-on event log for melodic tracks (indices 1-3)
    std::atomic<int> channelNoteOnCount[4] { 0, 0, 0, 0 };
    std::atomic<int> channelNoteOnNote [4] { -1, -1, -1, -1 };

    // Channel assignments — written from UI thread, read on audio thread.
    // drumVoiceMidiChannel[0..3]: MIDI channel for each drum voice (default 10).
    // melodicMidiChannel[0..2]:   MIDI channel for melodic tracks 1-3 (default 2,3,4).
    std::atomic<int> drumVoiceMidiChannel[4];
    std::atomic<int> melodicMidiChannel[3];

    // MIDI clock: counts every incoming clock pulse (24 per quarter note).
    std::atomic<int> midiClockPulse { 0 };

private:
    std::set<int> activeNotes[4]; // audio-thread only, indices 1-3 used
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidivisuAudioProcessor)
};
