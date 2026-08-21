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
        oscA.prepare(sampleRate);
        oscB.prepare(sampleRate);
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
    oscA.setWaveform(waveform);
    oscB.setWaveform(waveform);
}

void ChordVoice::setDetuneCents(float detuneCents) noexcept {
    if (!std::isfinite(detuneCents)) {
        currentDetuneCents = 0.0f;
    } else {
        currentDetuneCents = std::clamp(detuneCents, 0.0f, 100.0f);
    }
    if (currentBaseFrequencyHz > 0.0f) {
        float factor = std::pow(2.0f, currentDetuneCents / 1200.0f);
        oscA.setFrequency(currentBaseFrequencyHz / factor);
        oscB.setFrequency(currentBaseFrequencyHz * factor);
    }
}

void ChordVoice::startNote(
    int midiNoteNumber,
    float velocity,
    [[maybe_unused]] juce::SynthesiserSound* sound,
    [[maybe_unused]] int currentPitchWheelPosition) {
    currentVelocity = velocity;
    currentBaseFrequencyHz = music::midiToFrequency(midiNoteNumber);
    float factor = std::pow(2.0f, currentDetuneCents / 1200.0f);
    oscA.setFrequency(currentBaseFrequencyHz / factor);
    oscB.setFrequency(currentBaseFrequencyHz * factor);
    oscA.reset();
    oscB.reset();
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
        float sampleA = oscA.processSample() * envValue * currentVelocity * 0.5f;
        float sampleB = oscB.processSample() * envValue * currentVelocity * 0.5f;

        if (numChannels >= 2) {
            outputBuffer.addSample(0, startSample + sampleIdx, sampleA);
            outputBuffer.addSample(1, startSample + sampleIdx, sampleB);
            for (int channel = 2; channel < numChannels; ++channel) {
                outputBuffer.addSample(channel, startSample + sampleIdx, (sampleA + sampleB) * 0.5f);
            }
        } else if (numChannels == 1) {
            outputBuffer.addSample(0, startSample + sampleIdx, sampleA + sampleB);
        }

        if (!adsr.isActive()) {
            clearCurrentNote();
            break;
        }
    }
}

} // namespace chordsynth::dsp
