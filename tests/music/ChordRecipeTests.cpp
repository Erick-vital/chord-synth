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

TEST_CASE("resolveChordRecipe resolves diatonic and free chord identities", "[music][recipe]") {
    SECTION("C major diatonic jazz chords") {
        // Degree 0 (I) -> Cmaj9
        auto recI9 = resolveChordRecipe(Scale::major, 0, ChordShape::ninth, QualityRule::diatonic);
        REQUIRE(recI9.quality == ResolvedQuality::major);
        REQUIRE(recI9.seventh == SeventhKind::major);
        REQUIRE(recI9.includeNinth);
        REQUIRE(resolveChordLabel(0, recI9, ChordShape::ninth) == "Cmaj9");

        // Degree 1 (ii) -> Dm9
        auto recII9 = resolveChordRecipe(Scale::major, 1, ChordShape::ninth, QualityRule::diatonic);
        REQUIRE(recII9.quality == ResolvedQuality::minor);
        REQUIRE(recII9.seventh == SeventhKind::minor);
        REQUIRE(recII9.includeNinth);
        REQUIRE(resolveChordLabel(2, recII9, ChordShape::ninth) == "Dm9");

        // Degree 4 (V) -> G9
        auto recV9 = resolveChordRecipe(Scale::major, 4, ChordShape::ninth, QualityRule::diatonic);
        REQUIRE(recV9.quality == ResolvedQuality::dominant);
        REQUIRE(recV9.seventh == SeventhKind::minor);
        REQUIRE(recV9.includeNinth);
        REQUIRE(resolveChordLabel(7, recV9, ChordShape::ninth) == "G9");

        // Degree 4 (V) -> G13
        auto recV13 = resolveChordRecipe(Scale::major, 4, ChordShape::thirteenth, QualityRule::diatonic);
        REQUIRE(recV13.quality == ResolvedQuality::dominant);
        REQUIRE(recV13.includeThirteenth);
        REQUIRE(resolveChordLabel(7, recV13, ChordShape::thirteenth) == "G13");

        // Degree 5 (vi) -> Am11
        auto recVI11 = resolveChordRecipe(Scale::major, 5, ChordShape::eleventh, QualityRule::diatonic);
        REQUIRE(recVI11.quality == ResolvedQuality::minor);
        REQUIRE(recVI11.includeEleventh);
        REQUIRE(resolveChordLabel(9, recVI11, ChordShape::eleventh) == "Am11");

        // Degree 6 (vii) -> Bm7b5
        auto recVII7 = resolveChordRecipe(Scale::major, 6, ChordShape::seventh, QualityRule::diatonic);
        REQUIRE(recVII7.quality == ResolvedQuality::diminished);
        REQUIRE(recVII7.seventh == SeventhKind::halfDiminished);
        REQUIRE(resolveChordLabel(11, recVII7, ChordShape::seventh) == "Bm7b5");
    }

    SECTION("C natural minor diatonic jazz chords") {
        // Degree 0 (i) -> Cm9
        auto recI = resolveChordRecipe(Scale::naturalMinor, 0, ChordShape::ninth, QualityRule::diatonic);
        REQUIRE(recI.quality == ResolvedQuality::minor);
        REQUIRE(resolveChordLabel(0, recI, ChordShape::ninth) == "Cm9");

        // Degree 1 (ii) -> Dm7b5
        auto recII = resolveChordRecipe(Scale::naturalMinor, 1, ChordShape::seventh, QualityRule::diatonic);
        REQUIRE(recII.quality == ResolvedQuality::diminished);
        REQUIRE(resolveChordLabel(2, recII, ChordShape::seventh) == "Dm7b5");

        // Degree 2 (III) -> D#maj9
        auto recIII = resolveChordRecipe(Scale::naturalMinor, 2, ChordShape::ninth, QualityRule::diatonic);
        REQUIRE(recIII.quality == ResolvedQuality::major);
        REQUIRE(resolveChordLabel(3, recIII, ChordShape::ninth) == "D#maj9");

        // Degree 3 (iv) -> Fm9
        auto recIV = resolveChordRecipe(Scale::naturalMinor, 3, ChordShape::ninth, QualityRule::diatonic);
        REQUIRE(recIV.quality == ResolvedQuality::minor);
        REQUIRE(resolveChordLabel(5, recIV, ChordShape::ninth) == "Fm9");

        // Degree 4 (v) -> Gm9
        auto recV = resolveChordRecipe(Scale::naturalMinor, 4, ChordShape::ninth, QualityRule::diatonic);
        REQUIRE(recV.quality == ResolvedQuality::minor);
        REQUIRE(resolveChordLabel(7, recV, ChordShape::ninth) == "Gm9");

        // Degree 5 (VI) -> G#maj9
        auto recVI = resolveChordRecipe(Scale::naturalMinor, 5, ChordShape::ninth, QualityRule::diatonic);
        REQUIRE(recVI.quality == ResolvedQuality::major);
        REQUIRE(resolveChordLabel(8, recVI, ChordShape::ninth) == "G#maj9");

        // Degree 6 (VII) -> A#9
        auto recVII = resolveChordRecipe(Scale::naturalMinor, 6, ChordShape::ninth, QualityRule::diatonic);
        REQUIRE(recVII.quality == ResolvedQuality::dominant);
        REQUIRE(resolveChordLabel(10, recVII, ChordShape::ninth) == "A#9");
    }

    SECTION("Free rule overrides") {
        // Major + ninth -> maj9
        auto recMaj9 = resolveChordRecipe(Scale::major, 1, ChordShape::ninth, QualityRule::major);
        REQUIRE(recMaj9.quality == ResolvedQuality::major);
        REQUIRE(resolveChordLabel(2, recMaj9, ChordShape::ninth) == "Dmaj9");

        // Minor + ninth -> m9
        auto recMin9 = resolveChordRecipe(Scale::major, 0, ChordShape::ninth, QualityRule::minor);
        REQUIRE(recMin9.quality == ResolvedQuality::minor);
        REQUIRE(resolveChordLabel(0, recMin9, ChordShape::ninth) == "Cm9");

        // Dominant + ninth -> 9
        auto recDom9 = resolveChordRecipe(Scale::major, 0, ChordShape::ninth, QualityRule::dominant);
        REQUIRE(recDom9.quality == ResolvedQuality::dominant);
        REQUIRE(resolveChordLabel(0, recDom9, ChordShape::ninth) == "C9");

        // Diminished + seventh -> dim7
        auto recDim7 = resolveChordRecipe(Scale::major, 0, ChordShape::seventh, QualityRule::diminished);
        REQUIRE(recDim7.quality == ResolvedQuality::diminished);
        REQUIRE(resolveChordLabel(0, recDim7, ChordShape::seventh) == "Cdim7");
    }

    SECTION("add9 and 6/9 specific shapes") {
        auto recAdd9 = resolveChordRecipe(Scale::major, 0, ChordShape::add9, QualityRule::diatonic);
        REQUIRE(recAdd9.seventh == SeventhKind::none);
        REQUIRE(recAdd9.includeNinth);
        REQUIRE(resolveChordLabel(0, recAdd9, ChordShape::add9) == "Cadd9");

        auto rec69 = resolveChordRecipe(Scale::major, 0, ChordShape::sixNine, QualityRule::diatonic);
        REQUIRE(rec69.seventh == SeventhKind::none);
        REQUIRE(rec69.includeSixth);
        REQUIRE(rec69.includeNinth);
        REQUIRE(resolveChordLabel(0, rec69, ChordShape::sixNine) == "C6/9");

        auto recSus2 = resolveChordRecipe(Scale::major, 0, ChordShape::sus2, QualityRule::diatonic);
        REQUIRE(resolveChordLabel(0, recSus2, ChordShape::sus2) == "Csus2");

        auto recSus4 = resolveChordRecipe(Scale::major, 0, ChordShape::sus4, QualityRule::diatonic);
        REQUIRE(resolveChordLabel(0, recSus4, ChordShape::sus4) == "Csus4");
    }
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
