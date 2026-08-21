#include "PluginProcessor.h"
#if !JUCE_HEADLESS_PLUGIN_CLIENT
#include "PluginEditor.h"
#endif

namespace chordsynth {

ChordSynthAudioProcessor::ChordSynthAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    synth.addSound(new dsp::ChordSound());
    for (int i = 0; i < numVoices; ++i) {
        synth.addVoice(new dsp::ChordVoice());
    }
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

void ChordSynthAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);
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

void ChordSynthAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
}

bool ChordSynthAudioProcessor::hasEditor() const
{
#if JUCE_HEADLESS_PLUGIN_CLIENT
    return false;
#else
    return true;
#endif
}

juce::AudioProcessorEditor* ChordSynthAudioProcessor::createEditor()
{
#if JUCE_HEADLESS_PLUGIN_CLIENT
    return nullptr;
#else
    return new ChordSynthAudioProcessorEditor(*this);
#endif
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
