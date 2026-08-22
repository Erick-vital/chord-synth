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
