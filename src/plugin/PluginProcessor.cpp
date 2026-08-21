#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace chordsynth {

ChordSynthAudioProcessor::ChordSynthAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

ChordSynthAudioProcessor::~ChordSynthAudioProcessor()
{
}

const juce::String ChordSynthAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ChordSynthAudioProcessor::acceptsMidi() const
{
    return true;
}

bool ChordSynthAudioProcessor::producesMidi() const
{
    return false;
}

bool ChordSynthAudioProcessor::isMidiEffect() const
{
    return false;
}

double ChordSynthAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ChordSynthAudioProcessor::getNumPrograms()
{
    return 1;
}

int ChordSynthAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ChordSynthAudioProcessor::setCurrentProgram(int)
{
}

const juce::String ChordSynthAudioProcessor::getProgramName(int)
{
    return {};
}

void ChordSynthAudioProcessor::changeProgramName(int, const juce::String&)
{
}

void ChordSynthAudioProcessor::prepareToPlay(double, int)
{
}

void ChordSynthAudioProcessor::releaseResources()
{
}

bool ChordSynthAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void ChordSynthAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
}

bool ChordSynthAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* ChordSynthAudioProcessor::createEditor()
{
    return new ChordSynthAudioProcessorEditor(*this);
}

void ChordSynthAudioProcessor::getStateInformation(juce::MemoryBlock&)
{
}

void ChordSynthAudioProcessor::setStateInformation(const void*, int)
{
}

} // namespace chordsynth

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new chordsynth::ChordSynthAudioProcessor();
}
