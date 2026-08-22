file(READ "${SOURCE_DIR}/CMakeLists.txt" root_cmake)

foreach(required_ui_source
        "src/ui/PerformancePanel.cpp"
        "src/ui/PerformancePanel.h")
    string(REGEX MATCH
        "target_sources\\(ChordSynth PRIVATE[^\\)]*${required_ui_source}"
        found_source
        "${root_cmake}")
    if(found_source STREQUAL "")
        message(FATAL_ERROR
            "ChordSynth must include ${required_ui_source} in target_sources.")
    endif()
endforeach()
