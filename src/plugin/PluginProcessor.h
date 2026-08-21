#pragma once

#if JUCE_HEADLESS_PLUGIN_CLIENT
#include <juce_audio_processors_headless/juce_audio_processors_headless.h>
#else
#include <juce_audio_processors/juce_audio_processors.h>
#endif

#include "dsp/ChordSound.h"
#include "dsp/ChordVoice.h"

namespace chordsynth {

class ChordSynthAudioProcessor : public juce::AudioProcessor {
public:
    ChordSynthAudioProcessor();
    ~ChordSynthAudioProcessor() override;

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
    juce::Synthesiser synth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordSynthAudioProcessor)
};

} // namespace chordsynth
