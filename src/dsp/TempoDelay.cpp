#include "TempoDelay.h"
#include <algorithm>
#include <cmath>

namespace chordsynth::dsp {

void TempoDelay::prepare(double sampleRate, int maximumBlockSize, int channels,
                         float initialMix, float initialFeedback, float initialTimeMs,
                         bool initialSync, DelaySyncRate initialSyncRate)
{
    juce::ignoreUnused(maximumBlockSize);
    currentSampleRate = std::isfinite(sampleRate) && sampleRate > 0.0 ? sampleRate : 48000.0;
    preparedChannels = std::max(0, channels);

    // Max delay buffer size: maximumTimeMs (2000 ms = 2.0 sec) or 4 beats at lowest BPM (e.g. 40 BPM -> 6.0 sec max)
    // Preallocate 6.0 seconds to safely accommodate down to 40 BPM 1/1 (or 1/4 at 40 BPM = 1.5s)
    constexpr double maxBufferDurationSeconds = 6.0;
    maxDelaySamples = static_cast<int>(std::ceil(currentSampleRate * maxBufferDurationSeconds)) + 16;

    delayBuffers.assign(static_cast<size_t>(preparedChannels), std::vector<float>(static_cast<size_t>(maxDelaySamples), 0.0f));
    writeIndex = 0;

    mixSmoothed.reset(currentSampleRate, smoothingSeconds);
    const auto safeMix = sanitiseMix(initialMix);
    mixSmoothed.setCurrentAndTargetValue(safeMix);

    targetFeedback = sanitiseFeedback(initialFeedback);
    targetTimeMs = sanitiseTimeMs(initialTimeMs);
    targetTempoSync = initialSync;
    targetSyncRate = initialSyncRate;
}

void TempoDelay::reset() noexcept
{
    for (auto& buf : delayBuffers) {
        std::fill(buf.begin(), buf.end(), 0.0f);
    }
    writeIndex = 0;
}

void TempoDelay::setTargetParameters(float mix, float feedback, float timeMs,
                                     bool tempoSync, DelaySyncRate syncRate) noexcept
{
    mixSmoothed.setTargetValue(sanitiseMix(mix));
    targetFeedback = sanitiseFeedback(feedback);
    targetTimeMs = sanitiseTimeMs(timeMs);
    targetTempoSync = tempoSync;
    targetSyncRate = syncRate;
}

void TempoDelay::process(juce::AudioBuffer<float>& buffer, double bpm) noexcept
{
    const auto channels = std::min(buffer.getNumChannels(), preparedChannels);
    const auto numSamples = buffer.getNumSamples();

    if (channels <= 0 || numSamples <= 0 || maxDelaySamples <= 0)
        return;

    const bool isBypassed = mixSmoothed.getCurrentValue() <= 0.0f && mixSmoothed.getTargetValue() <= 0.0f;
    if (isBypassed) {
        // In bypass, we don't alter the buffer. If we want smooth entry later, we could advance write index
        // or just return immediately for maximum efficiency and exact bypass.
        return;
    }

    const double safeBpm = sanitiseBpm(bpm);
    const int delaySamples = calculateDelaySamples(safeBpm);
    const float feedback = targetFeedback;

    for (int i = 0; i < numSamples; ++i) {
        const float currentMix = mixSmoothed.isSmoothing() ? mixSmoothed.getNextValue() : mixSmoothed.getTargetValue();
        const float dryGain = 1.0f - currentMix;
        const float wetGain = currentMix;

        for (int ch = 0; ch < channels; ++ch) {
            auto& dBuf = delayBuffers[static_cast<size_t>(ch)];
            const float inSample = buffer.getSample(ch, i);

            // Read from circular delay buffer
            int readIndex = writeIndex - delaySamples;
            if (readIndex < 0)
                readIndex += maxDelaySamples;

            const float delayedSample = dBuf[static_cast<size_t>(readIndex)];

            // Output mix before overwriting buffer at writeIndex
            const float outSample = inSample * dryGain + delayedSample * wetGain;
            buffer.setSample(ch, i, outSample);

            // Write input + feedback to circular buffer
            const float newDelayInput = inSample + delayedSample * feedback;
            dBuf[static_cast<size_t>(writeIndex)] = std::isfinite(newDelayInput) ? newDelayInput : 0.0f;
        }

        writeIndex = (writeIndex + 1) % maxDelaySamples;
    }
}

int TempoDelay::calculateDelaySamples(double bpm) const noexcept
{
    double delayTimeSeconds = 0.0;
    if (targetTempoSync) {
        // Beats per second = bpm / 60.0 -> Seconds per beat = 60.0 / bpm
        const double secondsPerBeat = 60.0 / bpm;
        switch (targetSyncRate) {
            case DelaySyncRate::quarter: // 1 beat
                delayTimeSeconds = secondsPerBeat;
                break;
            case DelaySyncRate::eighth: // 1/2 beat
                delayTimeSeconds = secondsPerBeat * 0.5;
                break;
            case DelaySyncRate::sixteenth: // 1/4 beat
                delayTimeSeconds = secondsPerBeat * 0.25;
                break;
        }
    } else {
        delayTimeSeconds = static_cast<double>(targetTimeMs) * 0.001;
    }

    int delaySamples = static_cast<int>(std::round(delayTimeSeconds * currentSampleRate));
    return std::clamp(delaySamples, 1, maxDelaySamples - 1);
}

float TempoDelay::sanitiseMix(float value) noexcept
{
    if (!std::isfinite(value))
        value = defaultMix;
    return std::clamp(value, minimumMix, maximumMix);
}

float TempoDelay::sanitiseFeedback(float value) noexcept
{
    if (!std::isfinite(value))
        value = defaultFeedback;
    return std::clamp(value, minimumFeedback, maximumFeedback);
}

float TempoDelay::sanitiseTimeMs(float value) noexcept
{
    if (!std::isfinite(value))
        value = defaultTimeMs;
    return std::clamp(value, minimumTimeMs, maximumTimeMs);
}

double TempoDelay::sanitiseBpm(double value) noexcept
{
    if (!std::isfinite(value) || value <= 1.0)
        return defaultBpm;
    return std::clamp(value, 20.0, 400.0);
}

} // namespace chordsynth::dsp
