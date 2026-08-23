file(READ "${SOURCE_DIR}/CMakeLists.txt" root_cmake)

string(REGEX MATCH
    "target_sources\\(ChordSynth PRIVATE[^\\)]*src/parameters/ParameterLayout\\.cpp"
    production_layout_source
    "${root_cmake}")

if(production_layout_source STREQUAL "")
    message(FATAL_ERROR
        "ChordSynth must compile ParameterLayout.cpp against official juce_audio_processors; "
        "the headless parameters library returns a different APVTS ParameterLayout type on MSVC.")
endif()

string(REGEX MATCH
    "target_link_libraries\\(ChordSynth[^\\)]*chordsynth_parameters"
    production_links_headless_parameters
    "${root_cmake}")

if(NOT production_links_headless_parameters STREQUAL "")
    message(FATAL_ERROR
        "ChordSynth must not link the headless chordsynth_parameters variant.")
endif()

# JUCE 9's official juce_audio_processors module legitimately depends on its
# shared headless foundation. What production must never link is this project's
# custom test-only implementation target.
string(REGEX MATCH
    "target_link_libraries\\(ChordSynth[^\\)]*chordsynth_juce_headless_test_only"
    production_links_custom_headless_implementation
    "${root_cmake}")

if(NOT production_links_custom_headless_implementation STREQUAL "")
    message(FATAL_ERROR
        "ChordSynth must not link chordsynth_juce_headless_test_only; only ChordSynthTests may link the custom headless implementation.")
endif()
