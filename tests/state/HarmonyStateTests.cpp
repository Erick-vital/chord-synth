#include <catch2/catch_test_macros.hpp>
#include "state/HarmonyState.h"
#include <juce_data_structures/juce_data_structures.h>

using namespace chordsynth::state;
using namespace chordsynth::music;

TEST_CASE("HarmonyState ValueTree serialization and round-trip", "[state][harmony]") {
    SECTION("Default HarmonyState serializes as v3 and deserializes cleanly") {
        HarmonyState state;
        auto vt = state.toValueTree();
        REQUIRE(vt.isValid());
        REQUIRE(vt.getType().toString() == "HarmonyState");
        REQUIRE(static_cast<int>(vt.getProperty("version")) == 3);
        REQUIRE_FALSE(vt.hasProperty("selectedScene"));
        REQUIRE(static_cast<bool>(vt.getProperty("liveRevoice")) == false);
        REQUIRE(static_cast<int>(vt.getProperty("qualityRule")) == static_cast<int>(QualityRule::diatonic));
        REQUIRE(vt.getChildWithName("Degrees").isValid());
        REQUIRE_FALSE(vt.getChildWithName("Scenes").isValid());

        HarmonyState restored;
        REQUIRE(restored.loadFromValueTree(vt));
        REQUIRE(restored.getLiveRevoice() == false);
        REQUIRE(restored.getQualityRule() == QualityRule::diatonic);
        REQUIRE(restored.getConfiguration() == HarmonyConfiguration{});
    }

    SECTION("Custom HarmonyState preserves all v3 fields across round-trip") {
        HarmonyState original;
        original.setLiveRevoice(true);
        original.setQualityRule(QualityRule::major);

        VoicingSpec customSpec{
            .shape = ChordShape::thirteenth,
            .extension = ChordExtension::seventh,
            .inversion = 3,
            .style = VoicingStyle::rootless,
            .fifthPolicy = FifthPolicy::omit,
            .bassMode = BassMode::slashDegree,
            .slashDegree = 4,
            .voiceLeading = VoiceLeadingMode::nearest,
            .baseOctave = 4,
            .qualityRule = QualityRule::minor
        };
        original.getConfiguration().setSpec(1, customSpec); // Degree 1 (ii)

        auto vt = original.toValueTree();
        REQUIRE(vt.isValid());
        REQUIRE(static_cast<int>(vt.getProperty("version")) == 3);

        HarmonyState restored;
        REQUIRE(restored.loadFromValueTree(vt));
        REQUIRE(restored.getLiveRevoice() == true);
        REQUIRE(restored.getQualityRule() == QualityRule::major);
        REQUIRE(restored.getConfiguration().getSpec(1) == customSpec);
        REQUIRE(restored.getConfiguration() == original.getConfiguration());
    }

    SECTION("Legacy v1 state migrates cleanly with compatibility defaults") {
        juce::ValueTree v1Tree{"HarmonyState"};
        v1Tree.setProperty("version", 1, nullptr);
        v1Tree.setProperty("selectedScene", 0, nullptr);
        v1Tree.setProperty("liveRevoice", true, nullptr);
        v1Tree.setProperty("qualityRule", static_cast<int>(QualityRule::minor), nullptr);

        juce::ValueTree scenesNode{"Scenes"};
        juce::ValueTree scene0{"Scene"};
        scene0.setProperty("index", 0, nullptr);

        juce::ValueTree degree0{"Degree"};
        degree0.setProperty("index", 0, nullptr);
        degree0.setProperty("extension", 1, nullptr); // seventh
        degree0.setProperty("inversion", 1, nullptr);
        degree0.setProperty("style", 1, nullptr);     // open
        degree0.setProperty("baseOctave", 4, nullptr);
        degree0.setProperty("qualityRule", static_cast<int>(QualityRule::major), nullptr);
        scene0.appendChild(degree0, nullptr);
        scenesNode.appendChild(scene0, nullptr);
        v1Tree.appendChild(scenesNode, nullptr);

        HarmonyState restored;
        REQUIRE(restored.loadFromValueTree(v1Tree));
        REQUIRE(restored.getLiveRevoice() == true);
        REQUIRE(restored.getQualityRule() == QualityRule::minor);

        // Degree 0 was explicitly defined in legacy scene 0 of v1:
        // extension=1 -> shape=seventh, style=1 -> style=open
        // compatibility defaults: fifthPolicy=automatic, bassMode=none, voiceLeading=manual
        const auto spec0 = restored.getConfiguration().getSpec(0);
        REQUIRE(spec0.shape == ChordShape::seventh);
        REQUIRE(spec0.extension == ChordExtension::seventh);
        REQUIRE(spec0.inversion == 1);
        REQUIRE(spec0.style == VoicingStyle::open);
        REQUIRE(spec0.baseOctave == 4);
        REQUIRE(spec0.qualityRule == QualityRule::major);
        REQUIRE(spec0.fifthPolicy == FifthPolicy::automatic);
        REQUIRE(spec0.bassMode == BassMode::none);
        REQUIRE(spec0.voiceLeading == VoiceLeadingMode::manual);

        // When re-serialized, it must emit v3
        auto reSavedVt = restored.toValueTree();
        REQUIRE(static_cast<int>(reSavedVt.getProperty("version")) == 3);
        REQUIRE_FALSE(reSavedVt.hasProperty("selectedScene"));
        REQUIRE_FALSE(reSavedVt.getChildWithName("Scenes").isValid());
    }

    SECTION("Legacy v1 state with missing Scenes node populates legacy selected-scene defaults") {
        juce::ValueTree v1TreeNoScenes{"HarmonyState"};
        v1TreeNoScenes.setProperty("version", 1, nullptr);
        v1TreeNoScenes.setProperty("selectedScene", 2, nullptr);
        v1TreeNoScenes.setProperty("liveRevoice", false, nullptr);

        HarmonyState restored;
        REQUIRE(restored.loadFromValueTree(v1TreeNoScenes));
        REQUIRE(restored.getLiveRevoice() == false);
        REQUIRE(restored.getQualityRule() == QualityRule::diatonic);

        // Legacy defaults for selected Scene C (2): triad, open, inv 0
        // All with fifthPolicy=automatic, bassMode=none, voiceLeading=manual
        const auto spec = restored.getConfiguration().getSpec(0);
        REQUIRE(spec.shape == ChordShape::triad);
        REQUIRE(spec.style == VoicingStyle::open);
        REQUIRE(spec.inversion == 0);
        REQUIRE(spec.fifthPolicy == FifthPolicy::automatic);
        REQUIRE(spec.bassMode == BassMode::none);
        REQUIRE(spec.voiceLeading == VoiceLeadingMode::manual);
    }

    SECTION("Out of range or malformed properties are sanitized or rejected") {
        SECTION("Invalid type tag fails") {
            juce::ValueTree invalidTag{"WrongTag"};
            HarmonyState state;
            REQUIRE_FALSE(state.loadFromValueTree(invalidTag));
        }

        SECTION("Unsupported version (version <= 0 or version > 3) fails safely") {
            juce::ValueTree futureState{"HarmonyState"};
            futureState.setProperty("version", 99, nullptr);
            HarmonyState state;
            REQUIRE_FALSE(state.loadFromValueTree(futureState));

            juce::ValueTree zeroState{"HarmonyState"};
            zeroState.setProperty("version", 0, nullptr);
            REQUIRE_FALSE(state.loadFromValueTree(zeroState));
        }

        SECTION("Out-of-range top-level values are clamped safely") {
            juce::ValueTree vt{"HarmonyState"};
            vt.setProperty("version", 3, nullptr);
            vt.setProperty("liveRevoice", true, nullptr);
            vt.setProperty("qualityRule", 99, nullptr); // fallback to diatonic

            HarmonyState state;
            REQUIRE(state.loadFromValueTree(vt));
            REQUIRE(state.getLiveRevoice() == true);
            REQUIRE(state.getQualityRule() == QualityRule::diatonic);
        }

        SECTION("Malformed v3 specs are sanitized to safe bounds") {
            juce::ValueTree vt{"HarmonyState"};
            vt.setProperty("version", 3, nullptr);
            vt.setProperty("liveRevoice", false, nullptr);
            vt.setProperty("qualityRule", 0, nullptr);

            juce::ValueTree degreesNode{"Degrees"};
            juce::ValueTree degreeNode{"Degree"};
            degreeNode.setProperty("index", 0, nullptr);
            degreeNode.setProperty("shape", 99, nullptr);      // invalid -> fallback triad
            degreeNode.setProperty("inversion", 10, nullptr);  // clamped to 5
            degreeNode.setProperty("style", 99, nullptr);      // fallback compact
            degreeNode.setProperty("baseOctave", 10, nullptr); // clamped to 4
            degreeNode.setProperty("qualityRule", 99, nullptr);// fallback diatonic
            degreeNode.setProperty("fifthPolicy", 99, nullptr);// fallback automatic
            degreeNode.setProperty("bassMode", 99, nullptr);   // fallback none
            degreeNode.setProperty("slashDegree", 10, nullptr);// clamped to 6
            degreeNode.setProperty("voiceLeading", 99, nullptr);// fallback manual
            degreesNode.appendChild(degreeNode, nullptr);
            vt.appendChild(degreesNode, nullptr);

            HarmonyState state;
            REQUIRE(state.loadFromValueTree(vt));
            auto spec = state.getConfiguration().getSpec(0);
            REQUIRE(spec.shape == ChordShape::triad);
            REQUIRE(spec.inversion == 5);
            REQUIRE(spec.style == VoicingStyle::compact);
            REQUIRE(spec.baseOctave == 4);
            REQUIRE(spec.qualityRule == QualityRule::diatonic);
            REQUIRE(spec.fifthPolicy == FifthPolicy::automatic);
            REQUIRE(spec.bassMode == BassMode::none);
            REQUIRE(spec.slashDegree == 6);
            REQUIRE(spec.voiceLeading == VoiceLeadingMode::manual);
        }
    }
}
