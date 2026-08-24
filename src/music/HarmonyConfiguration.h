#pragma once

#include "VoicedChord.h"
#include <array>

namespace chordsynth::music {

class HarmonyConfiguration {
public:
    HarmonyConfiguration() noexcept;

    [[nodiscard]] static constexpr bool isValidDegree(int degreeIndex) noexcept {
        return degreeIndex >= 0 && degreeIndex < 7;
    }

    [[nodiscard]] VoicingSpec getSpec(int degreeIndex) const noexcept;
    bool setSpec(int degreeIndex, const VoicingSpec& spec) noexcept;

    bool resetDegree(int degreeIndex) noexcept;
    void resetAll() noexcept;

    [[nodiscard]] static VoicingSpec defaultSpecForDegree(int degreeIndex) noexcept;

    [[nodiscard]] static HarmonyConfiguration makeDiatonic() noexcept;
    [[nodiscard]] static HarmonyConfiguration makeSevenths() noexcept;
    [[nodiscard]] static HarmonyConfiguration makeLofiWarm() noexcept;
    [[nodiscard]] static HarmonyConfiguration makeJazzTension() noexcept;

    constexpr bool operator==(const HarmonyConfiguration& other) const noexcept = default;

private:
    std::array<VoicingSpec, 7> degrees{};
};

} // namespace chordsynth::music
