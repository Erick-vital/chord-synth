file(READ "${SOURCE_DIR}/CMakeLists.txt" root_cmake)

string(REGEX MATCH
    "target_sources\\(ChordSynth PRIVATE[^\\)]*src/presets/PresetSerializer\\.cpp"
    production_serializer_source
    "${root_cmake}")

if(production_serializer_source STREQUAL "")
    message(FATAL_ERROR
        "ChordSynth must compile PresetSerializer.cpp against the production JUCE APVTS type.")
endif()

string(REGEX MATCH
    "target_link_libraries\\(ChordSynth[^\\)]*chordsynth_presets"
    production_links_headless_presets
    "${root_cmake}")

if(NOT production_links_headless_presets STREQUAL "")
    message(FATAL_ERROR
        "ChordSynth must not link the headless chordsynth_presets variant.")
endif()
