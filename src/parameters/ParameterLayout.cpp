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

    return layout;
}

} // namespace chordsynth::parameters
