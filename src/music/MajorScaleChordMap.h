#pragma once

#include "Chord.h"

namespace chordsynth::music {

class MajorScaleChordMap {
public:
    [[nodiscard]] Chord chordForDegree(
        int tonicPitchClass,
        int baseOctave,
        int degree) const;
};

} // namespace chordsynth::music
