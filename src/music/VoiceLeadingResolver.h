#pragma once

#include "ChordVoicingEngine.h"
#include "VoicedChord.h"

namespace chordsynth::music {

class VoiceLeadingResolver {
public:
    static constexpr int safeMinMidi = ChordVoicingEngine::denseChordFloor; // 48 (C3)
    static constexpr int safeMaxMidi = ChordVoicingEngine::harmonicCeiling;  // 96 (C7)

    [[nodiscard]] static NoteSet resolveNearestVoiceLeading(
        const NoteSet& previousNotes,
        const NoteSet& targetDefaultNotes) noexcept;
};

} // namespace chordsynth::music
