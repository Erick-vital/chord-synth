#include <catch2/catch_test_macros.hpp>
#include <music/ChordRecipe.h>
#include <music/VoicedChord.h>

using namespace chordsynth::music;

TEST_CASE("ChordRecipe default construction", "[music][recipe]") {
    ChordRecipe recipe{};
    REQUIRE(recipe.quality == ResolvedQuality::major);
    REQUIRE(recipe.seventh == SeventhKind::none);
    REQUIRE_FALSE(recipe.includeSixth);
    REQUIRE_FALSE(recipe.includeNinth);
    REQUIRE_FALSE(recipe.includeEleventh);
    REQUIRE_FALSE(recipe.includeThirteenth);
    REQUIRE_FALSE(recipe.sus2);
    REQUIRE_FALSE(recipe.sus4);
}

TEST_CASE("ChordRecipe factories for basic shapes", "[music][recipe]") {
    SECTION("triad") {
        auto recipe = createChordRecipe(ChordShape::triad, ResolvedQuality::major);
        REQUIRE(recipe.quality == ResolvedQuality::major);
        REQUIRE(recipe.seventh == SeventhKind::none);
        REQUIRE_FALSE(recipe.includeSixth);
        REQUIRE_FALSE(recipe.includeNinth);
        REQUIRE_FALSE(recipe.includeEleventh);
        REQUIRE_FALSE(recipe.includeThirteenth);
        REQUIRE_FALSE(recipe.sus2);
        REQUIRE_FALSE(recipe.sus4);
    }

    SECTION("seventh") {
        auto recipe = createChordRecipe(ChordShape::seventh, ResolvedQuality::major, SeventhKind::major);
        REQUIRE(recipe.quality == ResolvedQuality::major);
        REQUIRE(recipe.seventh == SeventhKind::major);
        REQUIRE_FALSE(recipe.includeSixth);
        REQUIRE_FALSE(recipe.includeNinth);
        REQUIRE_FALSE(recipe.includeEleventh);
        REQUIRE_FALSE(recipe.includeThirteenth);
    }

    SECTION("ninth") {
        auto recipe = createChordRecipe(ChordShape::ninth, ResolvedQuality::major, SeventhKind::major);
        REQUIRE(recipe.quality == ResolvedQuality::major);
        REQUIRE(recipe.seventh == SeventhKind::major);
        REQUIRE(recipe.includeNinth);
        REQUIRE_FALSE(recipe.includeEleventh);
        REQUIRE_FALSE(recipe.includeThirteenth);
        REQUIRE_FALSE(recipe.includeSixth);
    }

    SECTION("eleventh") {
        auto recipe = createChordRecipe(ChordShape::eleventh, ResolvedQuality::minor, SeventhKind::minor);
        REQUIRE(recipe.quality == ResolvedQuality::minor);
        REQUIRE(recipe.seventh == SeventhKind::minor);
        REQUIRE(recipe.includeNinth);
        REQUIRE(recipe.includeEleventh);
        REQUIRE_FALSE(recipe.includeThirteenth);
        REQUIRE_FALSE(recipe.includeSixth);
    }

    SECTION("thirteenth") {
        auto recipe = createChordRecipe(ChordShape::thirteenth, ResolvedQuality::dominant, SeventhKind::minor);
        REQUIRE(recipe.quality == ResolvedQuality::dominant);
        REQUIRE(recipe.seventh == SeventhKind::minor);
        REQUIRE(recipe.includeNinth);
        REQUIRE(recipe.includeEleventh);
        REQUIRE(recipe.includeThirteenth);
        REQUIRE_FALSE(recipe.includeSixth);
    }

    SECTION("add9") {
        auto recipe = createChordRecipe(ChordShape::add9, ResolvedQuality::major);
        REQUIRE(recipe.quality == ResolvedQuality::major);
        REQUIRE(recipe.seventh == SeventhKind::none);
        REQUIRE(recipe.includeNinth);
        REQUIRE_FALSE(recipe.includeEleventh);
        REQUIRE_FALSE(recipe.includeThirteenth);
        REQUIRE_FALSE(recipe.includeSixth);
    }

    SECTION("sixNine") {
        auto recipe = createChordRecipe(ChordShape::sixNine, ResolvedQuality::major);
        REQUIRE(recipe.quality == ResolvedQuality::major);
        REQUIRE(recipe.seventh == SeventhKind::none); // 6/9 has no seventh
        REQUIRE(recipe.includeSixth);
        REQUIRE(recipe.includeNinth);
        REQUIRE_FALSE(recipe.includeEleventh);
        REQUIRE_FALSE(recipe.includeThirteenth);
    }

    SECTION("sus2") {
        auto recipe = createChordRecipe(ChordShape::sus2, ResolvedQuality::major);
        REQUIRE(recipe.sus2);
        REQUIRE_FALSE(recipe.sus4);
        REQUIRE(recipe.seventh == SeventhKind::none);
        REQUIRE_FALSE(recipe.includeSixth);
        REQUIRE_FALSE(recipe.includeNinth);
    }

    SECTION("sus4") {
        auto recipe = createChordRecipe(ChordShape::sus4, ResolvedQuality::major);
        REQUIRE(recipe.sus4);
        REQUIRE_FALSE(recipe.sus2);
        REQUIRE(recipe.seventh == SeventhKind::none);
        REQUIRE_FALSE(recipe.includeSixth);
        REQUIRE_FALSE(recipe.includeNinth);
    }
}

TEST_CASE("ChordRecipe sanitization ensures mutual exclusivity of sus2 and sus4", "[music][recipe]") {
    ChordRecipe r{};
    r.sus2 = true;
    r.sus4 = true;
    auto sanitized = sanitizeChordRecipe(r);
    // When both are set, sus2 takes precedence or sus4 is cleared
    REQUIRE_FALSE((sanitized.sus2 && sanitized.sus4));
}

TEST_CASE("VoicingSpec extended fields and defaults", "[music][voicing]") {
    VoicingSpec spec{};
    REQUIRE(spec.shape == ChordShape::triad);
    REQUIRE(spec.extension == ChordExtension::triad);
    REQUIRE(spec.inversion == 0);
    REQUIRE(spec.style == VoicingStyle::close);
    REQUIRE(spec.fifthPolicy == FifthPolicy::automatic);
    REQUIRE(spec.bassMode == BassMode::none);
    REQUIRE(spec.slashDegree == 0);
    REQUIRE(spec.voiceLeading == VoiceLeadingMode::manual);
    REQUIRE(spec.baseOctave == 3);
    REQUIRE(spec.qualityRule == QualityRule::diatonic);

    REQUIRE(static_cast<uint8_t>(QualityRule::dominant) == 4);
}
