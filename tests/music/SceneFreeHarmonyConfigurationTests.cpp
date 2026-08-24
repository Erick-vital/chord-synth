#include <catch2/catch_test_macros.hpp>
#include <music/HarmonyConfiguration.h>
#include <music/DiatonicChordVoicer.h>

using namespace chordsynth::music;

TEST_CASE("SceneFreeHarmonyConfiguration single configuration and factories", "[music][harmony]") {
    SECTION("Single 7-degree configuration getter and setter API") {
        HarmonyConfiguration config;

        VoicingSpec replacement{
            .shape = ChordShape::ninth,
            .extension = ChordExtension::seventh,
            .inversion = 2,
            .style = VoicingStyle::open,
            .fifthPolicy = FifthPolicy::omit,
            .bassMode = BassMode::slashDegree,
            .slashDegree = 4,
            .voiceLeading = VoiceLeadingMode::nearest,
            .baseOctave = 4,
            .qualityRule = QualityRule::major
        };

        REQUIRE(config.setSpec(2, replacement));
        REQUIRE(config.getSpec(2) == replacement);

        // Out of bounds checks
        REQUIRE_FALSE(config.setSpec(7, replacement));
        REQUIRE_FALSE(config.setSpec(-1, replacement));
        REQUIRE_FALSE(config.isValidDegree(-1));
        REQUIRE_FALSE(config.isValidDegree(7));
        REQUIRE(config.isValidDegree(0));
        REQUIRE(config.isValidDegree(6));

        // getSpec out of bounds returns safe fallback
        auto fallback = config.getSpec(99);
        REQUIRE(fallback.shape == ChordShape::triad);
        REQUIRE(fallback.baseOctave == 3);
    }

    SECTION("makeDiatonic factory produces diatonic compact triads") {
        auto config = HarmonyConfiguration::makeDiatonic();
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

    SECTION("makeSevenths factory produces diatonic compact sevenths") {
        auto config = HarmonyConfiguration::makeSevenths();
        for (int deg = 0; deg < 7; ++deg) {
            const auto spec = config.getSpec(deg);
            REQUIRE(spec.shape == ChordShape::seventh);
            REQUIRE(spec.extension == ChordExtension::seventh);
            REQUIRE(spec.style == VoicingStyle::compact);
            REQUIRE(spec.fifthPolicy == FifthPolicy::automatic);
            REQUIRE(spec.bassMode == BassMode::none);
            REQUIRE(spec.voiceLeading == VoiceLeadingMode::nearest);
            REQUIRE(spec.inversion == 0);
            REQUIRE(spec.baseOctave == 3);
            REQUIRE(spec.qualityRule == QualityRule::diatonic);
        }
    }

    SECTION("makeLofiWarm factory produces open extended chords with root bass") {
        auto config = HarmonyConfiguration::makeLofiWarm();
        const std::array<ChordShape, 7> expectedShapes = {
            ChordShape::ninth,       // I
            ChordShape::ninth,       // ii
            ChordShape::seventh,     // iii
            ChordShape::ninth,       // IV
            ChordShape::thirteenth,  // V
            ChordShape::ninth,       // vi
            ChordShape::seventh      // vii
        };

        for (int deg = 0; deg < 7; ++deg) {
            const auto spec = config.getSpec(deg);
            REQUIRE(spec.shape == expectedShapes[static_cast<std::size_t>(deg)]);
            REQUIRE(spec.style == VoicingStyle::open);
            REQUIRE(spec.fifthPolicy == FifthPolicy::automatic);
            REQUIRE(spec.bassMode == BassMode::root);
            REQUIRE(spec.voiceLeading == VoiceLeadingMode::nearest);
            REQUIRE(spec.inversion == 0);
            REQUIRE(spec.baseOctave == 3);
            REQUIRE(spec.qualityRule == QualityRule::diatonic);
        }
    }

    SECTION("makeJazzTension factory produces rootless extended chords with root bass") {
        auto config = HarmonyConfiguration::makeJazzTension();
        const std::array<ChordShape, 7> expectedShapes = {
            ChordShape::sixNine,     // I
            ChordShape::eleventh,    // ii
            ChordShape::ninth,       // iii
            ChordShape::ninth,       // IV
            ChordShape::thirteenth,  // V
            ChordShape::eleventh,    // vi
            ChordShape::seventh      // vii
        };

        for (int deg = 0; deg < 7; ++deg) {
            const auto spec = config.getSpec(deg);
            REQUIRE(spec.shape == expectedShapes[static_cast<std::size_t>(deg)]);
            REQUIRE(spec.style == VoicingStyle::rootless);
            REQUIRE(spec.fifthPolicy == FifthPolicy::automatic);
            REQUIRE(spec.bassMode == BassMode::root);
            REQUIRE(spec.voiceLeading == VoiceLeadingMode::nearest);
            REQUIRE(spec.inversion == 0);
            REQUIRE(spec.baseOctave == 3);
            REQUIRE(spec.qualityRule == QualityRule::diatonic);
        }
    }
}
