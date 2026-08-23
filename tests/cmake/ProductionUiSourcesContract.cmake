file(READ "${SOURCE_DIR}/CMakeLists.txt" root_cmake)

foreach(required_ui_source
        "src/plugin/PluginEditor.cpp"
        "src/plugin/PluginEditor.h"
        "src/ui/PerformancePanel.cpp"
        "src/ui/PerformancePanel.h"
        "src/ui/ChordColorPanel.cpp"
        "src/ui/ChordColorPanel.h"
        "src/ui/ChordKeyComponent.cpp"
        "src/ui/ChordKeyComponent.h"
        "src/ui/HarmonyToolbar.cpp"
        "src/ui/HarmonyToolbar.h"
        "src/ui/ChordDesignerPanel.cpp"
        "src/ui/ChordDesignerPanel.h"
        "src/ui/SoundPanel.cpp"
        "src/ui/SoundPanel.h")
    string(REGEX MATCH
        "target_sources\\(ChordSynth PRIVATE[^\\)]*${required_ui_source}"
        found_source
        "${root_cmake}")
    if(found_source STREQUAL "")
        message(FATAL_ERROR
            "ChordSynth must include ${required_ui_source} in target_sources.")
    endif()
endforeach()
