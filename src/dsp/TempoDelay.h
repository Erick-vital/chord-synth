#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <cmath>
#include <vector>

namespace chordsynth::dsp {

enum class DelaySyncRate {
    quarter = 0, // 1/4 (1 beat)
    eighth = 1,  // 1/8 (0.5 beats)
    sixteenth = 2 // 1/16 (0.25 beats)
};

[[nodiscard]] inline DelaySyncRate delaySyncRateFromRawChoice(float rawChoice) noexcept {
    int choice = 0;
    if (std::isfinite(rawChoice)) {
        if (rawChoice >= 2.0f)
            choice = 2;
        else if (rawChoice > 0.0f)
            choice = static_cast<int>(rawChoice + 0.5f);
    }
    switch (choice) {
        case 1: return DelaySyncRate::eighth;
        case 2: return DelaySyncRate::sixteenth;
        case 0:
        default: return DelaySyncRate::quarter;
    }
}

// Realtime-safe stereo tempo-synced and free delay line.
// Storage is preallocated in prepare() to hold the maximum supported delay (e.g. 2000 ms).
// No heap allocation or locking during process().
class TempoDelay final {
public:
    static constexpr float minimumMix = 0.0f;
    static constexpr float maximumMix = 1.0f;
    static constexpr float defaultMix = 0.0f;

    static constexpr float minimumFeedback = 0.0f;
    static constexpr float maximumFeedback = 0.95f;
    static constexpr float defaultFeedback = 0.3f;

    static constexpr float minimumTimeMs = 10.0f;
    static constexpr float maximumTimeMs = 2000.0f;
    static constexpr float defaultTimeMs = 250.0f;

    static constexpr double defaultBpm = 120.0;
    static constexpr double smoothingSeconds = 0.020;

    void prepare(double sampleRate, int maximumBlockSize, int channels,
                 float initialMix = defaultMix,
                 float initialFeedback = defaultFeedback,
                 float initialTimeMs = defaultTimeMs,
                 bool initialSync = true,
                 DelaySyncRate initialSyncRate = DelaySyncRate::quarter);

    void reset() noexcept;

    void setTargetParameters(float mix, float feedback, float timeMs,
                             bool tempoSync, DelaySyncRate syncRate) noexcept;

    void process(juce::AudioBuffer<float>& buffer, double bpm = defaultBpm) noexcept;

    [[nodiscard]] float getCurrentMix() const noexcept { return mixSmoothed.getCurrentValue(); }
    [[nodiscard]] float getTargetMix() const noexcept { return mixSmoothed.getTargetValue(); }
    [[nodiscard]] float getTargetFeedback() const noexcept { return targetFeedback; }
    [[nodiscard]] float getTargetTimeMs() const noexcept { return targetTimeMs; }
    [[nodiscard]] bool getTargetTempoSync() const noexcept { return targetTempoSync; }
    [[nodiscard]] DelaySyncRate getTargetSyncRate() const noexcept { return targetSyncRate; }

private:
    [[nodiscard]] static float sanitiseMix(float value) noexcept;
    [[nodiscard]] static float sanitiseFeedback(float value) noexcept;
    [[nodiscard]] static float sanitiseTimeMs(float value) noexcept;
    [[nodiscard]] static double sanitiseBpm(double value) noexcept;

    [[nodiscard]] int calculateDelaySamples(double bpm) const noexcept;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed;
    float targetFeedback{defaultFeedback};
    float targetTimeMs{defaultTimeMs};
    bool targetTempoSync{true};
    DelaySyncRate targetSyncRate{DelaySyncRate::quarter};

    double currentSampleRate{48000.0};
    int preparedChannels{0};

    // Circular delay buffer per channel
    std::vector<std::vector<float>> delayBuffers;
    int writeIndex{0};
    int maxDelaySamples{0};
};

} // namespace chordsynth::dsp
