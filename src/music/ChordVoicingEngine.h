#pragma once

#include "ChordRecipe.h"
#include "VoicedChord.h"
#include <array>
#include <cstddef>

namespace chordsynth::music {

struct VoicingCandidateTone {
    int midiNote{0};
    bool active{false};
    bool isRoot{false};
    bool isThird{false};
    bool isFifth{false};
    bool isSeventh{false};
    bool isHighestTension{false};
};

using VoicingCandidateTable = std::array<VoicingCandidateTone, 7>;

class ChordVoicingEngine {
public:
    [[nodiscard]] static NoteSet applyVoicing(
        const VoicingCandidateTable& candidates,
        const ChordRecipe& recipe,
        ChordShape activeShape,
        const VoicingSpec& spec) noexcept;
};

} // namespace chordsynth::music
