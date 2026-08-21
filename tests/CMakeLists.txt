include(FetchContent)
FetchContent_MakeAvailable(Catch2)

add_executable(ChordSynthTests
    dsp/ChordVoiceTests.cpp
    dsp/OscillatorTests.cpp
    dsp/UiMidiQueueTests.cpp
    music/MajorScaleChordMapTests.cpp
    music/NoteMathTests.cpp
    parameters/ParameterLayoutTests.cpp
    plugin/ProcessorSmokeTests.cpp)

target_link_libraries(ChordSynthTests PRIVATE
    chordsynth_plugin
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
