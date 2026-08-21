#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "Oscillator.h"

namespace chordsynth::dsp {

class ChordVoice : public juce::SynthesiserVoice {
public:
    ChordVoice() = default;

    bool canPlaySound(juce::SynthesiserSound* sound) override;

    void startNote(
        int midiNoteNumber,
        float velocity,
        juce::SynthesiserSound* sound,
        int currentPitchWheelPosition) override;

    void stopNote(float velocity, bool allowTailOff) override;

    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;

    void setCurrentPlaybackSampleRate(double newRate) override;
    void prepare(double sampleRate) noexcept;
    void setWaveform(Waveform waveform) noexcept;
    void setDetuneCents(float detuneCents) noexcept;

    void renderNextBlock(
        juce::AudioBuffer<float>& outputBuffer,
        int startSample,
        int numSamples) override;

private:
    Oscillator oscA;
    Oscillator oscB;
    juce::ADSR adsr;
    float currentVelocity{0.0f};
    double currentSampleRate{44100.0};
    float currentBaseFrequencyHz{440.0f};
    float currentDetuneCents{7.0f};
};

} // namespace chordsynth::dsp
