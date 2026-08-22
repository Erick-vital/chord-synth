#include "Oscillator.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace chordsynth::dsp {

void Oscillator::prepare(double sampleRate) noexcept {
    if (sampleRate > 0.0) {
        currentSampleRate = sampleRate;
        updatePhaseIncrement();
    }
}

void Oscillator::setFrequency(float frequencyHz) noexcept {
    if (std::isfinite(frequencyHz) && frequencyHz >= 0.0f) {
        currentFrequency = std::min(frequencyHz, static_cast<float>(currentSampleRate * 0.5));
        updatePhaseIncrement();
    }
}

void Oscillator::setWaveform(Waveform waveform) noexcept {
    currentWaveform = waveform;
}

void Oscillator::reset() noexcept {
    phase = 0.0;
}

float Oscillator::processSample() noexcept {
    float output = 0.0f;

    switch (currentWaveform) {
        case Waveform::sine:
            output = static_cast<float>(std::sin(phase * 2.0 * std::numbers::pi));
            break;
        case Waveform::saw:
            // Normalize phase in [0, 1) to [-1, 1)
            output = static_cast<float>(2.0 * phase - 1.0);
            break;
        case Waveform::square:
            output = phase < 0.5 ? 1.0f : -1.0f;
            break;
        case Waveform::triangle:
            // Triangle wave from phase [0, 1)
            if (phase < 0.25) {
                output = static_cast<float>(4.0 * phase);
            } else if (phase < 0.75) {
                output = static_cast<float>(2.0 - 4.0 * phase);
            } else {
                output = static_cast<float>(4.0 * phase - 4.0);
            }
            break;
    }

    // Advance phase and wrap within [0.0, 1.0)
    phase += phaseIncrement;
    if (phase >= 1.0) {
        phase = std::fmod(phase, 1.0);
    } else if (phase < 0.0) {
        phase = 1.0 + std::fmod(phase, 1.0);
        if (phase >= 1.0) phase = 0.0;
    }

    return output;
}

void Oscillator::updatePhaseIncrement() noexcept {
    if (currentSampleRate > 0.0) {
        phaseIncrement = static_cast<double>(currentFrequency) / currentSampleRate;
    } else {
        phaseIncrement = 0.0;
    }
}

} // namespace chordsynth::dsp
