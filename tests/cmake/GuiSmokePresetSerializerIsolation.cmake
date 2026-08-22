file(READ "${SOURCE_DIR}/tests/CMakeLists.txt" tests_cmake)

string(REGEX MATCH
    "add_executable\\(ChordSynthGuiSmokeTests[^\\)]*\\.\\./src/presets/PresetSerializer\\.cpp"
    production_serializer_source
    "${tests_cmake}")

if(production_serializer_source STREQUAL "")
    message(FATAL_ERROR
        "ChordSynthGuiSmokeTests must compile PresetSerializer.cpp against the production JUCE APVTS type.")
endif()

string(REGEX MATCH
    "target_link_libraries\\(ChordSynthGuiSmokeTests[^\\)]*chordsynth_presets"
    links_headless_presets
    "${tests_cmake}")

if(NOT links_headless_presets STREQUAL "")
    message(FATAL_ERROR
        "ChordSynthGuiSmokeTests must not link the headless chordsynth_presets variant.")
endif()
