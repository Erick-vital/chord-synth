#pragma once

#include <cmath>

namespace chordsynth::dsp {

enum class Waveform {
    sine,
    saw,
    square,
    triangle
};

// APVTS choice values are represented as floats. Clamp finite values, then
// round half up to the nearest choice; non-finite values use the sine default.
[[nodiscard]] inline Waveform waveformFromRawChoice(float rawChoice) noexcept {
    int choice = 0;
    if (std::isfinite(rawChoice)) {
        if (rawChoice >= 3.0f)
            choice = 3;
        else if (rawChoice > 0.0f)
            choice = static_cast<int>(rawChoice + 0.5f);
    }

    switch (choice) {
        case 1: return Waveform::saw;
        case 2: return Waveform::square;
        case 3: return Waveform::triangle;
        case 0:
        default: return Waveform::sine;
    }
}

class Oscillator {
public:
    Oscillator() noexcept = default;

    void prepare(double sampleRate) noexcept;
    void setFrequency(float frequencyHz) noexcept;
    void setWaveform(Waveform waveform) noexcept;
    void reset() noexcept;

    [[nodiscard]] float processSample() noexcept;

private:
    double currentSampleRate{44100.0};
    float currentFrequency{440.0f};
    Waveform currentWaveform{Waveform::sine};
    double phase{0.0};
    double phaseIncrement{0.0};

    void updatePhaseIncrement() noexcept;
};

} // namespace chordsynth::dsp
