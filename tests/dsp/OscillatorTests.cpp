#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "dsp/Oscillator.h"
#include <cmath>
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
