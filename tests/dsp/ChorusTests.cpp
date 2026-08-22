#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include "dsp/Chorus.h"
#include <cmath>
#include <limits>

using chordsynth::dsp::Chorus;

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

TEST_CASE("Chorus mix 0 is bit-exact or perceptual bypass", "[dsp][chorus]") {
    for (const auto sampleRate : {44100.0, 48000.0, 96000.0}) {
        Chorus chorus;
        chorus.prepare(sampleRate, 256, 2, 0.0f, 1.0f, 0.5f);

        juce::AudioBuffer<float> buffer(2, 256);
        for (int i = 0; i < 256; ++i) {
            float s = std::sin(static_cast<float>(i) * 0.1f);
            buffer.setSample(0, i, s);
            buffer.setSample(1, i, -s);
        }

        juce::AudioBuffer<float> originalCopy;
        originalCopy.makeCopyOf(buffer);

        chorus.process(buffer);

        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < 256; ++i) {
                REQUIRE(buffer.getSample(ch, i) == Catch::Approx(originalCopy.getSample(ch, i)).margin(1e-5f));
            }
        }
    }
}

TEST_CASE("Chorus mix > 0 modifies stereo signal and remains finite", "[dsp][chorus]") {
    Chorus chorus;
    chorus.prepare(48000.0, 512, 2, 0.5f, 1.5f, 0.5f);

    juce::AudioBuffer<float> buffer(2, 512);

    // Process continuous audio blocks to allow modulation to evolve
    double phase = 0.0;
    for (int block = 0; block < 10; ++block) {
        for (int i = 0; i < 512; ++i) {
            float s = static_cast<float>(std::sin(phase));
            phase += 2.0 * juce::MathConstants<double>::pi * 440.0 / 48000.0;
            buffer.setSample(0, i, s);
            buffer.setSample(1, i, s);
        }
        chorus.process(buffer);
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < 512; ++i) {
                REQUIRE(std::isfinite(buffer.getSample(ch, i)));
            }
        }
    }

    REQUIRE(calculateRms(buffer, 0) > 0.1f);
}

TEST_CASE("Chorus sanitises invalid and extreme runtime parameters", "[dsp][chorus][robustness]") {
    const float invalidValues[] = {
        -std::numeric_limits<float>::infinity(), -10.0f,
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity(), 100.0f
    };

    for (const auto sampleRate : {44100.0, 48000.0, 96000.0}) {
        Chorus chorus;
        chorus.prepare(sampleRate, 64, 2, 0.0f, 1.0f, 0.25f);
        juce::AudioBuffer<float> buffer(2, 64);

        for (const auto invalid : invalidValues) {
            chorus.setTargetParameters(invalid, invalid, invalid);
            buffer.clear();
            buffer.setSample(0, 0, 1.0f);
            buffer.setSample(1, 0, -1.0f);
            REQUIRE_NOTHROW(chorus.process(buffer));

            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < 64; ++i) {
                    REQUIRE(std::isfinite(buffer.getSample(ch, i)));
                }
            }
            REQUIRE(chorus.getTargetMix() >= Chorus::minimumMix);
            REQUIRE(chorus.getTargetMix() <= Chorus::maximumMix);
            REQUIRE(chorus.getTargetRateHz() >= Chorus::minimumRateHz);
            REQUIRE(chorus.getTargetRateHz() <= Chorus::maximumRateHz);
            REQUIRE(chorus.getTargetDepth() >= Chorus::minimumDepth);
            REQUIRE(chorus.getTargetDepth() <= Chorus::maximumDepth);
        }
    }
}
