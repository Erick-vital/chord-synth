#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

namespace chordsynth::dsp {

// Realtime-safe stereo chorus wrapping juce::dsp::Chorus<float>.
// All storage is reserved by prepare(); target parameter updates and processing
// perform no dynamic allocation or locking.
class Chorus final {
public:
    static constexpr float minimumMix = 0.0f;
    static constexpr float maximumMix = 1.0f;
    static constexpr float defaultMix = 0.0f;

    static constexpr float minimumRateHz = 0.1f;
    static constexpr float maximumRateHz = 10.0f;
    static constexpr float defaultRateHz = 1.0f;

    static constexpr float minimumDepth = 0.0f;
    static constexpr float maximumDepth = 1.0f;
    static constexpr float defaultDepth = 0.25f;

    static constexpr double smoothingSeconds = 0.020;

    void prepare(double sampleRate, int maximumBlockSize, int channels,
                 float initialMix = defaultMix,
                 float initialRateHz = defaultRateHz,
                 float initialDepth = defaultDepth);
    void reset() noexcept;
    void setTargetParameters(float mix, float rateHz, float depth) noexcept;
    void process(juce::AudioBuffer<float>& buffer) noexcept;

    [[nodiscard]] float getCurrentMix() const noexcept { return mixSmoothed.getCurrentValue(); }
    [[nodiscard]] float getTargetMix() const noexcept { return mixSmoothed.getTargetValue(); }
    [[nodiscard]] float getTargetRateHz() const noexcept { return targetRateHz; }
    [[nodiscard]] float getTargetDepth() const noexcept { return targetDepth; }

private:
    [[nodiscard]] static float sanitiseMix(float value) noexcept;
    [[nodiscard]] static float sanitiseRate(float value) noexcept;
    [[nodiscard]] static float sanitiseDepth(float value) noexcept;

    juce::dsp::Chorus<float> chorusDsp;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed;
    float targetRateHz{defaultRateHz};
    float targetDepth{defaultDepth};
    double currentSampleRate{48000.0};
    int preparedChannels{0};
};

} // namespace chordsynth::dsp
