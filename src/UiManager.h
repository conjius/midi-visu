/*
  ==============================================================================
    UiManager — owns all painting logic for the editor.
    Friend of MidiVisuEditor so it can access private state directly.
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class MidiVisuEditor; // forward declaration — full type in UiManager.cpp

class UiManager {
public:
    explicit UiManager(MidiVisuEditor& editor);

    void paint(Graphics& g) const;
    void paintOverChildren(Graphics& g) const;

private:
    MidiVisuEditor& editor;
};