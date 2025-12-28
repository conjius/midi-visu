cmake_minimum_required(VERSION 3.22)

project(midi-visu VERSION 1.0.0)

# If you have JUCE installed globally or via a package manager, you can use:
# find_package(JUCE CONFIG REQUIRED)

# If JUCE is in a subfolder or you want to specify its path:
# add_subdirectory(path/to/JUCE JUCE)
# For now, we assume JUCE is available. You might need to set JUCE_ROOT or use add_subdirectory.

# Based on your .jucer file, JUCE was at ../../Downloads/JUCE
if(NOT TARGET juce::juce_audio_processors)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../Downloads/JUCE")
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../Downloads/JUCE" JUCE)
    else()
        message(FATAL_ERROR "JUCE not found. Please set the path to JUCE or ensure it is installed.")
    endif()
endif()

juce_add_plugin(midi-visu
    # VERSION 1.0.0
    # ICON_BIG "icon.png"
    # ICON_SMALL "icon.png"
    COMPANY_NAME "conjius"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_STEP TRUE
    PLUGIN_CODE MDVIS
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "midi-visu")

target_sources(midi-visu PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h)

target_compile_definitions(midi-visu
    PUBLIC
        # JUCE_STRICT_REFCOUNTEDPOINTER=1 from .jucer
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(midi-visu PRIVATE
    juce::juce_animation
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_audio_processors_headless
    juce::juce_audio_utils
    juce::juce_box2d
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_midi_ci
    juce::juce_opengl)

juce_generate_juce_header(midi-visu)
