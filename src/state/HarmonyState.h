#pragma once

#include "music/HarmonyConfiguration.h"
#include <juce_data_structures/juce_data_structures.h>
#include <algorithm>

namespace chordsynth::state {

inline constexpr auto stateTag = "HarmonyState";
inline constexpr int currentVersion = 3;

class HarmonyState {
public:
    HarmonyState() noexcept = default;

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
        liveRevoice = false;
        qualityRule = music::QualityRule::diatonic;
        configuration.resetAll();
    }

    [[nodiscard]] juce::ValueTree toValueTree() const;
    bool loadFromValueTree(const juce::ValueTree& vt) noexcept;

    bool operator==(const HarmonyState& other) const noexcept = default;

private:
    bool liveRevoice{false};
    music::QualityRule qualityRule{music::QualityRule::diatonic};
    music::HarmonyConfiguration configuration{};
};

} // namespace chordsynth::state
