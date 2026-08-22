#pragma once

#include "VoicedChord.h"

namespace chordsynth::music {

class DiatonicChordVoicer {
public:
    [[nodiscard]] VoicedChord voiceChord(
        int tonicPitchClass,
        int degree,
        const VoicingSpec& spec) const;
};

} // namespace chordsynth::music
