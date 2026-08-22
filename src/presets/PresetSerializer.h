#pragma once

#include "Preset.h"
#include "parameters/ParameterLayout.h"
#include <optional>

namespace chordsynth::presets {

class PresetSerializer {
public:
    static juce::String toJson(const Preset& preset);
    static std::optional<Preset> fromJson(const juce::String& jsonString);

    static Preset fromAPVTS(const parameters::AudioProcessorValueTreeState& apvts, const juce::String& name = "User");
    static Preset fromProcessorState(
        const parameters::AudioProcessorValueTreeState& apvts,
        const state::HarmonyState& harmonyState,
        const juce::String& name = "User");
    static bool applyToAPVTS(const Preset& preset, parameters::AudioProcessorValueTreeState& apvts);
    static bool applyToProcessorState(
        const Preset& preset,
        parameters::AudioProcessorValueTreeState& apvts,
        state::HarmonyState& harmonyState);
};

} // namespace chordsynth::presets
