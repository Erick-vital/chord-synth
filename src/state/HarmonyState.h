#pragma once

#include "music/HarmonyConfiguration.h"
#include <juce_data_structures/juce_data_structures.h>
#include <algorithm>

namespace chordsynth::state {

inline constexpr auto stateTag = "HarmonyState";
inline constexpr int currentVersion = 2;

class HarmonyState {
public:
    HarmonyState() noexcept = default;

    [[nodiscard]] int getSelectedScene() const noexcept { return selectedScene; }
    void setSelectedScene(int sceneIndex) noexcept {
        selectedScene = std::clamp(sceneIndex, 0, 3);
    }

    [[nodiscard]] bool getLiveRevoice() const noexcept { return liveRevoice; }
    void setLiveRevoice(bool enabled) noexcept { liveRevoice = enabled; }

    [[nodiscard]] music::QualityRule getQualityRule() const noexcept { return qualityRule; }
    void setQualityRule(music::QualityRule rule) noexcept { qualityRule = rule; }

    [[nodiscard]] const music::HarmonyConfiguration& getConfiguration() const noexcept {
        return configuration;
    }
    [[nodiscard]] music::HarmonyConfiguration& getConfiguration() noexcept {
        return configuration;
    }
    void setConfiguration(const music::HarmonyConfiguration& config) noexcept {
        configuration = config;
    }

    void resetToDefaults() noexcept {
        selectedScene = 0;
        liveRevoice = false;
        qualityRule = music::QualityRule::diatonic;
        configuration.resetAll();
    }

    void resetToLegacyDefaults() noexcept;

    [[nodiscard]] juce::ValueTree toValueTree() const;
    bool loadFromValueTree(const juce::ValueTree& vt) noexcept;

    bool operator==(const HarmonyState& other) const noexcept = default;

private:
    int selectedScene{0}; // 0..3 (A..D)
    bool liveRevoice{false};
    music::QualityRule qualityRule{music::QualityRule::diatonic};
    music::HarmonyConfiguration configuration{};
};

} // namespace chordsynth::state
