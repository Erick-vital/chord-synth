#pragma once

#include "state/HarmonyState.h"
#include <juce_core/juce_core.h>
#include <string>

namespace chordsynth::presets {

struct PresetParameters {
    int key{0};
    int scale{0}; // 0 = Major, 1 = Natural minor
    juce::String waveform{"sine"};
    float attackMs{5.0f};
    float decayMs{80.0f};
    float sustain{0.8f};
    float releaseMs{120.0f};
    float cutoffHz{8000.0f};
    float resonance{0.2f};
    float detuneCents{7.0f};
    float chorusMix{0.0f};
    float chorusRateHz{1.0f};
    float chorusDepth{0.25f};
    float delayMix{0.0f};
    float delayFeedback{0.3f};
    float delayTimeMs{250.0f};
    bool delaySync{true};
    int delaySyncRate{0}; // 0 = 1/4, 1 = 1/8, 2 = 1/16
    float reverbMix{0.0f};
    float reverbRoomSize{0.5f};
    float reverbDamping{0.5f};
    float reverbWidth{1.0f};
    bool arpEnabled{false};
    int arpMode{0}; // 0 = Up, 1 = Down, 2 = Up/Down, 3 = Random
    int arpRate{1}; // 0 = 1/4, 1 = 1/8, 2 = 1/16
    float arpGate{0.8f};
    float masterGainDb{-12.0f};
};

struct Preset {
    int schemaVersion{3};
    juce::String name{"Default"};
    PresetParameters parameters;
    state::HarmonyState harmony;
};

} // namespace chordsynth::presets
