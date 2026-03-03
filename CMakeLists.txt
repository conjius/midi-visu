cmake_minimum_required(VERSION 3.22)

project(midi-visu VERSION 0.0.12)

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
    PLUGIN_CODE MDVI
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "midi-visu")

target_sources(midi-visu PRIVATE
    src/PluginProcessor.cpp
    src/PluginProcessor.h
    src/PluginEditor.cpp
    src/PluginEditor.h
    src/VideoBackground.mm
        src/VideoBackground.h
        src/VideoListManager.cpp
        src/VideoListManager.h
        src/VoiceManager.cpp
        src/VoiceManager.h
        src/MidiManager.cpp
        src/MidiManager.h
        src/InteractionManager.cpp
        src/InteractionManager.h
        src/UiManager.cpp
        src/UiManager.h
        src/AppConstants.h
        src/RangeSlider.cpp
        src/RangeSlider.h
        src/RangeSliderLogic.cpp
        src/RangeSliderLogic.h
        src/StyleTokens.h
        src/StyleManager.h
        src/StyleManager.cpp
        src/MultiHandleSliderLogic.cpp
        src/MultiHandleSliderLogic.h
        src/SeekBar.cpp
        src/SeekBar.h
        src/OptionsPanelLayout.cpp
        src/OptionsPanelLayout.h
        src/SvgWobbleLogic.cpp
        src/SvgWobbleLogic.h
        src/SvgShapeManager.cpp
        src/SvgShapeManager.h)

# VideoBackground.mm uses AVFoundation ObjC objects stored as members — enable ARC
# so they are properly retained/released without explicit retain/release calls.
set_source_files_properties(src/VideoBackground.mm PROPERTIES
    COMPILE_FLAGS "-fobjc-arc")

target_compile_definitions(midi-visu
    PUBLIC
        # JUCE_STRICT_REFCOUNTEDPOINTER=1 from .jucer
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        PRIVATE
        "MIDI_VISU_ASSETS_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/assets\"")

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
    juce::juce_opengl
    juce::juce_video)

juce_generate_juce_header(midi-visu)

# ── Unit tests ────────────────────────────────────────────────────────────────
enable_testing()

juce_add_console_app(midi-visu-tests
        PRODUCT_NAME "midi-visu Tests")

target_sources(midi-visu-tests PRIVATE
        tests/main.cpp
        tests/VoiceManagerTests.cpp
        tests/MidiManagerTests.cpp
        tests/RangeSliderLogicTests.cpp
        tests/StyleTokensTests.cpp
        tests/VideoListManagerTests.cpp
        tests/MultiHandleSliderLogicTests.cpp
        tests/OptionsPanelLayoutTests.cpp
        tests/SvgWobbleLogicTests.cpp
        src/VoiceManager.cpp
        src/VoiceManager.h
        src/MidiManager.cpp
        src/MidiManager.h
        src/RangeSliderLogic.cpp
        src/RangeSliderLogic.h
        src/StyleTokens.h
        src/VideoListManager.cpp
        src/VideoListManager.h
        src/MultiHandleSliderLogic.cpp
        src/MultiHandleSliderLogic.h
        src/OptionsPanelLayout.cpp
        src/OptionsPanelLayout.h
        src/SvgWobbleLogic.cpp
        src/SvgWobbleLogic.h)

target_include_directories(midi-visu-tests PRIVATE src)

target_compile_definitions(midi-visu-tests
        PUBLIC
        JUCE_STANDALONE_APPLICATION=1
        JUCE_STRICT_REFCOUNTEDPOINTER=1)

target_link_libraries(midi-visu-tests PRIVATE
        juce::juce_audio_basics
        juce::juce_core
        juce::juce_events)

juce_generate_juce_header(midi-visu-tests)

add_test(NAME midi-visu-tests COMMAND midi-visu-tests)
