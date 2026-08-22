#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

namespace chordsynth::dsp {

// Realtime-safe stereo reverb wrapping juce::dsp::Reverb.
// All state is prepared boundedly; setTargetParameters and process perform zero allocations/locks.
class Reverb final {
public:
    static constexpr float minimumMix = 0.0f;
    static constexpr float maximumMix = 1.0f;
    static constexpr float defaultMix = 0.0f;

    static constexpr float minimumRoomSize = 0.0f;
    static constexpr float maximumRoomSize = 1.0f;
    static constexpr float defaultRoomSize = 0.5f;

    static constexpr float minimumDamping = 0.0f;
    static constexpr float maximumDamping = 1.0f;
    static constexpr float defaultDamping = 0.5f;

    static constexpr float minimumWidth = 0.0f;
    static constexpr float maximumWidth = 1.0f;
    static constexpr float defaultWidth = 1.0f;

    static constexpr double smoothingSeconds = 0.020;

    void prepare(double sampleRate, int maximumBlockSize, int channels,
                 float initialMix = defaultMix,
                 float initialRoomSize = defaultRoomSize,
                 float initialDamping = defaultDamping,
                 float initialWidth = defaultWidth);

    void reset() noexcept;

    void setTargetParameters(float mix, float roomSize, float damping, float width) noexcept;

    void process(juce::AudioBuffer<float>& buffer) noexcept;

    [[nodiscard]] float getCurrentMix() const noexcept { return mixSmoothed.getCurrentValue(); }
    [[nodiscard]] float getTargetMix() const noexcept { return mixSmoothed.getTargetValue(); }
    [[nodiscard]] float getTargetRoomSize() const noexcept { return targetRoomSize; }
    [[nodiscard]] float getTargetDamping() const noexcept { return targetDamping; }
    [[nodiscard]] float getTargetWidth() const noexcept { return targetWidth; }

private:
    [[nodiscard]] static float sanitiseMix(float value) noexcept;
    [[nodiscard]] static float sanitiseRoomSize(float value) noexcept;
    [[nodiscard]] static float sanitiseDamping(float value) noexcept;
    [[nodiscard]] static float sanitiseWidth(float value) noexcept;

    juce::dsp::Reverb reverbDsp;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed;
    float targetRoomSize{defaultRoomSize};
    float targetDamping{defaultDamping};
    float targetWidth{defaultWidth};

    double currentSampleRate{48000.0};
    int preparedChannels{0};
};

} // namespace chordsynth::dsp
