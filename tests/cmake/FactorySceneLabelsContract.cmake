file(READ "${SOURCE_DIR}/src/ui/PerformancePanel.cpp" PANEL)
foreach(REQUIRED
        "1  A \\xc2\\xb7 Diat\\xc3\\xb3nica"
        "2  B \\xc2\\xb7 S\\xc3\\xa9ptimas"
        "3  C \\xc2\\xb7 Lo\\xe2\\x80\\x91"
        "Fi Warm"
        "4  D \\xc2\\xb7 Jazz Tension")
    string(FIND "${PANEL}" "${REQUIRED}" FOUND)
    if(FOUND EQUAL -1)
        message(FATAL_ERROR "Factory scene UI label missing: ${REQUIRED}")
    endif()
endforeach()
