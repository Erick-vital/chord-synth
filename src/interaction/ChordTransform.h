#pragma once

#include <cstdint>
#include <string_view>
#include "music/VoicedChord.h"

namespace chordsynth::interaction {

enum class TransformPalette : std::uint8_t {
    basic = 0,
    loFi = 1,
    spice = 2
};

enum class TransformSlot : std::uint8_t {
    one = 0,
    two = 1,
    three = 2,
    four = 3,
    five = 4,
    six = 5,
    seven = 6,
    eight = 7
};

struct TransformResult {
    music::VoicingSpec spec{};
    std::string_view label{};
};

[[nodiscard]] TransformResult applyChordTransform(
    TransformPalette palette,
    TransformSlot slot,
    const music::VoicingSpec& base,
    music::Scale scale,
    int degree) noexcept;

} // namespace chordsynth::interaction
