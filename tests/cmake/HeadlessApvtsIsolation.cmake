file(READ "${SOURCE_DIR}/src/juce/AudioProcessorValueTreeStateHeadless.h" wrapper_header)
file(READ "${SOURCE_DIR}/src/juce/AudioProcessorValueTreeStateHeadless.cpp" wrapper_source)

foreach(required_token
        "CHORDSYNTH_HEADLESS_APVTS_TEST_ONLY"
        "ChordSynthHeadlessAudioProcessorValueTreeState"
        "ChordSynthHeadlessSliderParameterAttachment"
        "ChordSynthHeadlessComboBoxParameterAttachment"
        "ChordSynthHeadlessButtonParameterAttachment")
    string(FIND "${wrapper_header}${wrapper_source}" "${required_token}" token_position)
    if(token_position EQUAL -1)
        message(FATAL_ERROR "Headless APVTS isolation token is missing: ${required_token}")
    endif()
endforeach()

foreach(forbidden_definition
        "class SliderParameterAttachment"
        "class ComboBoxParameterAttachment"
        "class ButtonParameterAttachment")
    string(FIND "${wrapper_source}" "${forbidden_definition}" definition_position)
    if(NOT definition_position EQUAL -1)
        message(FATAL_ERROR "Headless source defines a real JUCE public attachment: ${forbidden_definition}")
    endif()
endforeach()