#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "dsp/Oscillator.h"
#include <array>
#include <cmath>
#include <limits>
#include <vector>

using namespace chordsynth::dsp;
using Catch::Matchers::WithinAbs;

TEST_CASE("Oscillator renders deterministic, bounded, sample-rate-independent sine waveform", "[dsp][oscillator]") {
    Oscillator osc;

    SECTION("A4 (440 Hz) at 48000 Hz sample rate has period consistent with 440 Hz") {
        osc.prepare(48000.0);
        osc.setFrequency(440.0f);
        osc.reset();

        // Count zero crossings in 1 second (48000 samples)
        int zeroCrossings = 0;
        float prevSample = osc.processSample();

        for (int i = 1; i < 48000; ++i) {
            float sample = osc.processSample();
            if ((prevSample <= 0.0f && sample > 0.0f) || (prevSample >= 0.0f && sample < 0.0f)) {
                zeroCrossings++;
            }
            prevSample = sample;
        }

        // In 440 full cycles, there are ~880 zero crossings (2 per cycle)
        REQUIRE(zeroCrossings >= 878);
        REQUIRE(zeroCrossings <= 882);
    }

    SECTION("Phase and output remain strictly bounded in [-1.0, 1.0] over many blocks without NaN or drift") {
        osc.prepare(96000.0);
        osc.setFrequency(1000.0f);
        osc.reset();

        for (int i = 0; i < 96000 * 2; ++i) { // 2 seconds
            float sample = osc.processSample();
            REQUIRE_FALSE(std::isnan(sample));
            REQUIRE_FALSE(std::isinf(sample));
            REQUIRE(sample >= -1.0f);
            REQUIRE(sample <= 1.0f);
        }
    }

    SECTION("Reset produces an exact, deterministic output sequence") {
        osc.prepare(44100.0);
        osc.setFrequency(220.0f);
        osc.reset();

        std::vector<float> firstPass;
        firstPass.reserve(512);
        for (int i = 0; i < 512; ++i) {
            firstPass.push_back(osc.processSample());
        }

        // Reset and re-run
        osc.reset();
        for (int i = 0; i < 512; ++i) {
            float sample = osc.processSample();
            REQUIRE_THAT(sample, WithinAbs(firstPass[static_cast<std::size_t>(i)], 1e-6f));
        }
    }

    SECTION("Changing sample rate preserves the output frequency timing") {
        // Test at 44.1 kHz vs 88.2 kHz: 1 cycle at 441 Hz takes 100 samples at 44.1k, 200 samples at 88.2k
        // Sample index 0 is phase 0 (sin = 0).
        // Sample index 100 at 44.1k is phase 100 * 0.01 = 1.0 -> wrapped phase 0 (sin = 0).
        // Sample index 200 at 88.2k is phase 200 * 0.005 = 1.0 -> wrapped phase 0 (sin = 0).
        osc.prepare(44100.0);
        osc.setFrequency(441.0f);
        osc.reset();

        for (int i = 0; i < 100; ++i) {
            std::ignore = osc.processSample();
        }
        float sample44k = osc.processSample();
        REQUIRE_THAT(sample44k, WithinAbs(0.0f, 0.07f)); // sample at start of next cycle (index 100) is close to 0

        // Now at 88200.0 Hz
        osc.prepare(88200.0);
        osc.setFrequency(441.0f);
        osc.reset();
        for (int i = 0; i < 200; ++i) {
            std::ignore = osc.processSample();
        }
        float sample88k = osc.processSample();
        REQUIRE_THAT(sample88k, WithinAbs(0.0f, 0.07f));
    }
}

TEST_CASE("Raw waveform choices map by nearest selection with defensive clamping", "[dsp][oscillator][waveform]") {
    REQUIRE(waveformFromRawChoice(-100.0f) == Waveform::sine);
    REQUIRE(waveformFromRawChoice(0.49f) == Waveform::sine);
    REQUIRE(waveformFromRawChoice(0.5f) == Waveform::saw);
    REQUIRE(waveformFromRawChoice(2.49f) == Waveform::square);
    REQUIRE(waveformFromRawChoice(2.5f) == Waveform::triangle);
    REQUIRE(waveformFromRawChoice(100.0f) == Waveform::triangle);
    REQUIRE(waveformFromRawChoice(std::numeric_limits<float>::quiet_NaN()) == Waveform::sine);
    REQUIRE(waveformFromRawChoice(std::numeric_limits<float>::infinity()) == Waveform::sine);
}

TEST_CASE("Every oscillator waveform has its defined quarter-cycle sequence and period", "[dsp][oscillator][waveform]") {
    struct Case { Waveform waveform; std::array<float, 4> expected; };
    const std::array cases{
        Case{Waveform::sine, {0.0f, 1.0f, 0.0f, -1.0f}},
        Case{Waveform::saw, {-1.0f, -0.5f, 0.0f, 0.5f}},
        Case{Waveform::square, {1.0f, 1.0f, -1.0f, -1.0f}},
        Case{Waveform::triangle, {0.0f, 1.0f, 0.0f, -1.0f}},
    };
    for (const auto& testCase : cases) {
        Oscillator osc;
        osc.prepare(4.0);
        osc.setFrequency(1.0f);
        osc.setWaveform(testCase.waveform);
        osc.reset();
        for (int cycle = 0; cycle < 2; ++cycle)
            for (const auto expected : testCase.expected)
                REQUIRE_THAT(osc.processSample(), WithinAbs(expected, 1.0e-6f));
    }
}

TEST_CASE("All oscillator waveforms stay bounded and finite at representative rates and frequencies", "[dsp][oscillator][waveform]") {
    const std::array waveforms{Waveform::sine, Waveform::saw, Waveform::square, Waveform::triangle};
    const std::array sampleRates{8000.0, 44100.0, 96000.0};
    const std::array frequencies{0.0f, 20.0f, 440.0f, 12000.0f};
    for (const auto waveform : waveforms) for (const auto sampleRate : sampleRates) for (const auto frequency : frequencies) {
        Oscillator osc;
        osc.prepare(sampleRate);
        osc.setFrequency(frequency);
        osc.setWaveform(waveform);
        for (int i = 0; i < 2048; ++i) {
            const auto sample = osc.processSample();
            REQUIRE(std::isfinite(sample));
            REQUIRE(sample >= -1.0f);
            REQUIRE(sample <= 1.0f);
        }
    }
}

TEST_CASE("Switching oscillator waveforms never emits invalid samples", "[dsp][oscillator][waveform]") {
    const std::array waveforms{Waveform::sine, Waveform::saw, Waveform::square, Waveform::triangle};
    Oscillator osc;
    osc.prepare(48000.0);
    osc.setFrequency(997.0f);
    for (int i = 0; i < 4096; ++i) {
        osc.setWaveform(waveforms[static_cast<std::size_t>(i) % waveforms.size()]);
        const auto sample = osc.processSample();
        REQUIRE(std::isfinite(sample));
        REQUIRE(sample >= -1.0f);
        REQUIRE(sample <= 1.0f);
    }
}
