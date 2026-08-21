#include "PluginProcessor.h"
#include "parameters/ParameterIds.h"
#if !JUCE_HEADLESS_PLUGIN_CLIENT
#include "PluginEditor.h"
#endif

namespace chordsynth {

ChordSynthAudioProcessor::ChordSynthAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, parameters::stateRootType, parameters::createParameterLayout())
{
    synth.addSound(new dsp::ChordSound());
    for (int i = 0; i < numVoices; ++i)
        synth.addVoice(new dsp::ChordVoice());
    waveformParameter = apvts.getRawParameterValue(parameters::ids::waveform);
    cutoffParameter = apvts.getRawParameterValue(parameters::ids::cutoff);
    resonanceParameter = apvts.getRawParameterValue(parameters::ids::resonance);
    detuneParameter = apvts.getRawParameterValue(parameters::ids::detune);
    jassert(waveformParameter != nullptr);
    jassert(cutoffParameter != nullptr);
    jassert(resonanceParameter != nullptr);
    jassert(detuneParameter != nullptr);
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

void ChordSynthAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);
    const auto cutoff = cutoffParameter != nullptr
        ? cutoffParameter->load(std::memory_order_relaxed) : dsp::Filter::defaultCutoffHz;
    const auto resonance = resonanceParameter != nullptr
        ? resonanceParameter->load(std::memory_order_relaxed) : dsp::Filter::defaultResonance;
    globalFilter.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels(), cutoff, resonance);
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

    uiMidiQueue.drainTo(midiMessages, 0);

    const auto rawWaveform = waveformParameter != nullptr
        ? waveformParameter->load(std::memory_order_relaxed) : 0.0f;
    synth.setWaveformForAllVoices(dsp::waveformFromRawChoice(rawWaveform));

    const auto rawDetune = detuneParameter != nullptr
        ? detuneParameter->load(std::memory_order_relaxed) : 7.0f;
    synth.setDetuneCentsForAllVoices(rawDetune);

    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    const auto rawCutoff = cutoffParameter != nullptr
        ? cutoffParameter->load(std::memory_order_relaxed) : dsp::Filter::defaultCutoffHz;
    const auto rawResonance = resonanceParameter != nullptr
        ? resonanceParameter->load(std::memory_order_relaxed) : dsp::Filter::defaultResonance;
    globalFilter.setTargetParameters(rawCutoff, rawResonance);
    globalFilter.process(buffer);
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

void ChordSynthAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ChordSynthAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0)
        return;

    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType())) {
        auto incomingState = juce::ValueTree::fromXml(*xmlState);
        const auto addDefaultIfMissing = [&incomingState](const char* id, float value) {
            for (const auto& child : incomingState)
                if (child.getProperty("id").toString() == id)
                    return;
            juce::ValueTree parameterState{"PARAM"};
            parameterState.setProperty("id", id, nullptr);
            parameterState.setProperty("value", value, nullptr);
            incomingState.appendChild(parameterState, nullptr);
        };
        addDefaultIfMissing(parameters::ids::waveform, 0.0f);
        addDefaultIfMissing(parameters::ids::cutoff, 8000.0f);
        addDefaultIfMissing(parameters::ids::resonance, 0.2f);
        addDefaultIfMissing(parameters::ids::detune, 7.0f);

        apvts.replaceState(incomingState);
    }
}

} // namespace chordsynth

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new chordsynth::ChordSynthAudioProcessor();
}
