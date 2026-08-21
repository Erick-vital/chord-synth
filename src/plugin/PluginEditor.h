#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

namespace chordsynth {

class ChordSynthAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
    explicit ChordSynthAudioProcessorEditor(ChordSynthAudioProcessor& processor);
    ~ChordSynthAudioProcessorEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    ChordSynthAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordSynthAudioProcessorEditor)
};

} // namespace chordsynth
