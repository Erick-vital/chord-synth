include(FetchContent)
FetchContent_MakeAvailable(Catch2)

add_executable(ChordSynthTests
    dsp/ChordVoiceTests.cpp
    dsp/OscillatorTests.cpp
    music/MajorScaleChordMapTests.cpp
    music/NoteMathTests.cpp)

target_link_libraries(ChordSynthTests PRIVATE
    chordsynth_music
    chordsynth_dsp
    Catch2::Catch2WithMain)

target_include_directories(ChordSynthTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src)

target_compile_features(ChordSynthTests PRIVATE cxx_std_20)

include(Catch)
catch_discover_tests(ChordSynthTests)
