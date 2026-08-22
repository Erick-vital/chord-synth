file(READ "${SOURCE_DIR}/tests/CMakeLists.txt" tests_cmake)

foreach(required_definition
        "JucePlugin_Name"
        "JUCE_STANDALONE_APPLICATION=0")
    string(REGEX MATCH
        "target_compile_definitions\\(ChordSynthGuiSmokeTests PRIVATE[^\\)]*${required_definition}"
        found_definition
        "${tests_cmake}")
    if(found_definition STREQUAL "")
        message(FATAL_ERROR
            "ChordSynthGuiSmokeTests must define ${required_definition} because it compiles plugin sources outside juce_add_plugin.")
    endif()
endforeach()
