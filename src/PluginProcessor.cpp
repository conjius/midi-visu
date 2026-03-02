/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MidivisuAudioProcessor::MidivisuAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    for (int v = 0; v < 4; ++v) drumVoiceMidiChannel[v].store (10, std::memory_order_relaxed);
    melodicMidiChannel[0].store (2, std::memory_order_relaxed);
    melodicMidiChannel[1].store (3, std::memory_order_relaxed);
    melodicMidiChannel[2].store (4, std::memory_order_relaxed);
}

MidivisuAudioProcessor::~MidivisuAudioProcessor()
{
}

//==============================================================================
const juce::String MidivisuAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MidivisuAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MidivisuAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MidivisuAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MidivisuAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MidivisuAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int MidivisuAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MidivisuAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String MidivisuAudioProcessor::getProgramName (int index)
{
    return {};
}

void MidivisuAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void MidivisuAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void MidivisuAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MidivisuAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

static const int kDrumNotes[4] = { 48, 50, 52, 54 }; // C3, D3, E3, F#3 (JUCE octave numbering)

void MidivisuAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Snapshot channel assignments from UI thread.
    int drumCh[4], melCh[3];
    for (int v = 0; v < 4; ++v) drumCh[v] = drumVoiceMidiChannel[v].load (std::memory_order_relaxed);
    for (int i = 0; i < 3; ++i) melCh[i]  = melodicMidiChannel[i].load  (std::memory_order_relaxed);

    for (const auto meta : midiMessages)
    {
        const auto msg  = meta.getMessage();
        const int  ch   = msg.getChannel();
        const int  note = msg.getNoteNumber();

        // Drum voices — each voice has its own channel + fixed note.
        bool handledAsDrum = false;
        if (msg.isNoteOn())
        {
            for (int v = 0; v < 4; ++v)
            {
                if (ch == drumCh[v] && note == kDrumNotes[v])
                {
                    ch10RawHitNote.store      (note, std::memory_order_relaxed);
                    ch10RawHitCount.fetch_add (1,    std::memory_order_relaxed);
                    drumVoiceHitCount[v].fetch_add (1, std::memory_order_relaxed);
                    handledAsDrum = true;
                    break;
                }
            }
        }
        if (handledAsDrum) continue;

        // Melodic tracks — index 1-3 maps to melodicMidiChannel[0-2].
        for (int i = 0; i < 3; ++i)
        {
            if (ch != melCh[i]) continue;
            const int track = i + 1;
            if (msg.isNoteOn())
            {
                activeNotes[track].insert (note);
                channelNoteOnNote [track].store     (note, std::memory_order_relaxed);
                channelNoteOnCount[track].fetch_add (1,    std::memory_order_relaxed);
            }
            else if (msg.isNoteOff())
                activeNotes[track].erase (note);
            channelHighestNote[track].store (activeNotes[track].empty() ? -1 : *activeNotes[track].rbegin());
        }
    }
}

//==============================================================================
bool MidivisuAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* MidivisuAudioProcessor::createEditor()
{
    return new MidiVisuEditor (*this);
}

//==============================================================================
void MidivisuAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void MidivisuAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidivisuAudioProcessor();
}
