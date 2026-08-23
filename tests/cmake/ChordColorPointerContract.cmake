set(PANEL "${SOURCE_DIR}/src/ui/ChordColorPanel.cpp")
if(NOT EXISTS "${PANEL}")
    message(FATAL_ERROR "ChordColorPanel.cpp not found")
endif()

file(READ "${PANEL}" SOURCE)

foreach(REQUIRED
        "btn.onStateChange"
        "juce::Button::buttonDown"
        "pointerButtonsDown"
        "triggerColorPress(i)"
        "triggerColorRelease(i)")
    string(FIND "${SOURCE}" "${REQUIRED}" FOUND)
    if(FOUND EQUAL -1)
        message(FATAL_ERROR "ChordColorPanel momentary pointer contract missing: ${REQUIRED}")
    endif()
endforeach()

string(FIND "${SOURCE}" "btn.addMouseListener(this, false)" LEGACY_LISTENER)
if(NOT LEGACY_LISTENER EQUAL -1)
    message(FATAL_ERROR "Legacy inert mouse listener remains on palette buttons")
endif()
