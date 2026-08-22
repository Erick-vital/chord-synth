#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include "music/MusicalClock.h"
#include <cmath>

using chordsynth::music::MusicalClock;
using chordsynth::music::ArpRate;

TEST_CASE("MusicalClock at 120 BPM 1/8 produces correct sample interval", "[music][clock]") {
    constexpr double sampleRate = 48000.0;
    constexpr double bpm = 120.0; // 1 beat = 0.5s = 24000 samples -> 1/8 = 0.25s = 12000 samples

    MusicalClock clock;
    clock.prepare(sampleRate);
    clock.setRate(ArpRate::eighth);

    const int intervalSamples = clock.getSamplesPerStep(bpm);
    REQUIRE(intervalSamples == 12000);
}

TEST_CASE("MusicalClock sample-rate / BPM changes maintain valid non-zero step intervals", "[music][clock]") {
    MusicalClock clock;

    for (const auto sampleRate : {44100.0, 48000.0, 96000.0, 192000.0}) {
        clock.prepare(sampleRate);

        clock.setRate(ArpRate::quarter);
        REQUIRE(clock.getSamplesPerStep(120.0) == static_cast<int>(std::round(sampleRate * 0.5)));

        clock.setRate(ArpRate::eighth);
        REQUIRE(clock.getSamplesPerStep(120.0) == static_cast<int>(std::round(sampleRate * 0.25)));

        clock.setRate(ArpRate::sixteenth);
        REQUIRE(clock.getSamplesPerStep(120.0) == static_cast<int>(std::round(sampleRate * 0.125)));
    }
}

TEST_CASE("MusicalClock fallback BPM is used when BPM is invalid or missing", "[music][clock]") {
    MusicalClock clock;
    clock.prepare(48000.0);
    clock.setRate(ArpRate::quarter);

    // Invalid BPMs fall back to defaultBpm = 120.0
    REQUIRE(clock.getSamplesPerStep(-10.0) == 24000);
    REQUIRE(clock.getSamplesPerStep(0.0) == 24000);
    REQUIRE(clock.getSamplesPerStep(std::numeric_limits<double>::quiet_NaN()) == 24000);
    REQUIRE(clock.getSamplesPerStep(std::numeric_limits<double>::infinity()) == 24000);
}
