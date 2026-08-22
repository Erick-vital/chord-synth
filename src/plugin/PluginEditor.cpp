#include "PluginEditor.h"

namespace chordsynth {

ChordSynthAudioProcessorEditor::ChordSynthAudioProcessorEditor(ChordSynthAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      midiOutput(p.getUiMidiQueue()),
      performanceController(
          p.getHarmonyState().getConfiguration(),
          p.getHarmonyState().getVoicer(),
          midiOutput),
      performancePanel(
          performanceController,
          p.getHarmonyState().getConfiguration(),
          p.getHarmonyState().getVoicer())
{
    setLookAndFeel(&lookAndFeel);

    // Initial tonic from key parameter if present
    if (auto* keyParam = p.getAPVTS().getRawParameterValue(parameters::ids::key)) {
        performanceController.setTonic(static_cast<int>(*keyParam));
    }
    performanceController.setScene(p.getHarmonyState().getSelectedScene());
    performanceController.setLiveRevoice(p.getHarmonyState().isLiveRevoiceEnabled());

    addAndMakeVisible(performancePanel);

    setSize(1180, 760);
    setResizable(true, true);
    setResizeLimits(900, 620, 1920, 1200);
}

ChordSynthAudioProcessorEditor::~ChordSynthAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void ChordSynthAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(ui::colors::background);
}

void ChordSynthAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(18);
    // Performance panel occupies the middle performance section (height ~290 px)
    performancePanel.setBounds(bounds.removeFromTop(290));
}

} // namespace chordsynth
