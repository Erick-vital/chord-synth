#include <catch2/catch_test_macros.hpp>
#include <interaction/ChordTransform.h>
#include <music/DiatonicChordVoicer.h>
#include <music/VoicedChord.h>

using namespace chordsynth::interaction;
using namespace chordsynth::music;

TEST_CASE("ChordTransform contract definitions and dimensions", "[interaction][transform]") {
    VoicingSpec base{
        .shape = ChordShape::triad,
        .extension = ChordExtension::triad,
        .inversion = 0,
        .style = VoicingStyle::compact,
        .fifthPolicy = FifthPolicy::automatic,
        .bassMode = BassMode::none,
        .slashDegree = 0,
        .voiceLeading = VoiceLeadingMode::manual,
        .baseOctave = 3,
        .qualityRule = QualityRule::diatonic
    };

    const std::array<TransformPalette, 3> palettes{
        TransformPalette::basic,
        TransformPalette::loFi,
        TransformPalette::spice
    };

    const std::array<TransformSlot, 8> slots{
        TransformSlot::one,
        TransformSlot::two,
        TransformSlot::three,
        TransformSlot::four,
        TransformSlot::five,
        TransformSlot::six,
        TransformSlot::seven,
        TransformSlot::eight
    };

    DiatonicChordVoicer voicer;

    SECTION("All 24 transform slots produce valid, deterministic specs and non-empty labels across all degrees") {
        for (auto palette : palettes) {
            for (auto slot : slots) {
                for (int deg = 0; deg < 7; ++deg) {
                    auto result = applyChordTransform(palette, slot, base, Scale::major, deg);

                    REQUIRE_FALSE(result.label.empty());
                    REQUIRE(base.shape == ChordShape::triad);
                    REQUIRE(base.qualityRule == QualityRule::diatonic);

                    // Must be voicable without throwing or corrupting
                    auto voiced = voicer.voiceChord(0, deg, result.spec, Scale::major);
                    REQUIRE(voiced.notes.size() > 0);
                    REQUIRE(voiced.notes.size() <= static_cast<int>(maxChordTones));
                }
            }
        }
    }

    SECTION("Source spec remains completely immutable") {
        const VoicingSpec original = base;
        auto res = applyChordTransform(TransformPalette::loFi, TransformSlot::five, base, Scale::major, 0);
        REQUIRE(base == original);
        REQUIRE(res.spec.style == VoicingStyle::open);
        REQUIRE(res.spec.shape == ChordShape::ninth);
    }
}

TEST_CASE("Basic palette specific transformation semantics", "[interaction][transform]") {
    DiatonicChordVoicer voicer;
    VoicingSpec base{
        .shape = ChordShape::triad,
        .qualityRule = QualityRule::diatonic
    };

    SECTION("Slot 1: Major/Minor flip") {
        // Degree 0 in C major is C Major -> flip to Minor
        auto resI = applyChordTransform(TransformPalette::basic, TransformSlot::one, base, Scale::major, 0);
        REQUIRE(resI.spec.qualityRule == QualityRule::minor);
        auto vI = voicer.voiceChord(0, 0, resI.spec, Scale::major);
        REQUIRE(vI.label == "Cm");

        // Degree 1 in C major is D Minor -> flip to Major
        auto resii = applyChordTransform(TransformPalette::basic, TransformSlot::one, base, Scale::major, 1);
        REQUIRE(resii.spec.qualityRule == QualityRule::major);
        auto vii = voicer.voiceChord(0, 1, resii.spec, Scale::major);
        REQUIRE(vii.label == "D");
    }

    SECTION("Slot 2: Dominant 7") {
        auto res = applyChordTransform(TransformPalette::basic, TransformSlot::two, base, Scale::major, 0);
        REQUIRE(res.spec.shape == ChordShape::seventh);
        REQUIRE(res.spec.qualityRule == QualityRule::dominant);
        auto v = voicer.voiceChord(0, 0, res.spec, Scale::major);
        REQUIRE(v.label == "C7");
    }

    SECTION("Slot 3: Seventh Color") {
        auto res = applyChordTransform(TransformPalette::basic, TransformSlot::three, base, Scale::major, 0);
        REQUIRE(res.spec.shape == ChordShape::seventh);
        auto v = voicer.voiceChord(0, 0, res.spec, Scale::major);
        REQUIRE(v.label == "Cmaj7");
    }

    SECTION("Slot 4: add9") {
        auto res = applyChordTransform(TransformPalette::basic, TransformSlot::four, base, Scale::major, 0);
        REQUIRE(res.spec.shape == ChordShape::add9);
        auto v = voicer.voiceChord(0, 0, res.spec, Scale::major);
        REQUIRE(v.label == "Cadd9");
    }

    SECTION("Slot 5: sus4") {
        auto res = applyChordTransform(TransformPalette::basic, TransformSlot::five, base, Scale::major, 0);
        REQUIRE(res.spec.shape == ChordShape::sus4);
        auto v = voicer.voiceChord(0, 0, res.spec, Scale::major);
        REQUIRE(v.label == "Csus4");
    }

    SECTION("Slot 6: sus2") {
        auto res = applyChordTransform(TransformPalette::basic, TransformSlot::six, base, Scale::major, 0);
        REQUIRE(res.spec.shape == ChordShape::sus2);
        auto v = voicer.voiceChord(0, 0, res.spec, Scale::major);
        REQUIRE(v.label == "Csus2");
    }

    SECTION("Slot 7: 6/9") {
        auto res = applyChordTransform(TransformPalette::basic, TransformSlot::seven, base, Scale::major, 0);
        REQUIRE(res.spec.shape == ChordShape::sixNine);
        auto v = voicer.voiceChord(0, 0, res.spec, Scale::major);
        REQUIRE(v.label == "C6/9");
    }

    SECTION("Slot 8: Diminished") {
        auto res = applyChordTransform(TransformPalette::basic, TransformSlot::eight, base, Scale::major, 0);
        REQUIRE(res.spec.qualityRule == QualityRule::diminished);
        auto v = voicer.voiceChord(0, 0, res.spec, Scale::major);
        REQUIRE(v.label == "Cdim");
    }
}

TEST_CASE("Lo-Fi palette specific transformation semantics", "[interaction][transform]") {
    DiatonicChordVoicer voicer;
    VoicingSpec base{
        .shape = ChordShape::triad,
        .qualityRule = QualityRule::diatonic
    };

    SECTION("Slot 1: Ninth") {
        auto res = applyChordTransform(TransformPalette::loFi, TransformSlot::one, base, Scale::major, 0);
        REQUIRE(res.spec.shape == ChordShape::ninth);
        auto v = voicer.voiceChord(0, 0, res.spec, Scale::major);
        REQUIRE(v.label == "Cmaj9");
    }

    SECTION("Slot 2: add9") {
        auto res = applyChordTransform(TransformPalette::loFi, TransformSlot::two, base, Scale::major, 0);
        REQUIRE(res.spec.shape == ChordShape::add9);
        auto v = voicer.voiceChord(0, 0, res.spec, Scale::major);
        REQUIRE(v.label == "Cadd9");
    }

    SECTION("Slot 3: 6/9") {
        auto res = applyChordTransform(TransformPalette::loFi, TransformSlot::three, base, Scale::major, 0);
        REQUIRE(res.spec.shape == ChordShape::sixNine);
        auto v = voicer.voiceChord(0, 0, res.spec, Scale::major);
        REQUIRE(v.label == "C6/9");
    }

    SECTION("Slot 4: Eleventh") {
        auto res = applyChordTransform(TransformPalette::loFi, TransformSlot::four, base, Scale::major, 5); // vi (Am)
        REQUIRE(res.spec.shape == ChordShape::eleventh);
        auto v = voicer.voiceChord(0, 5, res.spec, Scale::major);
        REQUIRE(v.label == "Am11");
    }

    SECTION("Slot 5: Open 9") {
        auto res = applyChordTransform(TransformPalette::loFi, TransformSlot::five, base, Scale::major, 0);
        REQUIRE(res.spec.shape == ChordShape::ninth);
        REQUIRE(res.spec.style == VoicingStyle::open);
        auto v = voicer.voiceChord(0, 0, res.spec, Scale::major);
        REQUIRE(v.label == "Cmaj9");
    }

    SECTION("Slot 6: Rootless 7") {
        auto res = applyChordTransform(TransformPalette::loFi, TransformSlot::six, base, Scale::major, 0);
        REQUIRE(res.spec.shape == ChordShape::seventh);
        REQUIRE(res.spec.style == VoicingStyle::rootless);
        auto v = voicer.voiceChord(0, 0, res.spec, Scale::major);
        REQUIRE(v.label == "Cmaj7");
        // Root (48) must be omitted in rootless 7th
        REQUIRE_FALSE(v.notes[0] == 48);
    }

    SECTION("Slot 7: Warm 13") {
        auto res = applyChordTransform(TransformPalette::loFi, TransformSlot::seven, base, Scale::major, 4); // V (G)
        REQUIRE(res.spec.shape == ChordShape::thirteenth);
        REQUIRE(res.spec.style == VoicingStyle::open);
        REQUIRE(res.spec.fifthPolicy == FifthPolicy::omit);
        auto v = voicer.voiceChord(0, 4, res.spec, Scale::major);
        REQUIRE(v.label == "G13");
    }

    SECTION("Slot 8: Nearest Open") {
        auto res = applyChordTransform(TransformPalette::loFi, TransformSlot::eight, base, Scale::major, 0);
        REQUIRE(res.spec.style == VoicingStyle::open);
        REQUIRE(res.spec.voiceLeading == VoiceLeadingMode::nearest);
    }
}

TEST_CASE("Spice palette specific transformation semantics", "[interaction][transform]") {
    DiatonicChordVoicer voicer;
    VoicingSpec base{
        .shape = ChordShape::triad,
        .qualityRule = QualityRule::diatonic
    };

    SECTION("Slot 1: Dominant 7") {
        auto res = applyChordTransform(TransformPalette::spice, TransformSlot::one, base, Scale::major, 0);
        REQUIRE(res.spec.shape == ChordShape::seventh);
        REQUIRE(res.spec.qualityRule == QualityRule::dominant);
        auto v = voicer.voiceChord(0, 0, res.spec, Scale::major);
        REQUIRE(v.label == "C7");
    }

    SECTION("Slot 2: Diminished 7") {
        auto res = applyChordTransform(TransformPalette::spice, TransformSlot::two, base, Scale::major, 0);
        REQUIRE(res.spec.shape == ChordShape::seventh);
        REQUIRE(res.spec.qualityRule == QualityRule::diminished);
        auto v = voicer.voiceChord(0, 0, res.spec, Scale::major);
        REQUIRE(v.label == "Cdim7");
    }

    SECTION("Slot 3: sus4") {
        auto res = applyChordTransform(TransformPalette::spice, TransformSlot::three, base, Scale::major, 0);
        REQUIRE(res.spec.shape == ChordShape::sus4);
        auto v = voicer.voiceChord(0, 0, res.spec, Scale::major);
        REQUIRE(v.label == "Csus4");
    }

    SECTION("Slot 4: Dominant 9") {
        auto res = applyChordTransform(TransformPalette::spice, TransformSlot::four, base, Scale::major, 0);
        REQUIRE(res.spec.shape == ChordShape::ninth);
        REQUIRE(res.spec.qualityRule == QualityRule::dominant);
        auto v = voicer.voiceChord(0, 0, res.spec, Scale::major);
        REQUIRE(v.label == "C9");
    }

    SECTION("Slot 5: Dominant 13") {
        auto res = applyChordTransform(TransformPalette::spice, TransformSlot::five, base, Scale::major, 0);
        REQUIRE(res.spec.shape == ChordShape::thirteenth);
        REQUIRE(res.spec.qualityRule == QualityRule::dominant);
        REQUIRE(res.spec.fifthPolicy == FifthPolicy::omit);
        auto v = voicer.voiceChord(0, 0, res.spec, Scale::major);
        REQUIRE(v.label == "C13");
    }

    SECTION("Slot 6: Minor 9 tension") {
        auto res = applyChordTransform(TransformPalette::spice, TransformSlot::six, base, Scale::major, 0);
        REQUIRE(res.spec.shape == ChordShape::ninth);
        REQUIRE(res.spec.qualityRule == QualityRule::minor);
        auto v = voicer.voiceChord(0, 0, res.spec, Scale::major);
        REQUIRE(v.label == "Cm9");
    }

    SECTION("Slot 7: Rootless 9") {
        auto res = applyChordTransform(TransformPalette::spice, TransformSlot::seven, base, Scale::major, 0);
        REQUIRE(res.spec.shape == ChordShape::ninth);
        REQUIRE(res.spec.style == VoicingStyle::rootless);
        auto v = voicer.voiceChord(0, 0, res.spec, Scale::major);
        REQUIRE(v.label == "Cmaj9");
        REQUIRE_FALSE(v.notes[0] == 48); // root omitted
    }

    SECTION("Slot 8: Open 11") {
        auto res = applyChordTransform(TransformPalette::spice, TransformSlot::eight, base, Scale::major, 5);
        REQUIRE(res.spec.shape == ChordShape::eleventh);
        REQUIRE(res.spec.style == VoicingStyle::open);
        auto v = voicer.voiceChord(0, 5, res.spec, Scale::major);
        REQUIRE(v.label == "Am11");
    }
}
