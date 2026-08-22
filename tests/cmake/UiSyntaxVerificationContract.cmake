set(UI_SOURCES
    "${SOURCE_DIR}/src/ui/ChordDesignerPanel.cpp"
    "${SOURCE_DIR}/src/ui/ChordKeyComponent.cpp"
    "${SOURCE_DIR}/src/ui/ChordSynthLookAndFeel.cpp"
    "${SOURCE_DIR}/src/ui/HarmonyToolbar.cpp"
    "${SOURCE_DIR}/src/ui/HeaderBar.cpp"
    "${SOURCE_DIR}/src/ui/PerformancePanel.cpp"
    "${SOURCE_DIR}/src/ui/SoundPanel.cpp"
    "${SOURCE_DIR}/src/plugin/PluginEditor.cpp"
)

foreach(ui_source ${UI_SOURCES})
    get_filename_component(file_name "${ui_source}" NAME)
    message(STATUS "Checking syntax and types: ${file_name}")

    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} /std:c++20 /Zs
                /I${SOURCE_DIR}/src
                /I${SOURCE_DIR}/external/JUCE/modules
                /DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
                /DJUCE_STANDALONE_APPLICATION=0
                /DDONT_SET_USING_JUCE_NAMESPACE=1
                /DJUCE_USE_CURL=0
                /DJUCE_WEB_BROWSER=0
                /DJUCE_MODULE_AVAILABLE_juce_audio_processors=1
                /DJUCE_MODULE_AVAILABLE_juce_gui_basics=1
                /DJUCE_MODULE_AVAILABLE_juce_gui_extra=1
                /DJUCE_MODULE_AVAILABLE_juce_audio_utils=1
                "${ui_source}"
            RESULT_VARIABLE syntax_result
            OUTPUT_VARIABLE syntax_out
            ERROR_VARIABLE syntax_err
        )
    else()
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} -std=c++20 -fsyntax-only
                -I${SOURCE_DIR}/src
                -I${SOURCE_DIR}/external/JUCE/modules
                -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
                -DJUCE_STANDALONE_APPLICATION=0
                -DDONT_SET_USING_JUCE_NAMESPACE=1
                -DJUCE_USE_CURL=0
                -DJUCE_WEB_BROWSER=0
                -DJUCE_MODULE_AVAILABLE_juce_audio_processors=1
                -DJUCE_MODULE_AVAILABLE_juce_gui_basics=1
                -DJUCE_MODULE_AVAILABLE_juce_gui_extra=1
                -DJUCE_MODULE_AVAILABLE_juce_audio_utils=1
                "${ui_source}"
            RESULT_VARIABLE syntax_result
            OUTPUT_VARIABLE syntax_out
            ERROR_VARIABLE syntax_err
        )
    endif()

    if(NOT syntax_result EQUAL 0)
        message(FATAL_ERROR
            "UI syntax verification failed on ${file_name}:\n${syntax_err}\n${syntax_out}")
    endif()
endforeach()
