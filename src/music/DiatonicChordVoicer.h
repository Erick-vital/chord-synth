#pragma once

#include "VoicedChord.h"

namespace chordsynth::music {

class DiatonicChordVoicer {
public:
    [[nodiscard]] RealtimeVoicedChord voiceChordRealtime(
        int tonicPitchClass,
        int degree,
        const VoicingSpec& spec,
        Scale scale = Scale::major) const noexcept;

    [[nodiscard]] VoicedChord voiceChord(
        int tonicPitchClass,
        int degree,
        const VoicingSpec& spec,
        Scale scale = Scale::major) const;
};

} // namespace chordsynth::music
