# PerformancePanel and ChordDesignerPanel must not define or use runtime scene switches/labels/buttons/feedback.

file(READ "${SOURCE_DIR}/src/ui/PerformancePanel.h" PERF_H)
file(READ "${SOURCE_DIR}/src/ui/PerformancePanel.cpp" PERF_CPP)
file(READ "${SOURCE_DIR}/src/ui/ChordDesignerPanel.h" DESIGNER_H)
file(READ "${SOURCE_DIR}/src/ui/ChordDesignerPanel.cpp" DESIGNER_CPP)

set(COMBINED_SOURCES "${PERF_H}\n${PERF_CPP}\n${DESIGNER_H}\n${DESIGNER_CPP}")

foreach(FORBIDDEN_TOKEN
        "sceneButtons"
        "sceneLabels"
        "selectScene"
        "setSelectedScene"
        "onSceneSelected"
        "handleSceneButtonClicked"
        "selectedScene"
        "selected_scene"
        "escena"
        "Escena")
    string(FIND "${COMBINED_SOURCES}" "${FORBIDDEN_TOKEN}" FOUND_POS)
    if(NOT FOUND_POS EQUAL -1)
        message(FATAL_ERROR "Forbidden scene-based token found in UI sources: ${FORBIDDEN_TOKEN}")
    endif()
endforeach()
