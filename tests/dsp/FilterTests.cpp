#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include "dsp/Filter.h"
#include <cmath>
#include <limits>

using chordsynth::dsp::Filter;

namespace {

float renderSineRms(double sampleRate, float cutoff, float frequency) {
    constexpr int blockSize = 128;
    constexpr int blocks = 80;
    Filter filter;
    filter.prepare(sampleRate, blockSize, 2, cutoff, 0.2f);
    juce::AudioBuffer<float> buffer(2, blockSize);
    double phase = 0.0;
    double sumSquares = 0.0;
    int samplesMeasured = 0;
    for (int block = 0; block < blocks; ++block) {
        for (int sample = 0; sample < blockSize; ++sample) {
            const auto value = static_cast<float>(std::sin(phase));
            phase += 2.0 * juce::MathConstants<double>::pi * frequency / sampleRate;
            for (int channel = 0; channel < 2; ++channel)
                buffer.setSample(channel, sample, value);
        }
        filter.process(buffer);
        if (block >= 20) {
            for (int sample = 0; sample < blockSize; ++sample) {
                const auto value = buffer.getSample(0, sample);
                REQUIRE(std::isfinite(value));
                sumSquares += static_cast<double>(value) * value;
                ++samplesMeasured;
            }
        }
    }
    return static_cast<float>(std::sqrt(sumSquares / samplesMeasured));
}

} // namespace

TEST_CASE("Global low-pass attenuates highs and remains stable at supported rates", "[dsp][filter]") {
    for (const auto sampleRate : {44100.0, 48000.0, 96000.0}) {
        const auto lowCutoffRms = renderSineRms(sampleRate, 300.0f, 6000.0f);
        const auto highCutoffRms = renderSineRms(sampleRate, 18000.0f, 6000.0f);
        INFO("sample rate " << sampleRate);
        REQUIRE(lowCutoffRms < 0.08f);
        REQUIRE(highCutoffRms > lowCutoffRms * 5.0f);
        REQUIRE(highCutoffRms > 0.35f);
    }
}

TEST_CASE("Global low-pass sanitises invalid and extreme runtime values at supported rates",
          "[dsp][filter][robustness]") {
    const float invalidValues[] = {
        -std::numeric_limits<float>::infinity(), -1.0e30f,
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity(), 1.0e30f};

    for (const auto sampleRate : {44100.0, 48000.0, 96000.0}) {
        Filter filter;
        filter.prepare(sampleRate, 64, 1, 8000.0f, 0.2f);
        juce::AudioBuffer<float> buffer(1, 64);
        for (const auto invalid : invalidValues) {
            filter.setTargetParameters(invalid, invalid);
            buffer.clear();
            buffer.setSample(0, 0, 1.0f);
            REQUIRE_NOTHROW(filter.process(buffer));
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                REQUIRE(std::isfinite(buffer.getSample(0, sample)));
            REQUIRE(filter.getTargetCutoff() >= Filter::minimumCutoffHz);
            REQUIRE(filter.getTargetCutoff() < static_cast<float>(sampleRate * 0.5));
            REQUIRE(filter.getTargetResonance() >= Filter::minimumResonance);
            REQUIRE(filter.getTargetResonance() <= Filter::maximumResonance);
        }
    }

    Filter veryLowRateFilter;
    veryLowRateFilter.prepare(30.0, 8, 1, 20000.0f, 0.2f);
    REQUIRE(veryLowRateFilter.getTargetCutoff() > 0.0f);
    REQUIRE(veryLowRateFilter.getTargetCutoff() < 15.0f);
    juce::AudioBuffer<float> lowRateBuffer(1, 8);
    lowRateBuffer.clear();
    lowRateBuffer.setSample(0, 0, 1.0f);
    veryLowRateFilter.process(lowRateBuffer);
    for (int sample = 0; sample < lowRateBuffer.getNumSamples(); ++sample)
        REQUIRE(std::isfinite(lowRateBuffer.getSample(0, sample)));
}

TEST_CASE("Filter automation follows an exact deterministic 20 ms ramp", "[dsp][filter][automation]") {
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 64;
    constexpr int rampSamples = 960; // 20 ms at 48 kHz
    constexpr float initialCutoff = 8000.0f;
    constexpr float targetCutoff = 200.0f;
    constexpr float initialResonance = 0.2f;
    constexpr float targetResonance = 2.0f;

    Filter first;
    Filter second;
    first.prepare(sampleRate, blockSize, 2, initialCutoff, initialResonance);
    second.prepare(sampleRate, blockSize, 2, initialCutoff, initialResonance);
    first.setTargetParameters(targetCutoff, targetResonance);
    second.setTargetParameters(targetCutoff, targetResonance);
    REQUIRE(first.getCurrentCutoff() == Catch::Approx(initialCutoff));
    REQUIRE(first.getCurrentResonance() == Catch::Approx(initialResonance));

    juce::AudioBuffer<float> firstBuffer(2, blockSize);
    juce::AudioBuffer<float> secondBuffer(2, blockSize);
    for (int block = 1; block <= rampSamples / blockSize; ++block) {
        firstBuffer.clear();
        secondBuffer.clear();
        first.process(firstBuffer);
        second.process(secondBuffer);

        const auto samplesProcessed = block * blockSize;
        const auto progress = static_cast<float>(samplesProcessed) / rampSamples;
        const auto expectedCutoff = initialCutoff + (targetCutoff - initialCutoff) * progress;
        const auto expectedResonance = initialResonance
            + (targetResonance - initialResonance) * progress;
        REQUIRE(first.getCurrentCutoff() == Catch::Approx(expectedCutoff).margin(0.02f));
        REQUIRE(first.getCurrentResonance() == Catch::Approx(expectedResonance).margin(0.001f));
        REQUIRE(second.getCurrentCutoff() == first.getCurrentCutoff());
        REQUIRE(second.getCurrentResonance() == first.getCurrentResonance());

        if (block < rampSamples / blockSize)
            REQUIRE(first.getCurrentCutoff() > targetCutoff);
    }

    REQUIRE(first.getCurrentCutoff() == Catch::Approx(targetCutoff).margin(0.01f));
    REQUIRE(first.getCurrentResonance() == Catch::Approx(targetResonance).margin(0.001f));
}

TEST_CASE("Rapid filter automation remains finite while smoothing targets change",
          "[dsp][filter][automation]") {
    Filter filter;
    filter.prepare(48000.0, 64, 2, 8000.0f, 0.2f);
    juce::AudioBuffer<float> buffer(2, 64);
    for (int i = 0; i < 32; ++i) {
        filter.setTargetParameters(i % 2 == 0 ? 400.0f : 12000.0f,
                                   i % 2 == 0 ? 1.8f : 0.1f);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);
        buffer.setSample(1, 0, -1.0f);
        filter.process(buffer);
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                REQUIRE(std::isfinite(buffer.getSample(channel, sample)));
    }
}