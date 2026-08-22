#pragma once

#include "VoicedChord.h"
#include <array>

namespace chordsynth::music {

struct SceneConfiguration {
    std::array<VoicingSpec, 7> degrees{};

    constexpr bool operator==(const SceneConfiguration& other) const noexcept = default;
};

class HarmonyConfiguration {
public:
    HarmonyConfiguration() noexcept;

    [[nodiscard]] static constexpr bool isValidScene(int sceneIndex) noexcept {
        return sceneIndex >= 0 && sceneIndex < 4;
    }

    [[nodiscard]] static constexpr bool isValidDegree(int degreeIndex) noexcept {
        return degreeIndex >= 0 && degreeIndex < 7;
    }

    [[nodiscard]] VoicingSpec getSpec(int sceneIndex, int degreeIndex) const noexcept;
    bool setSpec(int sceneIndex, int degreeIndex, const VoicingSpec& spec) noexcept;

    bool resetDegree(int sceneIndex, int degreeIndex) noexcept;
    void resetAll() noexcept;

    [[nodiscard]] const SceneConfiguration& getScene(int sceneIndex) const noexcept;
    [[nodiscard]] static VoicingSpec defaultSpecForSceneAndDegree(int sceneIndex, int degreeIndex) noexcept;

    constexpr bool operator==(const HarmonyConfiguration& other) const noexcept = default;

private:
    std::array<SceneConfiguration, 4> scenes{};
};

} // namespace chordsynth::music
