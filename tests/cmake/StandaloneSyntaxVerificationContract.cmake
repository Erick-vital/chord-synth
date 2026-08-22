# Ensure the custom standalone app keeps JUCE's required include order.
# juce_StandaloneFilterWindow.h is an implementation header and does not include
# the public audio/GUI dependencies it uses itself.
set(STANDALONE_SOURCE "${SOURCE_DIR}/src/standalone/ChordSynthStandaloneApp.cpp")
set(JUCE_MODULES_DIR "${SOURCE_DIR}/external/JUCE/modules")

set(STANDALONE_DEFINITIONS
    JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
    JUCE_STANDALONE_APPLICATION=1
    JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_MODULE_AVAILABLE_juce_audio_processors=1
    JUCE_MODULE_AVAILABLE_juce_gui_basics=1
    JUCE_MODULE_AVAILABLE_juce_gui_extra=1
    JUCE_MODULE_AVAILABLE_juce_audio_utils=1
    JucePlugin_Build_Standalone=1
    JucePlugin_Name="ChordSynth"
    JucePlugin_VersionString="0.1.0")

set(COMMON_ARGUMENTS
    /std:c++20
    /Zs
    /I${SOURCE_DIR}/src
    /I${JUCE_MODULES_DIR})

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    foreach(definition IN LISTS STANDALONE_DEFINITIONS)
        list(APPEND COMMON_ARGUMENTS "/D${definition}")
    endforeach()
else()
    set(COMMON_ARGUMENTS
        -std=c++20
        -fsyntax-only
        -I${SOURCE_DIR}/src
        -I${JUCE_MODULES_DIR})
    foreach(definition IN LISTS STANDALONE_DEFINITIONS)
        list(APPEND COMMON_ARGUMENTS "-D${definition}")
    endforeach()
endif()

execute_process(
    COMMAND ${CMAKE_CXX_COMPILER} ${COMMON_ARGUMENTS} "${STANDALONE_SOURCE}"
    RESULT_VARIABLE syntax_result
    OUTPUT_VARIABLE syntax_out
    ERROR_VARIABLE syntax_err)

if(NOT syntax_result EQUAL 0)
    message(FATAL_ERROR
        "Standalone syntax verification failed:\n${syntax_err}\n${syntax_out}")
endif()
