#include "ParameterLayout.h"
#include "ParameterIds.h"

namespace chordsynth::parameters {

AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    AudioProcessorValueTreeState::ParameterLayout layout;

    // 12 Major Keys: C, C#, D, D#, E, F, F#, G, G#, A, A#, B
    const juce::StringArray keyChoices{
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ids::key, keyParameterVersion},
        names::key,
        keyChoices,
        0 // Default to C (index 0)
    ));

    const juce::StringArray scaleChoices{"Mayor", "Menor natural"};
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ids::scale, scaleParameterVersion},
        names::scale,
        scaleChoices,
        0));

    const juce::StringArray waveformChoices{"Sine", "Saw", "Square", "Triangle"};
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ids::waveform, waveformParameterVersion},
        names::waveform,
        waveformChoices,
        0));

    juce::NormalisableRange<float> cutoffRange{20.0f, 20000.0f};
    cutoffRange.setSkewForCentre(std::sqrt(20.0f * 20000.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ids::cutoff, cutoffParameterVersion},
        names::cutoff,
        cutoffRange,
        8000.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ids::resonance, resonanceParameterVersion},
        names::resonance,
        juce::NormalisableRange<float>{0.1f, 2.0f},
        0.2f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ids::detune, detuneParameterVersion},
        names::detune,
        juce::NormalisableRange<float>{0.0f, 20.0f},
        7.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ids::chorusMix, chorusMixParameterVersion},
        names::chorusMix,
        juce::NormalisableRange<float>{0.0f, 1.0f},
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ids::chorusRate, chorusRateParameterVersion},
        names::chorusRate,
        juce::NormalisableRange<float>{0.1f, 10.0f},
        1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ids::chorusDepth, chorusDepthParameterVersion},
        names::chorusDepth,
        juce::NormalisableRange<float>{0.0f, 1.0f},
        0.25f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ids::delayMix, delayMixParameterVersion},
        names::delayMix,
        juce::NormalisableRange<float>{0.0f, 1.0f},
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ids::delayFeedback, delayFeedbackParameterVersion},
        names::delayFeedback,
        juce::NormalisableRange<float>{0.0f, 0.95f},
        0.3f));

    juce::NormalisableRange<float> delayTimeRange{10.0f, 2000.0f};
    delayTimeRange.setSkewForCentre(std::sqrt(10.0f * 2000.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ids::delayTimeMs, delayTimeMsParameterVersion},
        names::delayTimeMs,
        delayTimeRange,
        250.0f));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ids::delaySync, delaySyncParameterVersion},
        names::delaySync,
        true));

    const juce::StringArray delaySyncRateChoices{"1/4", "1/8", "1/16"};
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ids::delaySyncRate, delaySyncRateParameterVersion},
        names::delaySyncRate,
        delaySyncRateChoices,
        0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ids::reverbMix, reverbMixParameterVersion},
        names::reverbMix,
        juce::NormalisableRange<float>{0.0f, 1.0f},
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ids::reverbRoomSize, reverbRoomSizeParameterVersion},
        names::reverbRoomSize,
        juce::NormalisableRange<float>{0.0f, 1.0f},
        0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ids::reverbDamping, reverbDampingParameterVersion},
        names::reverbDamping,
        juce::NormalisableRange<float>{0.0f, 1.0f},
        0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ids::reverbWidth, reverbWidthParameterVersion},
        names::reverbWidth,
        juce::NormalisableRange<float>{0.0f, 1.0f},
        1.0f));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ids::arpEnabled, arpEnabledParameterVersion},
        names::arpEnabled,
        false));

    const juce::StringArray arpModeChoices{"Up", "Down", "Up/Down", "Random"};
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ids::arpMode, arpModeParameterVersion},
        names::arpMode,
        arpModeChoices,
        0));

    const juce::StringArray arpRateChoices{"1/4", "1/8", "1/16"};
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ids::arpRate, arpRateParameterVersion},
        names::arpRate,
        arpRateChoices,
        1)); // Default 1/8

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ids::arpGate, arpGateParameterVersion},
        names::arpGate,
        juce::NormalisableRange<float>{0.1f, 1.0f},
        0.8f));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ids::performanceMidiEnabled, performanceMidiEnabledParameterVersion},
        names::performanceMidiEnabled,
        false));

    const juce::StringArray transformPaletteChoices{"Basic", "Lo-Fi", "Spice"};
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ids::transformPalette, transformPaletteParameterVersion},
        names::transformPalette,
        transformPaletteChoices,
        1)); // Default Lo-Fi (index 1)

    return layout;
}

} // namespace chordsynth::parameters
