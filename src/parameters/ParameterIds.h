#pragma once

#include <juce_core/juce_core.h>

namespace chordsynth::parameters {

inline constexpr auto stateRootType = "Parameters";
inline constexpr int keyParameterVersion = 1;

namespace ids {
    inline constexpr auto key = "key";
    inline constexpr auto attack = "attack";
    inline constexpr auto decay = "decay";
    inline constexpr auto sustain = "sustain";
    inline constexpr auto release = "release";
} // namespace ids

namespace names {
    inline constexpr auto key = "Key";
    inline constexpr auto attack = "Attack";
    inline constexpr auto decay = "Decay";
    inline constexpr auto sustain = "Sustain";
    inline constexpr auto release = "Release";
} // namespace names

} // namespace chordsynth::parameters
