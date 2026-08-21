#pragma once

#if ! defined(CHORDSYNTH_HEADLESS_APVTS_TEST_ONLY)
 #error "This APVTS adapter is restricted to the headless test build. Use juce_audio_processors in GUI/plugin builds."
#endif

#if defined(JUCE_AUDIO_PROCESSORS_H_INCLUDED)
 #error "The headless-test APVTS adapter must not be mixed with juce_audio_processors."
#endif

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>
#include <juce_data_structures/juce_data_structures.h>

// Remap APVTS and its optional attachment implementation types to project-unique
// names while parsing the vendored header. This adapter therefore never declares
// or defines JUCE's public APVTS/attachment class names.
namespace juce {
class Slider;
class ComboBox;
class Button;
class ChordSynthHeadlessSliderParameterAttachment;
class ChordSynthHeadlessComboBoxParameterAttachment;
class ChordSynthHeadlessButtonParameterAttachment;
} // namespace juce

#define AudioProcessorValueTreeState ChordSynthHeadlessAudioProcessorValueTreeState
#define SliderParameterAttachment ChordSynthHeadlessSliderParameterAttachment
#define ComboBoxParameterAttachment ChordSynthHeadlessComboBoxParameterAttachment
#define ButtonParameterAttachment ChordSynthHeadlessButtonParameterAttachment
#include <juce_audio_processors/utilities/juce_AudioProcessorValueTreeState.h>
#undef ButtonParameterAttachment
#undef ComboBoxParameterAttachment
#undef SliderParameterAttachment
#undef AudioProcessorValueTreeState
