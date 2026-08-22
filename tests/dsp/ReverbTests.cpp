#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include "dsp/Reverb.h"
#include <algorithm>
#include <cmath>
#include <limits>

using chordsynth::dsp::Reverb;

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

TEST_CASE("Reverb wet 0 is bit-exact bypass", "[dsp][reverb]") {
    for (const auto sampleRate : {44100.0, 48000.0, 96000.0}) {
        Reverb reverb;
        reverb.prepare(sampleRate, 256, 2, 0.0f, 0.5f, 0.5f, 0.5f);

        juce::AudioBuffer<float> buffer(2, 256);
        for (int i = 0; i < 256; ++i) {
            float s = std::sin(static_cast<float>(i) * 0.1f);
            buffer.setSample(0, i, s);
            buffer.setSample(1, i, -s);
        }

        juce::AudioBuffer<float> originalCopy;
        originalCopy.makeCopyOf(buffer);

        reverb.process(buffer);

        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < 256; ++i) {
                REQUIRE(buffer.getSample(ch, i) == Catch::Approx(originalCopy.getSample(ch, i)).margin(1e-5f));
            }
        }
    }
}

TEST_CASE("Reverb impulse produces tail and decays over time", "[dsp][reverb]") {
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256;

    Reverb reverb;
    reverb.prepare(sampleRate, blockSize, 2, 0.5f, 0.6f, 0.5f, 0.7f);

    juce::AudioBuffer<float> buffer(2, blockSize);
    buffer.clear();
    // Feed single impulse in block 0
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);
    reverb.process(buffer);

    // Comb filters have delay of ~1116 to 1617 samples (at 48kHz ~1200..1760 samples -> blocks 4..7)
    // Run several blocks and check that reverb tail develops and decays
    float maxTailRms = 0.0f;
    for (int b = 1; b < 15; ++b) {
        buffer.clear();
        reverb.process(buffer);
        float rms = calculateRms(buffer, 0);
        if (rms > maxTailRms)
            maxTailRms = rms;
    }
    REQUIRE(maxTailRms > 0.001f); // Reverb tail is audible

    // Run 100 more blocks (tail decay)
    float lastEnergy = maxTailRms;
    for (int b = 15; b < 120; ++b) {
        buffer.clear();
        reverb.process(buffer);
        lastEnergy = calculateRms(buffer, 0);
    }

    // Tail should have decayed significantly
    REQUIRE(lastEnergy < maxTailRms * 0.5f);
}

TEST_CASE("Reverb reset clears internal tail state", "[dsp][reverb]") {
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256;

    Reverb reverb;
    reverb.prepare(sampleRate, blockSize, 2, 0.5f, 0.8f, 0.5f, 0.5f);

    juce::AudioBuffer<float> buffer(2, blockSize);
    buffer.clear();
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);
    reverb.process(buffer);

    // Reset clears tail
    reverb.reset();

    buffer.clear();
    reverb.process(buffer);
    REQUIRE(calculateRms(buffer, 0) == Catch::Approx(0.0f).margin(1e-6f));
}

TEST_CASE("Reverb sanitises invalid and extreme runtime parameters", "[dsp][reverb][robustness]") {
    const float invalidValues[] = {
        -std::numeric_limits<float>::infinity(), -10.0f,
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity(), 100.0f
    };

    for (const auto sampleRate : {44100.0, 48000.0, 96000.0}) {
        Reverb reverb;
        reverb.prepare(sampleRate, 64, 2, 0.0f, 0.5f, 0.5f, 0.5f);
        juce::AudioBuffer<float> buffer(2, 64);

        for (const auto invalid : invalidValues) {
            reverb.setTargetParameters(invalid, invalid, invalid, invalid);
            buffer.clear();
            buffer.setSample(0, 0, 1.0f);
            buffer.setSample(1, 0, -1.0f);
            REQUIRE_NOTHROW(reverb.process(buffer));

            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < 64; ++i) {
                    REQUIRE(std::isfinite(buffer.getSample(ch, i)));
                }
            }
            REQUIRE(reverb.getTargetMix() >= Reverb::minimumMix);
            REQUIRE(reverb.getTargetMix() <= Reverb::maximumMix);
            REQUIRE(reverb.getTargetRoomSize() >= Reverb::minimumRoomSize);
            REQUIRE(reverb.getTargetRoomSize() <= Reverb::maximumRoomSize);
            REQUIRE(reverb.getTargetDamping() >= Reverb::minimumDamping);
            REQUIRE(reverb.getTargetDamping() <= Reverb::maximumDamping);
            REQUIRE(reverb.getTargetWidth() >= Reverb::minimumWidth);
            REQUIRE(reverb.getTargetWidth() <= Reverb::maximumWidth);
        }
    }
}
