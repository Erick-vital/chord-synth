cmake_minimum_required(VERSION 3.22)
project(ChordSynth VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/Dependencies.cmake)

# Si el entorno dispone de dependencias gráficas/audio de JUCE, se agrega el plugin
option(CHORDSYNTH_BUILD_PLUGIN "Build Standalone and VST3 plugin targets" ON)

# Music Theory Pure Library (independiente de JUCE y de hosts)
add_library(chordsynth_music STATIC
    src/music/Chord.h
    src/music/MajorScaleChordMap.h
    src/music/MajorScaleChordMap.cpp
    src/music/NoteMath.h
    src/music/NoteMath.cpp)

target_include_directories(chordsynth_music PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_features(chordsynth_music PUBLIC cxx_std_20)

if(CHORDSYNTH_BUILD_PLUGIN)
    add_subdirectory(external/JUCE)

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
        src/plugin/PluginProcessor.cpp
        src/plugin/PluginProcessor.h
        src/plugin/PluginEditor.cpp
        src/plugin/PluginEditor.h)

    target_include_directories(ChordSynth PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

    target_compile_definitions(ChordSynth PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

    target_link_libraries(ChordSynth
        PRIVATE
            chordsynth_music
            juce::juce_audio_utils
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
