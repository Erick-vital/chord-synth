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

    return layout;
}

} // namespace chordsynth::parameters
