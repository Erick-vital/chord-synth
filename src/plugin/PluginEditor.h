#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "ui/ChordSynthLookAndFeel.h"
#include "ui/HarmonyToolbar.h"
#include "ui/PerformancePanel.h"
#include "ui/ChordDesignerPanel.h"
#include "interaction/ChordPerformanceController.h"
#include "music/DiatonicChordVoicer.h"

namespace chordsynth {

class ChordSynthAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       private juce::Timer {
public:
    explicit ChordSynthAudioProcessorEditor(ChordSynthAudioProcessor& processor);
    ~ChordSynthAudioProcessorEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    ChordSynthAudioProcessor& audioProcessor;
    ui::ChordSynthLookAndFeel lookAndFeel;

    music::DiatonicChordVoicer chordVoicer;
    interaction::QueueMidiBatchOutput midiOutput;
    interaction::ChordPerformanceController performanceController;

    // UI Components
    ui::HarmonyToolbar harmonyToolbar;
    ui::PerformancePanel performancePanel;
    ui::ChordDesignerPanel chordDesignerPanel;

    int lastPolledKeyIndex{0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordSynthAudioProcessorEditor)
};

} // namespace chordsynth
