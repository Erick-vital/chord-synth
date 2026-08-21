#include "ChordVoice.h"
#include "ChordSound.h"
#include "music/NoteMath.h"

namespace chordsynth::dsp {

bool ChordVoice::canPlaySound(juce::SynthesiserSound* sound) {
    return dynamic_cast<ChordSound*>(sound) != nullptr;
}

void ChordVoice::setCurrentPlaybackSampleRate(double newRate) {
    juce::SynthesiserVoice::setCurrentPlaybackSampleRate(newRate);
    prepare(newRate);
}

void ChordVoice::prepare(double sampleRate) noexcept {
    if (sampleRate > 0.0) {
        currentSampleRate = sampleRate;
        osc.prepare(sampleRate);
        adsr.setSampleRate(sampleRate);

        // Initial default parameters: Attack 5ms, Decay 80ms, Sustain 0.8, Release 120ms
        juce::ADSR::Parameters params;
        params.attack = 0.005f;
        params.decay = 0.080f;
        params.sustain = 0.8f;
        params.release = 0.120f;
        adsr.setParameters(params);
    }
}

void ChordVoice::setWaveform(Waveform waveform) noexcept {
    osc.setWaveform(waveform);
}

void ChordVoice::startNote(
    int midiNoteNumber,
    float velocity,
    [[maybe_unused]] juce::SynthesiserSound* sound,
    [[maybe_unused]] int currentPitchWheelPosition) {
    currentVelocity = velocity;
    float freqHz = music::midiToFrequency(midiNoteNumber);
    osc.setFrequency(freqHz);
    osc.reset();
    adsr.noteOn();
}

void ChordVoice::stopNote([[maybe_unused]] float velocity, bool allowTailOff) {
    if (allowTailOff) {
        adsr.noteOff();
    } else {
        adsr.reset();
        clearCurrentNote();
    }
}

void ChordVoice::pitchWheelMoved([[maybe_unused]] int newPitchWheelValue) {}

void ChordVoice::controllerMoved([[maybe_unused]] int controllerNumber, [[maybe_unused]] int newControllerValue) {}

void ChordVoice::renderNextBlock(
    juce::AudioBuffer<float>& outputBuffer,
    int startSample,
    int numSamples) {
    if (!adsr.isActive()) {
        clearCurrentNote();
        return;
    }

    int numChannels = outputBuffer.getNumChannels();

    for (int sampleIdx = 0; sampleIdx < numSamples; ++sampleIdx) {
        float envValue = adsr.getNextSample();
        float oscValue = osc.processSample();
        float sampleVal = oscValue * envValue * currentVelocity;

        for (int channel = 0; channel < numChannels; ++channel) {
            outputBuffer.addSample(channel, startSample + sampleIdx, sampleVal);
        }

        if (!adsr.isActive()) {
            clearCurrentNote();
            break;
        }
    }
}

} // namespace chordsynth::dsp
