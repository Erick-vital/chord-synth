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
    static constexpr int denseChordFloor = 48;      // C3
    static constexpr int rootlessOpenFloor = 52;    // E3
    static constexpr int harmonicCeiling = 96;      // C7
    static constexpr int bassMin = 24;              // C1
    static constexpr int bassMax = 47;              // B2

    [[nodiscard]] static int transposeBassToRange(int midiNote, int minNote = bassMin, int maxNote = bassMax) noexcept;

    [[nodiscard]] static NoteSet applyVoicing(
        const VoicingCandidateTable& candidates,
        const ChordRecipe& recipe,
        ChordShape activeShape,
        const VoicingSpec& spec) noexcept;
};

} // namespace chordsynth::music
