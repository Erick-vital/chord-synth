#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace chordsynth::music {

enum class ChordQuality : std::uint8_t {
    major,
    minor,
    diminished
};

struct Chord {
    int degree{};                    // 0..6 (I, ii, iii, IV, V, vi, vii°)
    int rootMidi{};                  // 0..127
    ChordQuality quality{};
    std::array<int, 3> midiNotes{};  // root position triad: [root, third, fifth]
    std::string label;
};

} // namespace chordsynth::music
