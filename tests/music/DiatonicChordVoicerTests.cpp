#include <catch2/catch_test_macros.hpp>
#include <music/DiatonicChordVoicer.h>

using namespace chordsynth::music;

TEST_CASE("DiatonicChordVoicer generates root close triads in C major", "[music][voicer]") {
    DiatonicChordVoicer voicer;
    VoicingSpec spec{
        .extension = ChordExtension::triad,
        .inversion = 0,
        .style = VoicingStyle::close,
        .baseOctave = 3,
        .qualityRule = QualityRule::diatonic
    };

    // Degree 0 (I) -> C3 (48), E3 (52), G3 (55)
    auto chordI = voicer.voiceChord(0, 0, spec);
    REQUIRE(chordI.label == "C");
    REQUIRE(chordI.notes.size() == 3);
    REQUIRE(chordI.notes[0] == 48);
    REQUIRE(chordI.notes[1] == 52);
    REQUIRE(chordI.notes[2] == 55);

    // Degree 1 (ii) -> D3 (50), F3 (53), A3 (57)
    auto chordii = voicer.voiceChord(0, 1, spec);
    REQUIRE(chordii.label == "Dm");
    REQUIRE(chordii.notes.size() == 3);
    REQUIRE(chordii.notes[0] == 50);
    REQUIRE(chordii.notes[1] == 53);
    REQUIRE(chordii.notes[2] == 57);
}

TEST_CASE("DiatonicChordVoicer generates diatonic seventh chords in C major", "[music][voicer]") {
    DiatonicChordVoicer voicer;
    VoicingSpec spec{
        .extension = ChordExtension::seventh,
        .inversion = 0,
        .style = VoicingStyle::close,
        .baseOctave = 3,
        .qualityRule = QualityRule::diatonic
    };

    // I: Cmaj7 = C3 (48), E3 (52), G3 (55), B3 (59)
    auto chordI = voicer.voiceChord(0, 0, spec);
    REQUIRE(chordI.label == "Cmaj7");
    REQUIRE(chordI.notes.size() == 4);
    REQUIRE(chordI.notes[0] == 48);
    REQUIRE(chordI.notes[1] == 52);
    REQUIRE(chordI.notes[2] == 55);
    REQUIRE(chordI.notes[3] == 59);

    // ii: Dm7 = D3 (50), F3 (53), A3 (57), C4 (60)
    auto chordii = voicer.voiceChord(0, 1, spec);
    REQUIRE(chordii.label == "Dm7");
    REQUIRE(chordii.notes.size() == 4);
    REQUIRE(chordii.notes[0] == 50);
    REQUIRE(chordii.notes[1] == 53);
    REQUIRE(chordii.notes[2] == 57);
    REQUIRE(chordii.notes[3] == 60);

    // V: G7 = G3 (55), B3 (59), D4 (62), F4 (65)
    auto chordV = voicer.voiceChord(0, 4, spec);
    REQUIRE(chordV.label == "G7");
    REQUIRE(chordV.notes.size() == 4);
    REQUIRE(chordV.notes[0] == 55);
    REQUIRE(chordV.notes[1] == 59);
    REQUIRE(chordV.notes[2] == 62);
    REQUIRE(chordV.notes[3] == 65);

    // vii: Bm7b5 = B3 (59), D4 (62), F4 (65), A4 (69)
    auto chordvii = voicer.voiceChord(0, 6, spec);
    REQUIRE(chordvii.label == "Bm7b5");
    REQUIRE(chordvii.notes.size() == 4);
    REQUIRE(chordvii.notes[0] == 59);
    REQUIRE(chordvii.notes[1] == 62);
    REQUIRE(chordvii.notes[2] == 65);
    REQUIRE(chordvii.notes[3] == 69);
}

TEST_CASE("DiatonicChordVoicer applies inversions correctly", "[music][voicer]") {
    DiatonicChordVoicer voicer;

    // C major 1st inversion (triad): E3 (52), G3 (55), C4 (60)
    VoicingSpec spec1st{
        .extension = ChordExtension::triad,
        .inversion = 1,
        .style = VoicingStyle::close,
        .baseOctave = 3,
        .qualityRule = QualityRule::diatonic
    };
    auto c1st = voicer.voiceChord(0, 0, spec1st);
    REQUIRE(c1st.notes.size() == 3);
    REQUIRE(c1st.notes[0] == 52);
    REQUIRE(c1st.notes[1] == 55);
    REQUIRE(c1st.notes[2] == 60);

    // C major 2nd inversion (triad): G3 (55), C4 (60), E4 (64)
    VoicingSpec spec2nd{
        .extension = ChordExtension::triad,
        .inversion = 2,
        .style = VoicingStyle::close,
        .baseOctave = 3,
        .qualityRule = QualityRule::diatonic
    };
    auto c2nd = voicer.voiceChord(0, 0, spec2nd);
    REQUIRE(c2nd.notes.size() == 3);
    REQUIRE(c2nd.notes[0] == 55);
    REQUIRE(c2nd.notes[1] == 60);
    REQUIRE(c2nd.notes[2] == 64);

    // Inversion > count - 1 clamps deterministically (inversion 5 on 3-note chord clamped to 2)
    VoicingSpec specClamped{
        .extension = ChordExtension::triad,
        .inversion = 5,
        .style = VoicingStyle::close,
        .baseOctave = 3,
        .qualityRule = QualityRule::diatonic
    };
    auto cClamped = voicer.voiceChord(0, 0, specClamped);
    REQUIRE(cClamped.notes == c2nd.notes);

    // Negative inversion clamps to 0
    VoicingSpec specNeg{
        .extension = ChordExtension::triad,
        .inversion = -1,
        .style = VoicingStyle::close,
        .baseOctave = 3,
        .qualityRule = QualityRule::diatonic
    };
    VoicingSpec specRoot{
        .extension = ChordExtension::triad,
        .inversion = 0,
        .style = VoicingStyle::close,
        .baseOctave = 3,
        .qualityRule = QualityRule::diatonic
    };
    REQUIRE(voicer.voiceChord(0, 0, specNeg).notes == voicer.voiceChord(0, 0, specRoot).notes);
}

TEST_CASE("DiatonicChordVoicer applies open voicing style", "[music][voicer]") {
    DiatonicChordVoicer voicer;

    // C major open (triad): root C3 (48), 5th G3 (55), 3rd raised E4 (64) -> [48, 55, 64]
    VoicingSpec specOpenTriad{
        .extension = ChordExtension::triad,
        .inversion = 0,
        .style = VoicingStyle::open,
        .baseOctave = 3,
        .qualityRule = QualityRule::diatonic
    };
    auto cOpen = voicer.voiceChord(0, 0, specOpenTriad);
    REQUIRE(cOpen.notes.size() == 3);
    REQUIRE(cOpen.notes[0] == 48);
    REQUIRE(cOpen.notes[1] == 55);
    REQUIRE(cOpen.notes[2] == 64);

    // Cmaj7 open (seventh): root C3 (48), 5th G3 (55), 7th B3 (59), 3rd raised E4 (64) -> [48, 55, 59, 64] (drop-2)
    VoicingSpec specOpen7th{
        .extension = ChordExtension::seventh,
        .inversion = 0,
        .style = VoicingStyle::open,
        .baseOctave = 3,
        .qualityRule = QualityRule::diatonic
    };
    auto c7Open = voicer.voiceChord(0, 0, specOpen7th);
    REQUIRE(c7Open.notes.size() == 4);
    REQUIRE(c7Open.notes[0] == 48);
    REQUIRE(c7Open.notes[1] == 55);
    REQUIRE(c7Open.notes[2] == 59);
    REQUIRE(c7Open.notes[3] == 64);
}

TEST_CASE("DiatonicChordVoicer applies custom QualityRule (free mode override)", "[music][voicer]") {
    DiatonicChordVoicer voicer;

    // In C major context, degree 1 (D) with QualityRule::major and seventh -> D3 (50), F#3 (54), A3 (57), C#4 (61) -> Dmaj7
    VoicingSpec specCustomMajor{
        .extension = ChordExtension::seventh,
        .inversion = 0,
        .style = VoicingStyle::close,
        .baseOctave = 3,
        .qualityRule = QualityRule::major
    };
    auto dMaj7 = voicer.voiceChord(0, 1, specCustomMajor);
    REQUIRE(dMaj7.label == "Dmaj7");
    REQUIRE(dMaj7.notes.size() == 4);
    REQUIRE(dMaj7.notes[0] == 50);
    REQUIRE(dMaj7.notes[1] == 54);
    REQUIRE(dMaj7.notes[2] == 57);
    REQUIRE(dMaj7.notes[3] == 61);

    // Diatonic mode still produces Dm7 for degree 1
    VoicingSpec specDiatonic{
        .extension = ChordExtension::seventh,
        .inversion = 0,
        .style = VoicingStyle::close,
        .baseOctave = 3,
        .qualityRule = QualityRule::diatonic
    };
    auto dDiatonic = voicer.voiceChord(0, 1, specDiatonic);
    REQUIRE(dDiatonic.label == "Dm7");
    REQUIRE(dDiatonic.notes[1] == 53); // F3
}

TEST_CASE("DiatonicChordVoicer validates octave and MIDI bounds (0..127)", "[music][voicer]") {
    DiatonicChordVoicer voicer;
    VoicingSpec specOctave2{
        .extension = ChordExtension::triad,
        .inversion = 0,
        .style = VoicingStyle::close,
        .baseOctave = 2,
        .qualityRule = QualityRule::diatonic
    };
    auto c2 = voicer.voiceChord(0, 0, specOctave2);
    REQUIRE(c2.notes[0] == 36); // C2

    VoicingSpec specOctave4{
        .extension = ChordExtension::triad,
        .inversion = 0,
        .style = VoicingStyle::close,
        .baseOctave = 4,
        .qualityRule = QualityRule::diatonic
    };
    auto c4 = voicer.voiceChord(0, 0, specOctave4);
    REQUIRE(c4.notes[0] == 60); // C4

    // Extreme octave or invalid degree throws or handles cleanly without NaN/out-of-bounds
    VoicingSpec specDefault{};
    REQUIRE_THROWS_AS(voicer.voiceChord(0, 7, specDefault), std::out_of_range);
    REQUIRE_THROWS_AS(voicer.voiceChord(0, -1, specDefault), std::out_of_range);
}
