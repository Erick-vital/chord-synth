include(FetchContent)
FetchContent_MakeAvailable(Catch2)

add_executable(ChordSynthTests
    dsp/ChorusTests.cpp
    dsp/ChordVoiceTests.cpp
    dsp/FilterTests.cpp
    dsp/OscillatorTests.cpp
    dsp/RenderSafetyTests.cpp
    dsp/ReverbTests.cpp
    dsp/TempoDelayTests.cpp
    dsp/UiMidiQueueTests.cpp
    music/ArpeggiatorTests.cpp
    music/MajorScaleChordMapTests.cpp
    music/MusicalClockTests.cpp
    music/NoteMathTests.cpp
    parameters/ParameterLayoutTests.cpp
    plugin/ProcessorSmokeTests.cpp
    presets/PresetSerializerTests.cpp)

target_link_libraries(ChordSynthTests PRIVATE
    chordsynth_plugin
    chordsynth_presets
    chordsynth_parameters
    chordsynth_dsp
    chordsynth_music
    chordsynth_juce_headless_test_only
    Catch2::Catch2WithMain)

target_include_directories(ChordSynthTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src)

target_compile_features(ChordSynthTests PRIVATE cxx_std_20)

include(Catch)
catch_discover_tests(ChordSynthTests)

add_test(
    NAME HeadlessApvtsIsolationContract
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/HeadlessApvtsIsolation.cmake)

add_test(
    NAME ProductionParameterLayoutIsolationContract
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ProductionParameterLayoutIsolation.cmake)
