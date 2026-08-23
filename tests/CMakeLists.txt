include(FetchContent)
FetchContent_MakeAvailable(Catch2)

add_executable(ChordSynthTests
    dsp/ChorusTests.cpp
    dsp/ChordVoiceTests.cpp
    dsp/FilterTests.cpp
    dsp/OscillatorTests.cpp
    dsp/RenderSafetyTests.cpp
    dsp/ReverbTests.cpp
    dsp/TempoDelayTests.cpp
    dsp/UiMidiQueueTests.cpp
    interaction/ChordPerformanceControllerTests.cpp
    interaction/ChordTransformTests.cpp
    interaction/MidiPerformanceMapperTests.cpp
    music/ArpeggiatorTests.cpp
    music/ChordRecipeTests.cpp
    music/DiatonicChordVoicerTests.cpp
    music/HarmonyConfigurationTests.cpp
    music/MajorScaleChordMapTests.cpp
    music/MusicalClockTests.cpp
    music/NoteMathTests.cpp
    music/VoicedChordTests.cpp
    music/VoiceLeadingResolverTests.cpp
    parameters/ParameterLayoutTests.cpp
    plugin/ProcessorSmokeTests.cpp
    presets/PresetSerializerTests.cpp
    state/HarmonyStateTests.cpp)

target_link_libraries(ChordSynthTests PRIVATE
    chordsynth_plugin
    chordsynth_presets
    chordsynth_parameters
    chordsynth_dsp
    chordsynth_music
    chordsynth_interaction
    chordsynth_state
    chordsynth_juce_headless_test_only
    Catch2::Catch2WithMain)

target_include_directories(ChordSynthTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src)

target_compile_features(ChordSynthTests PRIVATE cxx_std_20)

include(Catch)
catch_discover_tests(ChordSynthTests)

add_test(
    NAME HeadlessApvtsIsolationContract
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/HeadlessApvtsIsolation.cmake)

add_test(
    NAME ProductionParameterLayoutIsolationContract
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ProductionParameterLayoutIsolation.cmake)

add_test(
    NAME ProductionPresetSerializerIsolationContract
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ProductionPresetSerializerIsolation.cmake)

add_test(
    NAME PerformancePanelSourceContract
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PerformancePanelSourceContract.cmake)

add_test(
    NAME ProductionUiSourcesContract
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ProductionUiSourcesContract.cmake)

add_test(
    NAME UiSyntaxVerificationContract
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_CXX_COMPILER_ID=${CMAKE_CXX_COMPILER_ID}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/UiSyntaxVerificationContract.cmake)

add_test(
    NAME StandaloneSyntaxVerificationContract
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_CXX_COMPILER_ID=${CMAKE_CXX_COMPILER_ID}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/StandaloneSyntaxVerificationContract.cmake)

add_test(
    NAME GuiSmokePluginDefinitionsContract
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GuiSmokePluginDefinitionsContract.cmake)

add_test(
    NAME GuiSmokePresetSerializerIsolationContract
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GuiSmokePresetSerializerIsolation.cmake)

if(CHORDSYNTH_BUILD_PLUGIN)
    add_executable(ChordSynthGuiSmokeTests
        gui/EditorSmokeTests.cpp
        ../src/parameters/ParameterLayout.cpp
        ../src/presets/PresetSerializer.cpp
        ../src/plugin/PluginProcessor.cpp
        ../src/plugin/PluginEditor.cpp
        ../src/ui/ChordSynthLookAndFeel.cpp
        ../src/ui/ChordKeyComponent.cpp
        ../src/ui/HeaderBar.cpp
        ../src/ui/PerformancePanel.cpp
        ../src/ui/ChordColorPanel.cpp
        ../src/ui/HarmonyToolbar.cpp
        ../src/ui/ChordDesignerPanel.cpp
        ../src/ui/SoundPanel.cpp)

    target_link_libraries(ChordSynthGuiSmokeTests PRIVATE
        chordsynth_dsp
        chordsynth_music
        chordsynth_interaction
        chordsynth_state
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_dsp
        Catch2::Catch2WithMain)

    target_include_directories(ChordSynthGuiSmokeTests PRIVATE
        ${CMAKE_SOURCE_DIR}/src)

    target_compile_features(ChordSynthGuiSmokeTests PRIVATE cxx_std_20)

    # This executable compiles PluginProcessor/PluginEditor directly instead of
    # being created through juce_add_plugin, so provide the plugin macros those
    # production sources receive from JUCE's generated target.
    target_compile_definitions(ChordSynthGuiSmokeTests PRIVATE
        JUCE_STANDALONE_APPLICATION=0
        JucePlugin_Name="ChordSynth")

    catch_discover_tests(ChordSynthGuiSmokeTests)
endif()
