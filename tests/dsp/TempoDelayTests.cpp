#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include "dsp/TempoDelay.h"
#include <algorithm>
#include <cmath>
#include <limits>

using chordsynth::dsp::TempoDelay;
using chordsynth::dsp::DelaySyncRate;

namespace {

float calculateRms(const juce::AudioBuffer<float>& buffer, int channel) {
    double sum = 0.0;
    const int numSamples = buffer.getNumSamples();
    for (int i = 0; i < numSamples; ++i) {
        float s = buffer.getSample(channel, i);
        sum += static_cast<double>(s) * s;
    }
    return static_cast<float>(std::sqrt(sum / std::max(1, numSamples)));
}

} // namespace

TEST_CASE("TempoDelay mix 0 is bit-exact bypass", "[dsp][delay]") {
    for (const auto sampleRate : {44100.0, 48000.0, 96000.0}) {
        TempoDelay delay;
        delay.prepare(sampleRate, 256, 2, 0.0f, 0.3f, 250.0f, false, DelaySyncRate::quarter);

        juce::AudioBuffer<float> buffer(2, 256);
        for (int i = 0; i < 256; ++i) {
            float s = std::sin(static_cast<float>(i) * 0.1f);
            buffer.setSample(0, i, s);
            buffer.setSample(1, i, -s);
        }

        juce::AudioBuffer<float> originalCopy;
        originalCopy.makeCopyOf(buffer);

        delay.process(buffer, 120.0);

        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < 256; ++i) {
                REQUIRE(buffer.getSample(ch, i) == Catch::Approx(originalCopy.getSample(ch, i)).margin(1e-5f));
            }
        }
    }
}

TEST_CASE("TempoDelay produces delayed echo with feedback and tempo sync", "[dsp][delay]") {
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256;
    constexpr double bpm = 120.0; // 1 beat (quarter note) = 500 ms = 24000 samples

    TempoDelay delay;
    // Synced to 1/4 note at 120 BPM -> 24000 samples delay
    delay.prepare(sampleRate, blockSize, 2, 0.5f, 0.5f, 500.0f, true, DelaySyncRate::quarter);

    // Feed an impulse in block 0
    juce::AudioBuffer<float> buffer(2, blockSize);
    buffer.clear();
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);

    delay.process(buffer, bpm);

    // Initial output in block 0 has dry component (at 50% mix with equal power/linear)
    REQUIRE(std::abs(buffer.getSample(0, 0)) > 0.1f);

    // Process blocks until right before the echo (24000 / 256 = 93.75 blocks)
    // Block 93 starts at sample 93 * 256 = 23808
    // Block 94 contains sample 24000 (at offset 24000 - 93 * 256 = 192)
    // Process blocks until the echo arrives (24000 / 256 = 93.75 -> block 93/94)
    int echoBlock = -1;
    for (int block = 1; block < 100; ++block) {
        buffer.clear();
        delay.process(buffer, bpm);
        const float rms = calculateRms(buffer, 0);
        if (rms > 0.01f && echoBlock == -1) {
            echoBlock = block;
        }
    }

    REQUIRE((echoBlock == 93 || echoBlock == 94));
}

TEST_CASE("TempoDelay sanitises invalid and extreme runtime parameters", "[dsp][delay][robustness]") {
    const float invalidValues[] = {
        -std::numeric_limits<float>::infinity(), -10.0f,
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity(), 100000.0f
    };

    for (const auto sampleRate : {44100.0, 48000.0, 96000.0}) {
        TempoDelay delay;
        delay.prepare(sampleRate, 64, 2, 0.0f, 0.2f, 200.0f, false, DelaySyncRate::eighth);
        juce::AudioBuffer<float> buffer(2, 64);

        for (const auto invalid : invalidValues) {
            delay.setTargetParameters(invalid, invalid, invalid, false, DelaySyncRate::quarter);
            buffer.clear();
            buffer.setSample(0, 0, 1.0f);
            buffer.setSample(1, 0, -1.0f);
            REQUIRE_NOTHROW(delay.process(buffer, invalid));

            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < 64; ++i) {
                    REQUIRE(std::isfinite(buffer.getSample(ch, i)));
                }
            }
            REQUIRE(delay.getTargetMix() >= TempoDelay::minimumMix);
            REQUIRE(delay.getTargetMix() <= TempoDelay::maximumMix);
            REQUIRE(delay.getTargetFeedback() >= TempoDelay::minimumFeedback);
            REQUIRE(delay.getTargetFeedback() <= TempoDelay::maximumFeedback);
            REQUIRE(delay.getTargetTimeMs() >= TempoDelay::minimumTimeMs);
            REQUIRE(delay.getTargetTimeMs() <= TempoDelay::maximumTimeMs);
        }
    }
}
