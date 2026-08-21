#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "music/NoteMath.h"

using Catch::Matchers::WithinAbs;

TEST_CASE("midiToFrequency converts standard reference pitches correctly", "[music][math]")
{
    SECTION("A4 (MIDI 69) is exactly 440.0 Hz")
    {
        REQUIRE_THAT(chordsynth::music::midiToFrequency(69), WithinAbs(440.0, 0.0001));
    }

    SECTION("Middle C (MIDI 60) is approximately 261.6256 Hz")
    {
        REQUIRE_THAT(chordsynth::music::midiToFrequency(60), WithinAbs(261.625565, 0.001));
    }

    SECTION("A3 (MIDI 57) is exactly 220.0 Hz")
    {
        REQUIRE_THAT(chordsynth::music::midiToFrequency(57), WithinAbs(220.0, 0.0001));
    }
}
