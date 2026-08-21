#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

namespace chordsynth::dsp {

// Realtime-safe global low-pass. All storage is reserved by prepare(); target
// updates and processing perform no allocation or locking.
class Filter final {
public:
    static constexpr float minimumCutoffHz = 20.0f;
    static constexpr float maximumCutoffHz = 20000.0f;
    static constexpr float defaultCutoffHz = 8000.0f;
    static constexpr float minimumResonance = 0.1f;
    static constexpr float maximumResonance = 2.0f;
    static constexpr float defaultResonance = 0.2f;
    static constexpr double smoothingSeconds = 0.020;

    void prepare(double sampleRate, int maximumBlockSize, int channels,
                 float initialCutoffHz, float initialResonance);
    void reset() noexcept;
    void setTargetParameters(float cutoffHz, float resonance) noexcept;
    void process(juce::AudioBuffer<float>& buffer) noexcept;

    [[nodiscard]] float getCurrentCutoff() const noexcept { return cutoff.getCurrentValue(); }
    [[nodiscard]] float getTargetCutoff() const noexcept { return cutoff.getTargetValue(); }
    [[nodiscard]] float getCurrentResonance() const noexcept { return resonance.getCurrentValue(); }
    [[nodiscard]] float getTargetResonance() const noexcept { return resonance.getTargetValue(); }

private:
    [[nodiscard]] float sanitiseCutoff(float value) const noexcept;
    [[nodiscard]] static float sanitiseResonance(float value) noexcept;

    juce::dsp::StateVariableTPTFilter<float> stateVariableFilter;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> cutoff;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> resonance;
    double currentSampleRate{48000.0};
    int preparedChannels{0};
};

} // namespace chordsynth::dsp