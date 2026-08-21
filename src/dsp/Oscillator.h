#pragma once

namespace chordsynth::dsp {

enum class Waveform {
    sine,
    saw,
    square,
    triangle
};

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
