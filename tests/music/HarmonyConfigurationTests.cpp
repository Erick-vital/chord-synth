#include <catch2/catch_test_macros.hpp>
#include <music/HarmonyConfiguration.h>

using namespace chordsynth::music;

TEST_CASE("HarmonyConfiguration initializes with default 4 scenes x 7 degrees matrix", "[music][harmony]") {
    HarmonyConfiguration config;

    // Scene A (0): triad, root (inversion 0), close, octave 3, diatonic
    for (int deg = 0; deg < 7; ++deg) {
        const auto spec = config.getSpec(0, deg);
        REQUIRE(spec.extension == ChordExtension::triad);
        REQUIRE(spec.inversion == 0);
        REQUIRE(spec.style == VoicingStyle::close);
        REQUIRE(spec.baseOctave == 3);
        REQUIRE(spec.qualityRule == QualityRule::diatonic);
    }

    // Scene B (1): seventh, root (inversion 0), close, octave 3, diatonic
    for (int deg = 0; deg < 7; ++deg) {
        const auto spec = config.getSpec(1, deg);
        REQUIRE(spec.extension == ChordExtension::seventh);
        REQUIRE(spec.inversion == 0);
        REQUIRE(spec.style == VoicingStyle::close);
        REQUIRE(spec.baseOctave == 3);
        REQUIRE(spec.qualityRule == QualityRule::diatonic);
    }

    // Scene C (2): triad, root (inversion 0), open, octave 3, diatonic
    for (int deg = 0; deg < 7; ++deg) {
        const auto spec = config.getSpec(2, deg);
        REQUIRE(spec.extension == ChordExtension::triad);
        REQUIRE(spec.inversion == 0);
        REQUIRE(spec.style == VoicingStyle::open);
        REQUIRE(spec.baseOctave == 3);
        REQUIRE(spec.qualityRule == QualityRule::diatonic);
    }

    // Scene D (3): triad, first-inversion (1), close, octave 3, diatonic
    for (int deg = 0; deg < 7; ++deg) {
        const auto spec = config.getSpec(3, deg);
        REQUIRE(spec.extension == ChordExtension::triad);
        REQUIRE(spec.inversion == 1);
        REQUIRE(spec.style == VoicingStyle::close);
        REQUIRE(spec.baseOctave == 3);
        REQUIRE(spec.qualityRule == QualityRule::diatonic);
    }
}

TEST_CASE("HarmonyConfiguration supports localized per-scene per-degree editing", "[music][harmony]") {
    HarmonyConfiguration config;

    VoicingSpec customSpec{
        .extension = ChordExtension::seventh,
        .inversion = 2,
        .style = VoicingStyle::open,
        .baseOctave = 4,
        .qualityRule = QualityRule::major
    };

    // Modify scene B (1), degree 1 (ii)
    REQUIRE(config.setSpec(1, 1, customSpec));

    // Verify modified cell
    REQUIRE(config.getSpec(1, 1) == customSpec);

    // Verify other cells remain untouched
    // Scene B, degree 0 (I) -> seventh, root, close, octave 3, diatonic
    REQUIRE(config.getSpec(1, 0).extension == ChordExtension::seventh);
    REQUIRE(config.getSpec(1, 0).inversion == 0);
    REQUIRE(config.getSpec(1, 0).qualityRule == QualityRule::diatonic);

    // Scene A, degree 1 (ii) -> triad, root, close, octave 3, diatonic
    REQUIRE(config.getSpec(0, 1).extension == ChordExtension::triad);
    REQUIRE(config.getSpec(0, 1).inversion == 0);
    REQUIRE(config.getSpec(0, 1).qualityRule == QualityRule::diatonic);

    // resetDegree restores default of that cell
    REQUIRE(config.resetDegree(1, 1));
    REQUIRE(config.getSpec(1, 1).extension == ChordExtension::seventh);
    REQUIRE(config.getSpec(1, 1).inversion == 0);
    REQUIRE(config.getSpec(1, 1).style == VoicingStyle::close);
    REQUIRE(config.getSpec(1, 1).baseOctave == 3);
    REQUIRE(config.getSpec(1, 1).qualityRule == QualityRule::diatonic);
}

TEST_CASE("HarmonyConfiguration validates bounds and sanitizes inputs", "[music][harmony]") {
    HarmonyConfiguration config;

    // Out of bounds scene index (< 0 or > 3)
    REQUIRE_FALSE(config.isValidScene(-1));
    REQUIRE_FALSE(config.isValidScene(4));
    REQUIRE(config.isValidScene(0));
    REQUIRE(config.isValidScene(3));

    // Out of bounds degree index (< 0 or > 6)
    REQUIRE_FALSE(config.isValidDegree(-1));
    REQUIRE_FALSE(config.isValidDegree(7));
    REQUIRE(config.isValidDegree(0));
    REQUIRE(config.isValidDegree(6));

    // getSpec on invalid scene or degree returns a safe fallback (Scene A default)
    auto fallback = config.getSpec(99, 99);
    REQUIRE(fallback.extension == ChordExtension::triad);
    REQUIRE(fallback.baseOctave == 3);

    // setSpec on invalid scene or degree returns false without modifying internal state
    VoicingSpec badSpec{
        .extension = ChordExtension::seventh,
        .inversion = 0,
        .style = VoicingStyle::close,
        .baseOctave = 3,
        .qualityRule = QualityRule::diatonic
    };
    REQUIRE_FALSE(config.setSpec(-1, 0, badSpec));
    REQUIRE_FALSE(config.setSpec(0, 7, badSpec));

    // setSpec with out-of-range baseOctave (< 2 or > 4) or out-of-range inversion clamps cleanly or validates
    VoicingSpec specHighOctave{
        .extension = ChordExtension::triad,
        .inversion = 5,
        .style = VoicingStyle::close,
        .baseOctave = 8,
        .qualityRule = QualityRule::diatonic
    };
    REQUIRE(config.setSpec(0, 0, specHighOctave));
    auto stored = config.getSpec(0, 0);
    REQUIRE(stored.baseOctave == 4); // clamped to max 4
    REQUIRE(stored.inversion == 2);  // clamped to max 2 for triad

    VoicingSpec specLowOctave{
        .extension = ChordExtension::triad,
        .inversion = -3,
        .style = VoicingStyle::close,
        .baseOctave = 0,
        .qualityRule = QualityRule::diatonic
    };
    REQUIRE(config.setSpec(0, 0, specLowOctave));
    auto storedLow = config.getSpec(0, 0);
    REQUIRE(storedLow.baseOctave == 2); // clamped to min 2
    REQUIRE(storedLow.inversion == 0);  // clamped to min 0

    // resetAll restores all defaults across all scenes
    VoicingSpec customSpec{
        .extension = ChordExtension::seventh,
        .inversion = 1,
        .style = VoicingStyle::open,
        .baseOctave = 4,
        .qualityRule = QualityRule::major
    };
    config.setSpec(0, 0, customSpec);
    config.resetAll();
    REQUIRE(config.getSpec(0, 0).extension == ChordExtension::triad);
    REQUIRE(config.getSpec(0, 0).inversion == 0);
}
