#include <catch2/catch_test_macros.hpp>
#include <music/VoicedChord.h>

using namespace chordsynth::music;

TEST_CASE("NoteSet exposes only its active MIDI notes", "[music][voicing]") {
    NoteSet notes{{60, 64, 67, 0, 0, 0}, 3};
    REQUIRE(notes.size() == 3);
    REQUIRE(notes[0] == 60);
    REQUIRE(notes[1] == 64);
    REQUIRE(notes[2] == 67);
    REQUIRE_FALSE(notes.empty());
}

TEST_CASE("NoteSet clamps count within 0..6", "[music][voicing]") {
    NoteSet emptyNotes{{60, 64, 67, 71, 74, 77}, -2};
    REQUIRE(emptyNotes.size() == 0);
    REQUIRE(emptyNotes.empty());

    NoteSet maxNotes{{60, 64, 67, 71, 74, 77}, 10};
    REQUIRE(maxNotes.size() == 6);
    REQUIRE(maxNotes[5] == 77);
}

TEST_CASE("NoteSet iteration and equality", "[music][voicing]") {
    NoteSet a{{60, 64, 67, 0, 0, 0}, 3};
    NoteSet b{{60, 64, 67, 0, 0, 0}, 3};
    NoteSet c{{60, 64, 67, 71, 74, 77}, 6};

    REQUIRE(a == b);
    REQUIRE_FALSE(a == c);

    int count = 0;
    for (int note : a) {
        (void)note;
        ++count;
    }
    REQUIRE(count == 3);
}

TEST_CASE("VoicingSpec defines contracts with expected defaults", "[music][voicing]") {
    VoicingSpec spec{};
    REQUIRE(spec.extension == ChordExtension::triad);
    REQUIRE(spec.inversion == 0);
    REQUIRE(spec.style == VoicingStyle::close);
    REQUIRE(spec.baseOctave == 3);
    REQUIRE(spec.qualityRule == QualityRule::diatonic);

    VoicingSpec customSpec{
        ChordExtension::seventh,
        1,
        VoicingStyle::open,
        4,
        QualityRule::major
    };
    REQUIRE(customSpec.extension == ChordExtension::seventh);
    REQUIRE(customSpec.inversion == 1);
    REQUIRE(customSpec.style == VoicingStyle::open);
    REQUIRE(customSpec.baseOctave == 4);
    REQUIRE(customSpec.qualityRule == QualityRule::major);
}
