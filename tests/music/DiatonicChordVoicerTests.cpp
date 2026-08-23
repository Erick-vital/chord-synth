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

TEST_CASE("DiatonicChordVoicer generates natural minor diatonic chords", "[music][voicer][minor]") {
    DiatonicChordVoicer voicer;
    VoicingSpec spec{
        .extension = ChordExtension::seventh,
        .inversion = 0,
        .style = VoicingStyle::close,
        .baseOctave = 3,
        .qualityRule = QualityRule::diatonic
    };

    // C natural minor: C, D, Eb, F, G, Ab, Bb. The diatonic sevenths are
    // Cm7, Dm7b5, Ebmaj7, Fm7, Gm7, Abmaj7, Bb7.
    const auto tonic = 0;
    const auto cMinor = voicer.voiceChord(tonic, 0, spec, Scale::naturalMinor);
    REQUIRE(cMinor.label == "Cm7");
    REQUIRE(cMinor.notes == NoteSet({48, 51, 55, 58}, 4));

    const auto dHalfDiminished = voicer.voiceChord(tonic, 1, spec, Scale::naturalMinor);
    REQUIRE(dHalfDiminished.label == "Dm7b5");
    REQUIRE(dHalfDiminished.notes == NoteSet({50, 53, 56, 60}, 4));

    const auto eFlatMajorSeven = voicer.voiceChord(tonic, 2, spec, Scale::naturalMinor);
    REQUIRE(eFlatMajorSeven.label == "D#maj7");
    REQUIRE(eFlatMajorSeven.notes == NoteSet({51, 55, 58, 62}, 4));

    const auto bFlatSeven = voicer.voiceChord(tonic, 6, spec, Scale::naturalMinor);
    REQUIRE(bFlatSeven.label == "A#7");
    REQUIRE(bFlatSeven.notes == NoteSet({58, 62, 65, 68}, 4));

    spec.extension = ChordExtension::triad;
    const std::array<std::string, 7> expectedTriadLabels{"Cm", "Ddim", "D#", "Fm", "Gm", "G#", "A#"};
    for (int degree = 0; degree < 7; ++degree) {
        const auto triad = voicer.voiceChord(tonic, degree, spec, Scale::naturalMinor);
        REQUIRE(triad.label == expectedTriadLabels[static_cast<std::size_t>(degree)]);
    }
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
    auto chordDmaj = voicer.voiceChord(0, 1, specCustomMajor);
    REQUIRE(chordDmaj.label == "Dmaj7");
    REQUIRE(chordDmaj.notes.size() == 4);
    REQUIRE(chordDmaj.notes[0] == 50);
    REQUIRE(chordDmaj.notes[1] == 54);
    REQUIRE(chordDmaj.notes[2] == 57);
    REQUIRE(chordDmaj.notes[3] == 61);

    // Degree 0 (C) with QualityRule::dominant and seventh -> C3 (48), E3 (52), G3 (55), A#3/Bb3 (58) -> C7
    VoicingSpec specCustomDom{
        .extension = ChordExtension::seventh,
        .inversion = 0,
        .style = VoicingStyle::close,
        .baseOctave = 3,
        .qualityRule = QualityRule::dominant
    };
    auto chordCdom = voicer.voiceChord(0, 0, specCustomDom);
    REQUIRE(chordCdom.label == "C7");
    REQUIRE(chordCdom.notes.size() == 4);
    REQUIRE(chordCdom.notes[0] == 48);
    REQUIRE(chordCdom.notes[1] == 52);
    REQUIRE(chordCdom.notes[2] == 55);
    REQUIRE(chordCdom.notes[3] == 58);

    // Degree 0 (C) with QualityRule::diminished and seventh -> C3 (48), D#3 (51), F#3 (54), A3 (57) -> Cdim7
    VoicingSpec specCustomDim{
        .extension = ChordExtension::seventh,
        .inversion = 0,
        .style = VoicingStyle::close,
        .baseOctave = 3,
        .qualityRule = QualityRule::diminished
    };
    auto chordCdim = voicer.voiceChord(0, 0, specCustomDim);
    REQUIRE(chordCdim.label == "Cdim7");
    REQUIRE(chordCdim.notes.size() == 4);
    REQUIRE(chordCdim.notes[0] == 48);
    REQUIRE(chordCdim.notes[1] == 51);
    REQUIRE(chordCdim.notes[2] == 54);
    REQUIRE(chordCdim.notes[3] == 57);

    // QualityRule::diatonic on degree 1 (ii) should still be Dm7
    VoicingSpec specDiatonic{
        .extension = ChordExtension::seventh,
        .inversion = 0,
        .style = VoicingStyle::close,
        .baseOctave = 3,
        .qualityRule = QualityRule::diatonic
    };
    auto chordDm = voicer.voiceChord(0, 1, specDiatonic);
    REQUIRE(chordDm.label == "Dm7");
}

TEST_CASE("DiatonicChordVoicer generates 9th, 11th, 13th, add9, 6/9 and suspended tones", "[music][voicer][extensions]") {
    DiatonicChordVoicer voicer;

    SECTION("C major extended acceptance cases in root position") {
        // I ninth -> Cmaj9: C3 (48), E3 (52), G3 (55), B3 (59), D4 (62)
        VoicingSpec specCmaj9{
            .shape = ChordShape::ninth,
            .baseOctave = 3,
            .qualityRule = QualityRule::diatonic
        };
        auto chordCmaj9 = voicer.voiceChord(0, 0, specCmaj9);
        REQUIRE(chordCmaj9.label == "Cmaj9");
        REQUIRE(chordCmaj9.notes.size() == 5);
        REQUIRE(chordCmaj9.notes == NoteSet({48, 52, 55, 59, 62}, 5));

        // ii ninth -> Dm9: D3 (50), F3 (53), A3 (57), C4 (60), E4 (64)
        VoicingSpec specDm9{
            .shape = ChordShape::ninth,
            .baseOctave = 3,
            .qualityRule = QualityRule::diatonic
        };
        auto chordDm9 = voicer.voiceChord(0, 1, specDm9);
        REQUIRE(chordDm9.label == "Dm9");
        REQUIRE(chordDm9.notes.size() == 5);
        REQUIRE(chordDm9.notes == NoteSet({50, 53, 57, 60, 64}, 5));

        // V ninth -> G9: G3 (55), B3 (59), D4 (62), F4 (65), A4 (69)
        VoicingSpec specG9{
            .shape = ChordShape::ninth,
            .baseOctave = 3,
            .qualityRule = QualityRule::diatonic
        };
        auto chordG9 = voicer.voiceChord(0, 4, specG9);
        REQUIRE(chordG9.label == "G9");
        REQUIRE(chordG9.notes.size() == 5);
        REQUIRE(chordG9.notes == NoteSet({55, 59, 62, 65, 69}, 5));

        // V thirteenth -> G13: G3 (55), B3 (59), D4 (62), F4 (65), A4 (69), E5 (76)
        VoicingSpec specG13{
            .shape = ChordShape::thirteenth,
            .baseOctave = 3,
            .qualityRule = QualityRule::diatonic
        };
        auto chordG13 = voicer.voiceChord(0, 4, specG13);
        REQUIRE(chordG13.label == "G13");
        REQUIRE(chordG13.notes.size() == 6);
        REQUIRE(chordG13.notes == NoteSet({55, 59, 62, 65, 69, 76}, 6));

        // vi eleventh -> Am11: A3 (57), C4 (60), E4 (64), G4 (67), B4 (71), D5 (74)
        VoicingSpec specAm11{
            .shape = ChordShape::eleventh,
            .baseOctave = 3,
            .qualityRule = QualityRule::diatonic
        };
        auto chordAm11 = voicer.voiceChord(0, 5, specAm11);
        REQUIRE(chordAm11.label == "Am11");
        REQUIRE(chordAm11.notes.size() == 6);
        REQUIRE(chordAm11.notes == NoteSet({57, 60, 64, 67, 71, 74}, 6));

        // I add9 -> Cadd9: C3 (48), E3 (52), G3 (55), D4 (62)
        VoicingSpec specCadd9{
            .shape = ChordShape::add9,
            .baseOctave = 3,
            .qualityRule = QualityRule::diatonic
        };
        auto chordCadd9 = voicer.voiceChord(0, 0, specCadd9);
        REQUIRE(chordCadd9.label == "Cadd9");
        REQUIRE(chordCadd9.notes.size() == 4);
        REQUIRE(chordCadd9.notes == NoteSet({48, 52, 55, 62}, 4));

        // I 6/9 -> C6/9: C3 (48), E3 (52), G3 (55), A3 (57), D4 (62)
        VoicingSpec specC69{
            .shape = ChordShape::sixNine,
            .baseOctave = 3,
            .qualityRule = QualityRule::diatonic
        };
        auto chordC69 = voicer.voiceChord(0, 0, specC69);
        REQUIRE(chordC69.label == "C6/9");
        REQUIRE(chordC69.notes.size() == 5);
        REQUIRE(chordC69.notes == NoteSet({48, 52, 55, 57, 62}, 5));

        // I sus2 -> Csus2: C3 (48), D3 (50), G3 (55)
        VoicingSpec specCsus2{
            .shape = ChordShape::sus2,
            .baseOctave = 3,
            .qualityRule = QualityRule::diatonic
        };
        auto chordCsus2 = voicer.voiceChord(0, 0, specCsus2);
        REQUIRE(chordCsus2.label == "Csus2");
        REQUIRE(chordCsus2.notes.size() == 3);
        REQUIRE(chordCsus2.notes == NoteSet({48, 50, 55}, 3));

        // I sus4 -> Csus4: C3 (48), F3 (53), G3 (55)
        VoicingSpec specCsus4{
            .shape = ChordShape::sus4,
            .baseOctave = 3,
            .qualityRule = QualityRule::diatonic
        };
        auto chordCsus4 = voicer.voiceChord(0, 0, specCsus4);
        REQUIRE(chordCsus4.label == "Csus4");
        REQUIRE(chordCsus4.notes.size() == 3);
        REQUIRE(chordCsus4.notes == NoteSet({48, 53, 55}, 3));
    }

    SECTION("Natural minor extended chords") {
        // C natural minor:
        // i ninth -> Cm9: C3 (48), D#3 (51), G3 (55), A#3 (58), D4 (62)
        VoicingSpec specCm9{
            .shape = ChordShape::ninth,
            .baseOctave = 3,
            .qualityRule = QualityRule::diatonic
        };
        auto chordCm9 = voicer.voiceChord(0, 0, specCm9, Scale::naturalMinor);
        REQUIRE(chordCm9.label == "Cm9");
        REQUIRE(chordCm9.notes.size() == 5);
        REQUIRE(chordCm9.notes == NoteSet({48, 51, 55, 58, 62}, 5));

        // ii seventh -> Dm7b5: D3 (50), F3 (53), G#3 (56), C4 (60)
        VoicingSpec specDm7b5{
            .shape = ChordShape::seventh,
            .baseOctave = 3,
            .qualityRule = QualityRule::diatonic
        };
        auto chordDm7b5 = voicer.voiceChord(0, 1, specDm7b5, Scale::naturalMinor);
        REQUIRE(chordDm7b5.label == "Dm7b5");
        REQUIRE(chordDm7b5.notes.size() == 4);
        REQUIRE(chordDm7b5.notes == NoteSet({50, 53, 56, 60}, 4));
        // Upper MIDI bound check
        VoicingSpec specHighOctave{
            .shape = ChordShape::thirteenth,
            .baseOctave = 9,
            .qualityRule = QualityRule::diatonic
        };
        REQUIRE_THROWS_AS(voicer.voiceChord(0, 0, specHighOctave), std::out_of_range);
    }
}

TEST_CASE("DiatonicChordVoicer applies compact, open, rootless policies and fifth omission", "[music][voicer][style]") {
    DiatonicChordVoicer voicer;

    SECTION("Cmaj9 under compact, open, rootless and fifth policies") {
        // Cmaj9 base notes: C3 (48), E3 (52), G3 (55), B3 (59), D4 (62)
        // compact: [48, 52, 55, 59, 62]
        VoicingSpec specCompact{
            .shape = ChordShape::ninth,
            .style = VoicingStyle::compact,
            .fifthPolicy = FifthPolicy::automatic,
            .baseOctave = 3,
            .qualityRule = QualityRule::diatonic
        };
        auto cmaj9Compact = voicer.voiceChord(0, 0, specCompact);
        REQUIRE(cmaj9Compact.notes == NoteSet({48, 52, 55, 59, 62}, 5));

        // open (drop-2 from root position [48, 52, 55, 59, 62]: 2nd from top is B3 (59) -> B2 (47) -> [47, 48, 52, 55, 62]
        // or standard drop-2 / spread: [48, 55, 59, 62, 64] / drop 2nd highest down octave)
        VoicingSpec specOpen{
            .shape = ChordShape::ninth,
            .style = VoicingStyle::open,
            .fifthPolicy = FifthPolicy::automatic,
            .baseOctave = 3,
            .qualityRule = QualityRule::diatonic
        };
        auto cmaj9Open = voicer.voiceChord(0, 0, specOpen);
        REQUIRE(cmaj9Open.notes.size() >= 4);

        // rootless: omit root (C3=48), retain 3rd (52), 5th (55), 7th (59), 9th (62) -> [52, 55, 59, 62]
        VoicingSpec specRootless{
            .shape = ChordShape::ninth,
            .style = VoicingStyle::rootless,
            .fifthPolicy = FifthPolicy::automatic,
            .baseOctave = 3,
            .qualityRule = QualityRule::diatonic
        };
        auto cmaj9Rootless = voicer.voiceChord(0, 0, specRootless);
        REQUIRE(cmaj9Rootless.notes == NoteSet({52, 55, 59, 62}, 4));

        // rootless with fifthPolicy::omit: omit root (48) AND fifth (55) -> [52, 59, 62]
        VoicingSpec specRootlessNo5th{
            .shape = ChordShape::ninth,
            .style = VoicingStyle::rootless,
            .fifthPolicy = FifthPolicy::omit,
            .baseOctave = 3,
            .qualityRule = QualityRule::diatonic
        };
        auto cmaj9RootlessNo5 = voicer.voiceChord(0, 0, specRootlessNo5th);
        REQUIRE(cmaj9RootlessNo5.notes == NoteSet({52, 59, 62}, 3));

        // compact with fifthPolicy::omit: omit fifth (55) -> [48, 52, 59, 62]
        VoicingSpec specCompactNo5th{
            .shape = ChordShape::ninth,
            .style = VoicingStyle::compact,
            .fifthPolicy = FifthPolicy::omit,
            .baseOctave = 3,
            .qualityRule = QualityRule::diatonic
        };
        auto cmaj9CompactNo5 = voicer.voiceChord(0, 0, specCompactNo5th);
        REQUIRE(cmaj9CompactNo5.notes == NoteSet({48, 52, 59, 62}, 4));
    }

    SECTION("Dm9 under compact, open and rootless") {
        // Dm9: D3 (50), F3 (53), A3 (57), C4 (60), E4 (64)
        VoicingSpec specDm9Rootless{
            .shape = ChordShape::ninth,
            .style = VoicingStyle::rootless,
            .fifthPolicy = FifthPolicy::automatic,
            .baseOctave = 3,
            .qualityRule = QualityRule::diatonic
        };
        auto dm9Rootless = voicer.voiceChord(0, 1, specDm9Rootless);
        // omit root (50) -> [53, 57, 60, 64]
        REQUIRE(dm9Rootless.notes == NoteSet({53, 57, 60, 64}, 4));

        // Dm9 with fifthPolicy::omit -> omit A3 (57) -> [50, 53, 60, 64]
        VoicingSpec specDm9No5th{
            .shape = ChordShape::ninth,
            .style = VoicingStyle::compact,
            .fifthPolicy = FifthPolicy::omit,
            .baseOctave = 3,
            .qualityRule = QualityRule::diatonic
        };
        auto dm9No5 = voicer.voiceChord(0, 1, specDm9No5th);
        REQUIRE(dm9No5.notes == NoteSet({50, 53, 60, 64}, 4));
    }

    SECTION("G13 under compact, open, rootless and fifth policy") {
        // G13 base: G3 (55), B3 (59), D4 (62), F4 (65), A4 (69), E5 (76) (11th omitted by 6-voice capacity)
        VoicingSpec specG13Compact{
            .shape = ChordShape::thirteenth,
            .style = VoicingStyle::compact,
            .fifthPolicy = FifthPolicy::include,
            .baseOctave = 3,
            .qualityRule = QualityRule::diatonic
        };
        auto g13Compact = voicer.voiceChord(0, 4, specG13Compact);
        REQUIRE(g13Compact.notes == NoteSet({55, 59, 62, 65, 69, 76}, 6));

        // G13 rootless: omit root G3 (55) -> B3 (59), D4 (62), F4 (65), A4 (69), E5 (76) -> 5 notes
        VoicingSpec specG13Rootless{
            .shape = ChordShape::thirteenth,
            .style = VoicingStyle::rootless,
            .fifthPolicy = FifthPolicy::automatic,
            .baseOctave = 3,
            .qualityRule = QualityRule::diatonic
        };
        auto g13Rootless = voicer.voiceChord(0, 4, specG13Rootless);
        REQUIRE(g13Rootless.notes == NoteSet({59, 62, 65, 69, 76}, 5));

        // G13 omit 5th: omit D4 (62) -> G3 (55), B3 (59), F4 (65), A4 (69), E5 (76) -> 5 notes
        VoicingSpec specG13No5th{
            .shape = ChordShape::thirteenth,
            .style = VoicingStyle::compact,
            .fifthPolicy = FifthPolicy::omit,
            .baseOctave = 3,
            .qualityRule = QualityRule::diatonic
        };
        auto g13No5 = voicer.voiceChord(0, 4, specG13No5th);
        REQUIRE(g13No5.notes == NoteSet({55, 59, 65, 69, 76}, 5));
    }

    SECTION("Am11 under compact and rootless") {
        // Am11 base: A3 (57), C4 (60), E4 (64), G4 (67), B4 (71), D5 (74)
        VoicingSpec specAm11Rootless{
            .shape = ChordShape::eleventh,
            .style = VoicingStyle::rootless,
            .fifthPolicy = FifthPolicy::automatic,
            .baseOctave = 3,
            .qualityRule = QualityRule::diatonic
        };
        auto am11Rootless = voicer.voiceChord(0, 5, specAm11Rootless);
        // omit root A3 (57) -> [60, 64, 67, 71, 74]
        REQUIRE(am11Rootless.notes == NoteSet({60, 64, 67, 71, 74}, 5));
    }

    SECTION("Triad under rootless falls back to compact") {
        VoicingSpec specTriadRootless{
            .shape = ChordShape::triad,
            .style = VoicingStyle::rootless,
            .baseOctave = 3,
            .qualityRule = QualityRule::diatonic
        };
        auto triad = voicer.voiceChord(0, 0, specTriadRootless);
        REQUIRE(triad.notes == NoteSet({48, 52, 55}, 3));
    }

    SECTION("FifthPolicy automatic behavior") {
        // For triads/sus: Fifth is included
        VoicingSpec specTriadAuto{
            .shape = ChordShape::triad,
            .fifthPolicy = FifthPolicy::automatic,
            .baseOctave = 3,
            .qualityRule = QualityRule::diatonic
        };
        auto triadAuto = voicer.voiceChord(0, 0, specTriadAuto);
        REQUIRE(triadAuto.notes == NoteSet({48, 52, 55}, 3));

        VoicingSpec specSus4Auto{
            .shape = ChordShape::sus4,
            .fifthPolicy = FifthPolicy::automatic,
            .baseOctave = 3,
            .qualityRule = QualityRule::diatonic
        };
        auto sus4Auto = voicer.voiceChord(0, 0, specSus4Auto);
        REQUIRE(sus4Auto.notes == NoteSet({48, 53, 55}, 3));
    }
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
