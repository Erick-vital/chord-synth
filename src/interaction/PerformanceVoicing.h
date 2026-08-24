#pragma once

#include <optional>
#include "interaction/ChordTransform.h"
#include "music/HarmonyConfiguration.h"

namespace chordsynth::interaction {

struct TransformSelection {
    TransformPalette palette{TransformPalette::basic};
    TransformSlot slot{TransformSlot::one};

    constexpr bool operator==(const TransformSelection& other) const noexcept = default;
};

// Pure, shared resolution path for the voicing configuration and a temporary
// color transform. UI and realtime MIDI use the same musical result while
// retaining their own thread-safe note ownership and output mechanisms.
struct PerformanceVoicingContext {
    music::Scale scale{music::Scale::major};
    bool diatonicMode{true};
};

[[nodiscard]] music::VoicingSpec resolvePerformanceVoicingSpec(
    const music::HarmonyConfiguration& config,
    const PerformanceVoicingContext& context,
    int degree,
    std::optional<TransformSelection> transform = std::nullopt) noexcept;

} // namespace chordsynth::interaction
