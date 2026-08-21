#include "PluginEditor.h"

namespace chordsynth {

ChordSynthAudioProcessorEditor::ChordSynthAudioProcessorEditor(ChordSynthAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(400, 300);
}

ChordSynthAudioProcessorEditor::~ChordSynthAudioProcessorEditor()
{
}

void ChordSynthAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e24));
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(20.0f));
    g.drawFittedText("ChordSynth", getLocalBounds(), juce::Justification::centred, 1);
}

void ChordSynthAudioProcessorEditor::resized()
{
}

} // namespace chordsynth
