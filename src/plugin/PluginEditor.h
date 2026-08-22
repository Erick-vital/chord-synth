#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "ui/ChordSynthLookAndFeel.h"
#include "ui/PerformancePanel.h"
#include "interaction/ChordPerformanceController.h"

namespace chordsynth {

class ChordSynthAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
    explicit ChordSynthAudioProcessorEditor(ChordSynthAudioProcessor& processor);
    ~ChordSynthAudioProcessorEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    ChordSynthAudioProcessor& audioProcessor;
    ui::ChordSynthLookAndFeel lookAndFeel;

    interaction::QueueMidiBatchOutput midiOutput;
    interaction::ChordPerformanceController performanceController;

    ui::PerformancePanel performancePanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordSynthAudioProcessorEditor)
};

} // namespace chordsynth
