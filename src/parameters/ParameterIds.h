#pragma once

#include <juce_core/juce_core.h>

namespace chordsynth::parameters {

inline constexpr auto stateRootType = "Parameters";
inline constexpr int keyParameterVersion = 1;
inline constexpr int waveformParameterVersion = 1;
inline constexpr int cutoffParameterVersion = 1;
inline constexpr int resonanceParameterVersion = 1;
inline constexpr int detuneParameterVersion = 1;
inline constexpr int chorusMixParameterVersion = 1;
inline constexpr int chorusRateParameterVersion = 1;
inline constexpr int chorusDepthParameterVersion = 1;
inline constexpr int delayMixParameterVersion = 1;
inline constexpr int delayFeedbackParameterVersion = 1;
inline constexpr int delayTimeMsParameterVersion = 1;
inline constexpr int delaySyncParameterVersion = 1;
inline constexpr int delaySyncRateParameterVersion = 1;
inline constexpr int reverbMixParameterVersion = 1;
inline constexpr int reverbRoomSizeParameterVersion = 1;
inline constexpr int reverbDampingParameterVersion = 1;
inline constexpr int reverbWidthParameterVersion = 1;
inline constexpr int arpEnabledParameterVersion = 1;
inline constexpr int arpModeParameterVersion = 1;
inline constexpr int arpRateParameterVersion = 1;
inline constexpr int arpGateParameterVersion = 1;

namespace ids {
    inline constexpr auto key = "key";
    inline constexpr auto waveform = "waveform";
    inline constexpr auto cutoff = "cutoff";
    inline constexpr auto resonance = "resonance";
    inline constexpr auto detune = "detune";
    inline constexpr auto chorusMix = "chorus_mix";
    inline constexpr auto chorusRate = "chorus_rate";
    inline constexpr auto chorusDepth = "chorus_depth";
    inline constexpr auto delayMix = "delay_mix";
    inline constexpr auto delayFeedback = "delay_feedback";
    inline constexpr auto delayTimeMs = "delay_time_ms";
    inline constexpr auto delaySync = "delay_sync";
    inline constexpr auto delaySyncRate = "delay_sync_rate";
    inline constexpr auto reverbMix = "reverb_mix";
    inline constexpr auto reverbRoomSize = "reverb_room_size";
    inline constexpr auto reverbDamping = "reverb_damping";
    inline constexpr auto reverbWidth = "reverb_width";
    inline constexpr auto arpEnabled = "arp_enabled";
    inline constexpr auto arpMode = "arp_mode";
    inline constexpr auto arpRate = "arp_rate";
    inline constexpr auto arpGate = "arp_gate";
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
    inline constexpr auto detune = "Detune";
    inline constexpr auto chorusMix = "Chorus Mix";
    inline constexpr auto chorusRate = "Chorus Rate";
    inline constexpr auto chorusDepth = "Chorus Depth";
    inline constexpr auto delayMix = "Delay Mix";
    inline constexpr auto delayFeedback = "Delay Feedback";
    inline constexpr auto delayTimeMs = "Delay Time";
    inline constexpr auto delaySync = "Delay Sync";
    inline constexpr auto delaySyncRate = "Delay Rate";
    inline constexpr auto reverbMix = "Reverb Mix";
    inline constexpr auto reverbRoomSize = "Reverb Room Size";
    inline constexpr auto reverbDamping = "Reverb Damping";
    inline constexpr auto reverbWidth = "Reverb Width";
    inline constexpr auto arpEnabled = "Arp Enabled";
    inline constexpr auto arpMode = "Arp Mode";
    inline constexpr auto arpRate = "Arp Rate";
    inline constexpr auto arpGate = "Arp Gate";
    inline constexpr auto attack = "Attack";
    inline constexpr auto decay = "Decay";
    inline constexpr auto sustain = "Sustain";
    inline constexpr auto release = "Release";
} // namespace names

} // namespace chordsynth::parameters
