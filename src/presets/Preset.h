#pragma once

#include <juce_core/juce_core.h>
#include <string>

namespace chordsynth::presets {

struct PresetParameters {
    int key{0};
    juce::String waveform{"sine"};
    float attackMs{5.0f};
    float decayMs{80.0f};
    float sustain{0.8f};
    float releaseMs{120.0f};
    float cutoffHz{8000.0f};
    float resonance{0.2f};
    float detuneCents{7.0f};
    float masterGainDb{-12.0f};
};

struct Preset {
    int schemaVersion{1};
    juce::String name{"Default"};
    PresetParameters parameters;
};

} // namespace chordsynth::presets
