#pragma once

#include "ParameterIds.h"

#if CHORDSYNTH_HEADLESS_APVTS_TEST_ONLY
 #include "juce/AudioProcessorValueTreeStateHeadless.h"
#else
 #include <juce_audio_processors/juce_audio_processors.h>
#endif

namespace chordsynth::parameters {

#if CHORDSYNTH_HEADLESS_APVTS_TEST_ONLY
using AudioProcessorValueTreeState = juce::ChordSynthHeadlessAudioProcessorValueTreeState;
#else
using AudioProcessorValueTreeState = juce::AudioProcessorValueTreeState;
#endif

AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

} // namespace chordsynth::parameters
