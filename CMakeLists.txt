cmake_minimum_required(VERSION 3.22)
project(ChordSynth VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/Dependencies.cmake)

# Direct JUCE core/audio_basics target without requiring system pkg-config on Linux
set(JUCE_MODULES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/modules")

add_library(juce_audio_basics STATIC
    ${JUCE_MODULES_DIR}/juce_core/juce_core_CompilationTime.cpp
    ${JUCE_MODULES_DIR}/juce_core/juce_core_zlib.c
    ${JUCE_MODULES_DIR}/juce_core/juce_core.cpp
    ${JUCE_MODULES_DIR}/juce_audio_basics/juce_audio_basics.cpp)

target_include_directories(juce_audio_basics PUBLIC
    ${JUCE_MODULES_DIR})

target_compile_definitions(juce_audio_basics PUBLIC
    JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
    JUCE_STANDALONE_APPLICATION=0
    DONT_SET_USING_JUCE_NAMESPACE=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_LOAD_CURL_SYMBOLS_LAZILY=1
    JUCE_INCLUDE_ZLIB_CODE=1
    JUCE_DISABLE_CAUTIOUS_PARAMETER_ID_CHECKING=1)

target_compile_features(juce_audio_basics PUBLIC cxx_std_20)

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

# DSP Library (independiente de UI)
add_library(chordsynth_dsp STATIC
    src/dsp/ChordSound.h
    src/dsp/ChordVoice.h
    src/dsp/ChordVoice.cpp
    src/dsp/Oscillator.h
    src/dsp/Oscillator.cpp)

target_include_directories(chordsynth_dsp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(chordsynth_dsp PUBLIC
    chordsynth_music
    juce_audio_basics)

target_compile_features(chordsynth_dsp PUBLIC cxx_std_20)

include(CTest)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
