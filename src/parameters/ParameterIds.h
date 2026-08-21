#pragma once

#include <juce_core/juce_core.h>

namespace chordsynth::parameters {

inline constexpr auto stateRootType = "Parameters";
inline constexpr int keyParameterVersion = 1;
inline constexpr int waveformParameterVersion = 1;
inline constexpr int cutoffParameterVersion = 1;
inline constexpr int resonanceParameterVersion = 1;

namespace ids {
    inline constexpr auto key = "key";
    inline constexpr auto waveform = "waveform";
    inline constexpr auto cutoff = "cutoff";
    inline constexpr auto resonance = "resonance";
    inline constexpr auto attack = "attack";
    inline constexpr auto decay = "decay";
    inline constexpr auto sustain = "sustain";
    inline constexpr auto release = "release";
} // namespace ids

namespace names {
    inline constexpr auto key = "Key";
    inline constexpr auto waveform = "Waveform";
    inline constexpr auto cutoff = "Cutoff";
    inline constexpr auto resonance = "Resonance";
    inline constexpr auto attack = "Attack";
    inline constexpr auto decay = "Decay";
    inline constexpr auto sustain = "Sustain";
    inline constexpr auto release = "Release";
} // namespace names

} // namespace chordsynth::parameters
