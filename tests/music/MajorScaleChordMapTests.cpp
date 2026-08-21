#include <catch2/catch_test_macros.hpp>
#include "music/MajorScaleChordMap.h"
#include <stdexcept>

using namespace chordsynth::music;

TEST_CASE("MajorScaleChordMap maps C major scale degrees to diatonic triads", "[music][chords]") {
    MajorScaleChordMap map;
    // C is pitch class 0, base octave 4 -> C4 is MIDI note 60 (12 * (4 + 1) = 60)

    SECTION("degrees 0 to 6 produce correct roots, qualities, and notes in C major (octave 4)") {
        // Degree 0 (I): C major triad -> C4 (60), E4 (64), G4 (67)
        auto c = map.chordForDegree(0, 4, 0);
        REQUIRE(c.degree == 0);
        REQUIRE(c.rootMidi == 60);
        REQUIRE(c.quality == ChordQuality::major);
        REQUIRE(c.midiNotes == std::array<int, 3>{60, 64, 67});
        REQUIRE(c.label == "C");

        // Degree 1 (ii): D minor triad -> D4 (62), F4 (65), A4 (69)
        auto dm = map.chordForDegree(0, 4, 1);
        REQUIRE(dm.degree == 1);
        REQUIRE(dm.rootMidi == 62);
        REQUIRE(dm.quality == ChordQuality::minor);
        REQUIRE(dm.midiNotes == std::array<int, 3>{62, 65, 69});
        REQUIRE(dm.label == "Dm");

        // Degree 2 (iii): E minor triad -> E4 (64), G4 (67), B4 (71)
        auto em = map.chordForDegree(0, 4, 2);
        REQUIRE(em.degree == 2);
        REQUIRE(em.rootMidi == 64);
        REQUIRE(em.quality == ChordQuality::minor);
        REQUIRE(em.midiNotes == std::array<int, 3>{64, 67, 71});
        REQUIRE(em.label == "Em");

        // Degree 3 (IV): F major triad -> F4 (65), A4 (69), C5 (72)
        auto f = map.chordForDegree(0, 4, 3);
        REQUIRE(f.degree == 3);
        REQUIRE(f.rootMidi == 65);
        REQUIRE(f.quality == ChordQuality::major);
        REQUIRE(f.midiNotes == std::array<int, 3>{65, 69, 72});
        REQUIRE(f.label == "F");

        // Degree 4 (V): G major triad -> G4 (67), B4 (71), D5 (74)
        auto g = map.chordForDegree(0, 4, 4);
        REQUIRE(g.degree == 4);
        REQUIRE(g.rootMidi == 67);
        REQUIRE(g.quality == ChordQuality::major);
        REQUIRE(g.midiNotes == std::array<int, 3>{67, 71, 74});
        REQUIRE(g.label == "G");

        // Degree 5 (vi): A minor triad -> A4 (69), C5 (72), E5 (76)
        auto am = map.chordForDegree(0, 4, 5);
        REQUIRE(am.degree == 5);
        REQUIRE(am.rootMidi == 69);
        REQUIRE(am.quality == ChordQuality::minor);
        REQUIRE(am.midiNotes == std::array<int, 3>{69, 72, 76});
        REQUIRE(am.label == "Am");

        // Degree 6 (vii°): B diminished triad -> B4 (71), D5 (74), F5 (77)
        auto bdim = map.chordForDegree(0, 4, 6);
        REQUIRE(bdim.degree == 6);
        REQUIRE(bdim.rootMidi == 71);
        REQUIRE(bdim.quality == ChordQuality::diminished);
        REQUIRE(bdim.midiNotes == std::array<int, 3>{71, 74, 77});
        REQUIRE(bdim.label == "Bdim");
    }

    SECTION("C# major (pitch class 1) transposes all results by +1 semitone") {
        // Degree 0 in C#4: C#4 (61), E#4/F4 (65), G#4 (68)
        auto cs = map.chordForDegree(1, 4, 0);
        REQUIRE(cs.rootMidi == 61);
        REQUIRE(cs.quality == ChordQuality::major);
        REQUIRE(cs.midiNotes == std::array<int, 3>{61, 65, 68});
        REQUIRE(cs.label == "C#");

        // Degree 4 (V) in C#4: G#4 (68), B#4/C5 (72), D#5 (75)
        auto gs = map.chordForDegree(1, 4, 4);
        REQUIRE(gs.rootMidi == 68);
        REQUIRE(gs.quality == ChordQuality::major);
        REQUIRE(gs.midiNotes == std::array<int, 3>{68, 72, 75});
        REQUIRE(gs.label == "G#");
    }

    SECTION("invalid degree values throw std::out_of_range") {
        REQUIRE_THROWS_AS(map.chordForDegree(0, 4, -1), std::out_of_range);
        REQUIRE_THROWS_AS(map.chordForDegree(0, 4, 7), std::out_of_range);
        REQUIRE_THROWS_AS(map.chordForDegree(0, 4, 100), std::out_of_range);
    }

    SECTION("tonic pitch class normalizes modulo 12") {
        auto c = map.chordForDegree(12, 4, 0);
        REQUIRE(c.rootMidi == 60);
        REQUIRE(c.label == "C");
    }

    SECTION("notes stay bounded within MIDI range 0..127 or throw if out of range") {
        // C-1 degree 0 gives root=0, third=4, fifth=7 -> all >= 0 and <= 127
        auto low = map.chordForDegree(0, -1, 0);
        REQUIRE(low.rootMidi == 0);
        REQUIRE(low.midiNotes[0] == 0);
        REQUIRE(low.midiNotes[1] == 4);
        REQUIRE(low.midiNotes[2] == 7);

        // An octave/degree that would push notes above 127 or below 0 throws out_of_range
        REQUIRE_THROWS_AS(map.chordForDegree(0, -2, 0), std::out_of_range);
        REQUIRE_THROWS_AS(map.chordForDegree(0, 10, 0), std::out_of_range);
    }
}
