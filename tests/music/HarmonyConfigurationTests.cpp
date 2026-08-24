#include <catch2/catch_test_macros.hpp>
#include <music/HarmonyConfiguration.h>
#include <music/DiatonicChordVoicer.h>

using namespace chordsynth::music;

TEST_CASE("HarmonyConfiguration default initialization produces diatonic triads", "[music][harmony]") {
    HarmonyConfiguration config;

    for (int deg = 0; deg < 7; ++deg) {
        const auto spec = config.getSpec(deg);
        REQUIRE(spec.shape == ChordShape::triad);
        REQUIRE(spec.extension == ChordExtension::triad);
        REQUIRE(spec.style == VoicingStyle::compact);
        REQUIRE(spec.fifthPolicy == FifthPolicy::include);
        REQUIRE(spec.bassMode == BassMode::none);
        REQUIRE(spec.voiceLeading == VoiceLeadingMode::manual);
        REQUIRE(spec.inversion == 0);
        REQUIRE(spec.baseOctave == 3);
        REQUIRE(spec.qualityRule == QualityRule::diatonic);
    }
}

TEST_CASE("HarmonyConfiguration default specs voice cleanly in C Major with DiatonicChordVoicer", "[music][harmony]") {
    DiatonicChordVoicer voicer;
    HarmonyConfiguration config;

    SECTION("Diatonic default: Root close triads in C Major without bass") {
        // I -> C [48, 52, 55] (C3, E3, G3), "C"
        auto v0 = voicer.voiceChord(0, 0, config.getSpec(0), Scale::major);
        REQUIRE(v0.label == "C");
        REQUIRE_FALSE(v0.bassMidi.has_value());
        REQUIRE(v0.notes.size() == 3);
        REQUIRE(v0.notes[0] == 48);
        REQUIRE(v0.notes[1] == 52);
        REQUIRE(v0.notes[2] == 55);

        // V -> G [55, 59, 62] (G3, B3, D4), "G"
        auto v4 = voicer.voiceChord(0, 4, config.getSpec(4), Scale::major);
        REQUIRE(v4.label == "G");
        REQUIRE_FALSE(v4.bassMidi.has_value());
        REQUIRE(v4.notes.size() == 3);
        REQUIRE(v4.notes[0] == 55);
        REQUIRE(v4.notes[1] == 59);
        REQUIRE(v4.notes[2] == 62);
    }

    SECTION("Sevenths factory: Diatonic sevenths in C Major without bass") {
        auto seventhsConfig = HarmonyConfiguration::makeSevenths();

        // I -> Cmaj7 [48, 52, 55, 59], "Cmaj7"
        auto v0 = voicer.voiceChord(0, 0, seventhsConfig.getSpec(0), Scale::major);
        REQUIRE(v0.label == "Cmaj7");
        REQUIRE_FALSE(v0.bassMidi.has_value());
        REQUIRE(v0.notes.size() == 4);
        REQUIRE(v0.notes[0] == 48);
        REQUIRE(v0.notes[1] == 52);
        REQUIRE(v0.notes[2] == 55);
        REQUIRE(v0.notes[3] == 59);

        // V -> G7 [55, 59, 62, 65], "G7"
        auto v4 = voicer.voiceChord(0, 4, seventhsConfig.getSpec(4), Scale::major);
        REQUIRE(v4.label == "G7");
        REQUIRE_FALSE(v4.bassMidi.has_value());
        REQUIRE(v4.notes.size() == 4);
        REQUIRE(v4.notes[0] == 55);
        REQUIRE(v4.notes[1] == 59);
        REQUIRE(v4.notes[2] == 62);
        REQUIRE(v4.notes[3] == 65);
    }

    SECTION("Lo-Fi Warm factory: Extended open chords with root bass") {
        auto lofiConfig = HarmonyConfiguration::makeLofiWarm();

        // I -> Cmaj9 open with root bass (C2 = 36)
        auto v0 = voicer.voiceChord(0, 0, lofiConfig.getSpec(0), Scale::major);
        REQUIRE(v0.label == "Cmaj9");
        REQUIRE(v0.bassMidi.has_value());
        REQUIRE(v0.bassMidi.value() == 36); // C2
        // Cmaj9 open drop-2 transposed into safe register (52..84): [59, 60, 64, 67, 74]
        REQUIRE(v0.notes.size() == 5);
        REQUIRE(v0.notes[0] == 59);
        REQUIRE(v0.notes[1] == 60);
        REQUIRE(v0.notes[2] == 64);
        REQUIRE(v0.notes[3] == 67);
        REQUIRE(v0.notes[4] == 74);

        // V -> G13 open with root bass (G2 = 43)
        auto v4 = voicer.voiceChord(0, 4, lofiConfig.getSpec(4), Scale::major);
        REQUIRE(v4.label == "G13");
        REQUIRE(v4.bassMidi.has_value());
        REQUIRE(v4.bassMidi.value() == 43); // G2
    }

    SECTION("Jazz Tension factory: Extended rootless chords with root bass") {
        auto jazzConfig = HarmonyConfiguration::makeJazzTension();

        // I -> C6/9 rootless with root bass (C2 = 36)
        auto v0 = voicer.voiceChord(0, 0, jazzConfig.getSpec(0), Scale::major);
        REQUIRE(v0.label == "C6/9");
        REQUIRE(v0.bassMidi.has_value());
        REQUIRE(v0.bassMidi.value() == 36); // C2
        // C6/9 tones: C(48), E(52), G(55), A(57), D(62). Rootless omits C(48) -> [52, 55, 57, 62]
        REQUIRE(v0.notes.size() == 4);
        REQUIRE(v0.notes[0] == 52);
        REQUIRE(v0.notes[1] == 55);
        REQUIRE(v0.notes[2] == 57);
        REQUIRE(v0.notes[3] == 62);

        // ii -> Dm11 rootless with root bass (D2 = 38)
        auto v1 = voicer.voiceChord(0, 1, jazzConfig.getSpec(1), Scale::major);
        REQUIRE(v1.label == "Dm11");
        REQUIRE(v1.bassMidi.has_value());
        REQUIRE(v1.bassMidi.value() == 38); // D2
    }
}

TEST_CASE("HarmonyConfiguration supports localized per-degree editing", "[music][harmony]") {
    HarmonyConfiguration config;

    VoicingSpec customSpec{
        .shape = ChordShape::add9,
        .extension = ChordExtension::seventh,
        .inversion = 2,
        .style = VoicingStyle::open,
        .fifthPolicy = FifthPolicy::omit,
        .bassMode = BassMode::slashDegree,
        .slashDegree = 4,
        .voiceLeading = VoiceLeadingMode::manual,
        .baseOctave = 4,
        .qualityRule = QualityRule::major
    };

    // Modify degree 1 (ii)
    REQUIRE(config.setSpec(1, customSpec));

    // Verify modified cell
    REQUIRE(config.getSpec(1) == customSpec);

    // Verify other cells remain untouched
    // Degree 0 (I) -> triad, compact, fifth included, no bass, manual leading
    REQUIRE(config.getSpec(0).shape == ChordShape::triad);
    REQUIRE(config.getSpec(0).inversion == 0);
    REQUIRE(config.getSpec(0).qualityRule == QualityRule::diatonic);

    // resetDegree restores default of that cell
    REQUIRE(config.resetDegree(1));
    REQUIRE(config.getSpec(1).shape == ChordShape::triad);
    REQUIRE(config.getSpec(1).extension == ChordExtension::triad);
    REQUIRE(config.getSpec(1).inversion == 0);
    REQUIRE(config.getSpec(1).style == VoicingStyle::compact);
    REQUIRE(config.getSpec(1).fifthPolicy == FifthPolicy::include);
    REQUIRE(config.getSpec(1).bassMode == BassMode::none);
    REQUIRE(config.getSpec(1).voiceLeading == VoiceLeadingMode::manual);
    REQUIRE(config.getSpec(1).baseOctave == 3);
    REQUIRE(config.getSpec(1).qualityRule == QualityRule::diatonic);
}

TEST_CASE("HarmonyConfiguration validates bounds and sanitizes inputs", "[music][harmony]") {
    HarmonyConfiguration config;

    // Out of bounds degree index (< 0 or > 6)
    REQUIRE_FALSE(config.isValidDegree(-1));
    REQUIRE_FALSE(config.isValidDegree(7));
    REQUIRE(config.isValidDegree(0));
    REQUIRE(config.isValidDegree(6));

    // getSpec on invalid degree returns a safe fallback
    auto fallback = config.getSpec(99);
    REQUIRE(fallback.shape == ChordShape::triad);
    REQUIRE(fallback.extension == ChordExtension::triad);
    REQUIRE(fallback.baseOctave == 3);

    // setSpec on invalid degree returns false without modifying internal state
    VoicingSpec badSpec{
        .shape = ChordShape::seventh,
        .extension = ChordExtension::seventh,
        .inversion = 0,
        .style = VoicingStyle::compact,
        .baseOctave = 3,
        .qualityRule = QualityRule::diatonic
    };
    REQUIRE_FALSE(config.setSpec(-1, badSpec));
    REQUIRE_FALSE(config.setSpec(7, badSpec));

    // setSpec with out-of-range baseOctave (< 2 or > 4), inversion (< 0 or > 5), slashDegree (< 0 or > 6)
    VoicingSpec specHighRanges{
        .shape = ChordShape::triad,
        .extension = ChordExtension::triad,
        .inversion = 10,
        .style = VoicingStyle::compact,
        .bassMode = BassMode::slashDegree,
        .slashDegree = 15,
        .baseOctave = 8,
        .qualityRule = QualityRule::diatonic
    };
    REQUIRE(config.setSpec(0, specHighRanges));
    auto stored = config.getSpec(0);
    REQUIRE(stored.baseOctave == 4);     // clamped to max 4
    REQUIRE(stored.inversion == 5);      // clamped to max 5 (safe upper inversion bound)
    REQUIRE(stored.slashDegree == 6);    // clamped to max 6

    VoicingSpec specLowRanges{
        .shape = ChordShape::triad,
        .extension = ChordExtension::triad,
        .inversion = -3,
        .style = VoicingStyle::compact,
        .bassMode = BassMode::slashDegree,
        .slashDegree = -5,
        .baseOctave = 0,
        .qualityRule = QualityRule::diatonic
    };
    REQUIRE(config.setSpec(0, specLowRanges));
    auto storedLow = config.getSpec(0);
    REQUIRE(storedLow.baseOctave == 2);   // clamped to min 2
    REQUIRE(storedLow.inversion == 0);    // clamped to min 0
    REQUIRE(storedLow.slashDegree == 0);  // clamped to min 0

    // resetAll restores all defaults
    VoicingSpec customSpec{
        .shape = ChordShape::seventh,
        .extension = ChordExtension::seventh,
        .inversion = 1,
        .style = VoicingStyle::open,
        .baseOctave = 4,
        .qualityRule = QualityRule::major
    };
    config.setSpec(0, customSpec);
    config.resetAll();
    REQUIRE(config.getSpec(0).shape == ChordShape::triad);
    REQUIRE(config.getSpec(0).extension == ChordExtension::triad);
    REQUIRE(config.getSpec(0).inversion == 0);
    REQUIRE(config.getSpec(0).style == VoicingStyle::compact);
    REQUIRE(config.getSpec(0).fifthPolicy == FifthPolicy::include);
    REQUIRE(config.getSpec(0).bassMode == BassMode::none);
    REQUIRE(config.getSpec(0).voiceLeading == VoiceLeadingMode::manual);
}
