#pragma once

#if JUCE_HEADLESS_PLUGIN_CLIENT
#include <juce_audio_processors_headless/juce_audio_processors_headless.h>
#else
#include <juce_audio_processors/juce_audio_processors.h>
#endif

#include "dsp/ChordSound.h"
#include "dsp/ChordVoice.h"
#include "dsp/Filter.h"
#include "dsp/UiMidiQueue.h"
#include "parameters/ParameterLayout.h"
#include <atomic>

namespace chordsynth {

namespace dsp {

// This private synthesiser is populated once with ChordVoice instances in the
// processor constructor, then its voice collection remains immutable.
class ChordSynthesiser final : public juce::Synthesiser {
public:
    void setWaveformForAllVoices(Waveform waveform) noexcept
    {
        for (auto* baseVoice : voices) {
            jassert(dynamic_cast<ChordVoice*>(baseVoice) != nullptr);
            static_cast<ChordVoice*>(baseVoice)->setWaveform(waveform);
        }
    }
};

} // namespace dsp

class ChordSynthAudioProcessor : public juce::AudioProcessor {
public:
    ChordSynthAudioProcessor();
    ~ChordSynthAudioProcessor() override;

    dsp::UiMidiQueue& getUiMidiQueue() noexcept { return uiMidiQueue; }
    parameters::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

private:
    static constexpr int numVoices = 16;
    dsp::ChordSynthesiser synth;
    dsp::Filter globalFilter;
    dsp::UiMidiQueue uiMidiQueue;
    parameters::AudioProcessorValueTreeState apvts;
    std::atomic<float>* waveformParameter{nullptr};
    std::atomic<float>* cutoffParameter{nullptr};
    std::atomic<float>* resonanceParameter{nullptr};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordSynthAudioProcessor)
};

} // namespace chordsynth
