cmake_minimum_required(VERSION 3.22)
project(ChordSynth VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/Dependencies.cmake)

# Direct JUCE modules target without requiring system pkg-config on Linux
set(JUCE_MODULES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/modules")

# Processor-only JUCE substitute used by this repository's Linux tests.
# Production GUI/plugin targets must use JUCE's official audio-processors module.
add_library(chordsynth_juce_headless_test_only STATIC
    ${JUCE_MODULES_DIR}/juce_core/juce_core_CompilationTime.cpp
    ${JUCE_MODULES_DIR}/juce_core/juce_core_zlib.c
    ${JUCE_MODULES_DIR}/juce_core/juce_core.cpp
    ${JUCE_MODULES_DIR}/juce_events/juce_events.cpp
    ${JUCE_MODULES_DIR}/juce_audio_basics/juce_audio_basics.cpp
    ${JUCE_MODULES_DIR}/juce_data_structures/juce_data_structures.cpp
    ${JUCE_MODULES_DIR}/juce_audio_formats/juce_audio_formats.cpp
    ${JUCE_MODULES_DIR}/juce_dsp/juce_dsp.cpp
    src/juce/AudioProcessorValueTreeStateHeadless.cpp
    ${JUCE_MODULES_DIR}/juce_audio_processors_headless/juce_audio_processors_headless.cpp)

target_include_directories(chordsynth_juce_headless_test_only PUBLIC
    ${JUCE_MODULES_DIR})

target_compile_definitions(chordsynth_juce_headless_test_only PUBLIC
    CHORDSYNTH_HEADLESS_APVTS_TEST_ONLY=1
    JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
    JUCE_STANDALONE_APPLICATION=0
    DONT_SET_USING_JUCE_NAMESPACE=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_LOAD_CURL_SYMBOLS_LAZILY=1
    JUCE_INCLUDE_ZLIB_CODE=1
    JUCE_USE_FLAC=0
    JUCE_DISABLE_CAUTIOUS_PARAMETER_ID_CHECKING=1
    JUCE_HEADLESS_PLUGIN_CLIENT=1
    JucePlugin_Name="ChordSynth"
    JucePlugin_Desc="Diatonic Chord Synthesizer"
    JucePlugin_Manufacturer="ErickEsc"
    JucePlugin_ManufacturerCode=0x45657363
    JucePlugin_PluginCode=0x43687331
    JucePlugin_IsSynth=1
    JucePlugin_WantsMidiInput=1
    JucePlugin_ProducesMidiOutput=0
    JucePlugin_IsMidiEffect=0
    JucePlugin_EditorRequiresKeyboardFocus=1
    JucePlugin_Version="0.1.0"
    JucePlugin_VersionCode=0x100
    JucePlugin_VersionString="0.1.0")

target_compile_features(chordsynth_juce_headless_test_only PUBLIC cxx_std_20)

# Static production libraries need the same JUCE headers and plugin macros while
# compiling, but must not link the test-only JUCE implementation into a real
# JUCE plugin. The test executable links that implementation explicitly.
add_library(chordsynth_juce_headless_headers INTERFACE)
target_include_directories(chordsynth_juce_headless_headers INTERFACE
    ${JUCE_MODULES_DIR})
target_compile_definitions(chordsynth_juce_headless_headers INTERFACE
    CHORDSYNTH_HEADLESS_APVTS_TEST_ONLY=1
    JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
    JUCE_STANDALONE_APPLICATION=0
    DONT_SET_USING_JUCE_NAMESPACE=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_LOAD_CURL_SYMBOLS_LAZILY=1
    JUCE_INCLUDE_ZLIB_CODE=1
    JUCE_USE_FLAC=0
    JUCE_DISABLE_CAUTIOUS_PARAMETER_ID_CHECKING=1
    JUCE_HEADLESS_PLUGIN_CLIENT=1
    JucePlugin_Name="ChordSynth"
    JucePlugin_Desc="Diatonic Chord Synthesizer"
    JucePlugin_Manufacturer="ErickEsc"
    JucePlugin_ManufacturerCode=0x45657363
    JucePlugin_PluginCode=0x43687331
    JucePlugin_IsSynth=1
    JucePlugin_WantsMidiInput=1
    JucePlugin_ProducesMidiOutput=0
    JucePlugin_IsMidiEffect=0
    JucePlugin_EditorRequiresKeyboardFocus=1
    JucePlugin_Version="0.1.0"
    JucePlugin_VersionCode=0x100
    JucePlugin_VersionString="0.1.0")

# Music Theory Pure Library (independiente de JUCE y de hosts)
add_library(chordsynth_music STATIC
    src/music/Arpeggiator.h
    src/music/Arpeggiator.cpp
    src/music/Chord.h
    src/music/ChordRecipe.h
    src/music/ChordRecipe.cpp
    src/music/ChordVoicingEngine.h
    src/music/ChordVoicingEngine.cpp
    src/music/DiatonicChordVoicer.h
    src/music/DiatonicChordVoicer.cpp
    src/music/HarmonyConfiguration.h
    src/music/HarmonyConfiguration.cpp
    src/music/MajorScaleChordMap.h
    src/music/MajorScaleChordMap.cpp
    src/music/MusicalClock.h
    src/music/NoteMath.h
    src/music/NoteMath.cpp
    src/music/VoiceLeadingResolver.h
    src/music/VoiceLeadingResolver.cpp
    src/music/VoicedChord.h)

target_include_directories(chordsynth_music PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(chordsynth_music PRIVATE
    chordsynth_juce_headless_headers)

target_compile_features(chordsynth_music PUBLIC cxx_std_20)

# Harmony State Library (Persistence and Migration)
add_library(chordsynth_state STATIC
    src/state/HarmonyState.h
    src/state/HarmonyState.cpp)

target_include_directories(chordsynth_state PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(chordsynth_state PUBLIC
    chordsynth_music
    PRIVATE
    chordsynth_juce_headless_headers)

target_compile_features(chordsynth_state PUBLIC cxx_std_20)

# Interaction Library (Message thread controllers)
add_library(chordsynth_interaction STATIC
    src/interaction/ChordPerformanceController.h
    src/interaction/ChordPerformanceController.cpp
    src/interaction/ChordTransform.h
    src/interaction/ChordTransform.cpp
    src/interaction/MidiPerformanceMapper.h
    src/interaction/MidiPerformanceMapper.cpp)

target_include_directories(chordsynth_interaction PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(chordsynth_interaction PUBLIC
    chordsynth_music
    chordsynth_dsp
    PRIVATE
    chordsynth_juce_headless_headers)

target_compile_features(chordsynth_interaction PUBLIC cxx_std_20)

# DSP Library (independiente de UI)
add_library(chordsynth_dsp STATIC
    src/dsp/ChordSound.h
    src/dsp/ChordVoice.h
    src/dsp/ChordVoice.cpp
    src/dsp/Chorus.h
    src/dsp/Chorus.cpp
    src/dsp/TempoDelay.h
    src/dsp/TempoDelay.cpp
    src/dsp/Reverb.h
    src/dsp/Reverb.cpp
    src/dsp/Filter.h
    src/dsp/Filter.cpp
    src/dsp/Oscillator.h
    src/dsp/Oscillator.cpp
    src/dsp/UiMidiQueue.h
    src/dsp/UiMidiQueue.cpp)

target_include_directories(chordsynth_dsp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(chordsynth_dsp PUBLIC
    chordsynth_music
    PRIVATE
    chordsynth_juce_headless_headers)

target_compile_features(chordsynth_dsp PUBLIC cxx_std_20)

# Parameters Library
add_library(chordsynth_parameters STATIC
    src/parameters/ParameterIds.h
    src/parameters/ParameterLayout.h
    src/parameters/ParameterLayout.cpp)

target_include_directories(chordsynth_parameters PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(chordsynth_parameters PRIVATE
    chordsynth_juce_headless_headers)

target_compile_features(chordsynth_parameters PUBLIC cxx_std_20)

# Presets Library
add_library(chordsynth_presets STATIC
    src/presets/Preset.h
    src/presets/PresetSerializer.h
    src/presets/PresetSerializer.cpp)

target_include_directories(chordsynth_presets PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(chordsynth_presets PUBLIC
    chordsynth_parameters
    chordsynth_state
    PRIVATE
    chordsynth_juce_headless_headers)

target_compile_features(chordsynth_presets PUBLIC cxx_std_20)

# Plugin Core Library (Processor & Editor)
add_library(chordsynth_plugin STATIC
    src/plugin/PluginProcessor.h
    src/plugin/PluginProcessor.cpp)

target_include_directories(chordsynth_plugin PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(chordsynth_plugin PUBLIC
    chordsynth_presets
    chordsynth_parameters
    chordsynth_dsp
    chordsynth_music
    chordsynth_interaction
    chordsynth_state
    PRIVATE
    chordsynth_juce_headless_headers)

target_compile_features(chordsynth_plugin PUBLIC cxx_std_20)

# Full JUCE Plugin / Standalone Target (Windows / Hosts)
option(CHORDSYNTH_BUILD_PLUGIN "Build Standalone and VST3 plugin targets" OFF)

# When production static libraries are compiled in Debug, the headless JUCE
# implementation used by ChordSynthTests must use the same JUCE layout macros.
# Keep this scoped to plugin builds so the headless-only preset retains its
# existing, internally consistent configuration.
if(CHORDSYNTH_BUILD_PLUGIN)
    target_compile_definitions(chordsynth_juce_headless_test_only PUBLIC
        "$<$<CONFIG:Debug>:DEBUG=1>"
        "$<$<CONFIG:Debug>:_DEBUG=1>")
endif()

if(CHORDSYNTH_BUILD_PLUGIN)
    add_subdirectory(external/JUCE)

    # These reusable static libraries become part of the VST3 shared object.
    # Match JUCE's configuration macros and compile them as PIC so their object
    # files are ABI-compatible with the generated plugin client.
    foreach(chordsynth_production_static_target IN ITEMS
            chordsynth_dsp
            chordsynth_music
            chordsynth_interaction
            chordsynth_state
            chordsynth_parameters
            chordsynth_presets
            chordsynth_plugin)
        set_target_properties(${chordsynth_production_static_target} PROPERTIES
            POSITION_INDEPENDENT_CODE ON)
        target_compile_definitions(${chordsynth_production_static_target} PRIVATE
            "$<$<CONFIG:Debug>:DEBUG=1>"
            "$<$<CONFIG:Debug>:_DEBUG=1>")
        target_link_libraries(${chordsynth_production_static_target} PRIVATE
            juce::juce_recommended_config_flags)
    endforeach()

    juce_add_plugin(ChordSynth
        COMPANY_NAME "ErickEsc"
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        COPY_PLUGIN_AFTER_BUILD FALSE
        PLUGIN_MANUFACTURER_CODE Eesc
        PLUGIN_CODE Chs1
        FORMATS Standalone VST3
        PRODUCT_NAME "ChordSynth"
        NEEDS_WEB_BROWSER FALSE
        NEEDS_CURL FALSE)

    target_sources(ChordSynth PRIVATE
        src/parameters/ParameterLayout.cpp
        src/parameters/ParameterLayout.h
        src/presets/PresetSerializer.cpp
        src/presets/PresetSerializer.h
        src/plugin/PluginProcessor.cpp
        src/plugin/PluginProcessor.h
        src/plugin/PluginEditor.cpp
        src/plugin/PluginEditor.h
        src/standalone/ChordSynthStandaloneApp.cpp
        src/ui/ChordSynthLookAndFeel.cpp
        src/ui/ChordSynthLookAndFeel.h
        src/ui/ChordKeyComponent.cpp
        src/ui/ChordKeyComponent.h
        src/ui/HeaderBar.cpp
        src/ui/HeaderBar.h
        src/ui/PerformancePanel.cpp
        src/ui/PerformancePanel.h
        src/ui/ChordColorPanel.cpp
        src/ui/ChordColorPanel.h
        src/ui/HarmonyToolbar.cpp
        src/ui/HarmonyToolbar.h
        src/ui/ChordDesignerPanel.cpp
        src/ui/ChordDesignerPanel.h
        src/ui/SoundPanel.cpp
        src/ui/SoundPanel.h)


    target_include_directories(ChordSynth PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

    target_compile_definitions(ChordSynth PUBLIC
        JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

    target_link_libraries(ChordSynth
        PRIVATE
            chordsynth_dsp
            chordsynth_music
            chordsynth_interaction
            chordsynth_state
            juce::juce_audio_utils
            juce::juce_audio_processors
            juce::juce_gui_basics
            juce::juce_gui_extra
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)
endif()

include(CTest)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
