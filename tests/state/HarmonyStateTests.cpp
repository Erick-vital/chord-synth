#include <catch2/catch_test_macros.hpp>
#include "state/HarmonyState.h"
#include <juce_data_structures/juce_data_structures.h>

using namespace chordsynth::state;
using namespace chordsynth::music;

TEST_CASE("HarmonyState ValueTree serialization and round-trip", "[state][harmony]") {
    SECTION("Default HarmonyState serializes as v2 and deserializes cleanly") {
        HarmonyState state;
        auto vt = state.toValueTree();
        REQUIRE(vt.isValid());
        REQUIRE(vt.getType().toString() == "HarmonyState");
        REQUIRE(static_cast<int>(vt.getProperty("version")) == 2);
        REQUIRE(static_cast<int>(vt.getProperty("selectedScene")) == 0);
        REQUIRE(static_cast<bool>(vt.getProperty("liveRevoice")) == false);
        REQUIRE(static_cast<int>(vt.getProperty("qualityRule")) == static_cast<int>(QualityRule::diatonic));

        HarmonyState restored;
        REQUIRE(restored.loadFromValueTree(vt));
        REQUIRE(restored.getSelectedScene() == 0);
        REQUIRE(restored.getLiveRevoice() == false);
        REQUIRE(restored.getQualityRule() == QualityRule::diatonic);
        REQUIRE(restored.getConfiguration() == HarmonyConfiguration{});
    }

    SECTION("Custom HarmonyState preserves all v2 fields across round-trip") {
        HarmonyState original;
        original.setSelectedScene(2);
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
        original.getConfiguration().setSpec(2, 1, customSpec); // Scene 2, Degree 1 (ii)

        auto vt = original.toValueTree();
        REQUIRE(vt.isValid());
        REQUIRE(static_cast<int>(vt.getProperty("version")) == 2);

        HarmonyState restored;
        REQUIRE(restored.loadFromValueTree(vt));
        REQUIRE(restored.getSelectedScene() == 2);
        REQUIRE(restored.getLiveRevoice() == true);
        REQUIRE(restored.getQualityRule() == QualityRule::major);
        REQUIRE(restored.getConfiguration().getSpec(2, 1) == customSpec);
        REQUIRE(restored.getConfiguration() == original.getConfiguration());
    }

    SECTION("Legacy v1 state migrates cleanly with compatibility defaults") {
        juce::ValueTree v1Tree{"HarmonyState"};
        v1Tree.setProperty("version", 1, nullptr);
        v1Tree.setProperty("selectedScene", 1, nullptr);
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
        REQUIRE(restored.getSelectedScene() == 1);
        REQUIRE(restored.getLiveRevoice() == true);
        REQUIRE(restored.getQualityRule() == QualityRule::minor);

        // Degree (0, 0) was explicitly defined in v1:
        // extension=1 -> shape=seventh, style=1 -> style=open
        // compatibility defaults: fifthPolicy=automatic, bassMode=none, voiceLeading=manual
        const auto spec00 = restored.getConfiguration().getSpec(0, 0);
        REQUIRE(spec00.shape == ChordShape::seventh);
        REQUIRE(spec00.extension == ChordExtension::seventh);
        REQUIRE(spec00.inversion == 1);
        REQUIRE(spec00.style == VoicingStyle::open);
        REQUIRE(spec00.baseOctave == 4);
        REQUIRE(spec00.qualityRule == QualityRule::major);
        REQUIRE(spec00.fifthPolicy == FifthPolicy::automatic);
        REQUIRE(spec00.bassMode == BassMode::none);
        REQUIRE(spec00.voiceLeading == VoiceLeadingMode::manual);

        // When re-serialized, it must emit v2
        auto reSavedVt = restored.toValueTree();
        REQUIRE(static_cast<int>(reSavedVt.getProperty("version")) == 2);
    }

    SECTION("Legacy v1 state with missing Scenes node populates legacy scene defaults") {
        juce::ValueTree v1TreeNoScenes{"HarmonyState"};
        v1TreeNoScenes.setProperty("version", 1, nullptr);
        v1TreeNoScenes.setProperty("selectedScene", 2, nullptr);
        v1TreeNoScenes.setProperty("liveRevoice", false, nullptr);

        HarmonyState restored;
        REQUIRE(restored.loadFromValueTree(v1TreeNoScenes));
        REQUIRE(restored.getSelectedScene() == 2);
        REQUIRE(restored.getLiveRevoice() == false);
        REQUIRE(restored.getQualityRule() == QualityRule::diatonic);

        // Legacy defaults for A, B, C, D:
        // Scene A (0): triad, close (compact), inv 0
        // Scene B (1): seventh, close (compact), inv 0
        // Scene C (2): triad, open, inv 0
        // Scene D (3): triad, close (compact), inv 1
        // All with fifthPolicy=automatic, bassMode=none, voiceLeading=manual
        const auto specA = restored.getConfiguration().getSpec(0, 0);
        REQUIRE(specA.shape == ChordShape::triad);
        REQUIRE(specA.style == VoicingStyle::compact);
        REQUIRE(specA.inversion == 0);
        REQUIRE(specA.fifthPolicy == FifthPolicy::automatic);
        REQUIRE(specA.bassMode == BassMode::none);
        REQUIRE(specA.voiceLeading == VoiceLeadingMode::manual);

        const auto specB = restored.getConfiguration().getSpec(1, 0);
        REQUIRE(specB.shape == ChordShape::seventh);
        REQUIRE(specB.style == VoicingStyle::compact);
        REQUIRE(specB.inversion == 0);

        const auto specC = restored.getConfiguration().getSpec(2, 0);
        REQUIRE(specC.shape == ChordShape::triad);
        REQUIRE(specC.style == VoicingStyle::open);
        REQUIRE(specC.inversion == 0);

        const auto specD = restored.getConfiguration().getSpec(3, 0);
        REQUIRE(specD.shape == ChordShape::triad);
        REQUIRE(specD.style == VoicingStyle::compact);
        REQUIRE(specD.inversion == 1);
    }

    SECTION("Out of range or malformed properties are sanitized or rejected") {
        SECTION("Invalid type tag fails") {
            juce::ValueTree invalidTag{"WrongTag"};
            HarmonyState state;
            REQUIRE_FALSE(state.loadFromValueTree(invalidTag));
        }

        SECTION("Unsupported version (version <= 0 or version > 2) fails safely") {
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
            vt.setProperty("version", 2, nullptr);
            vt.setProperty("selectedScene", 10, nullptr); // clamped to 3
            vt.setProperty("liveRevoice", true, nullptr);
            vt.setProperty("qualityRule", 99, nullptr); // clamped/fallback to diatonic

            HarmonyState state;
            REQUIRE(state.loadFromValueTree(vt));
            REQUIRE(state.getSelectedScene() == 3);
            REQUIRE(state.getLiveRevoice() == true);
            REQUIRE(state.getQualityRule() == QualityRule::diatonic);
        }

        SECTION("Malformed v2 specs are sanitized to safe bounds") {
            juce::ValueTree vt{"HarmonyState"};
            vt.setProperty("version", 2, nullptr);
            vt.setProperty("selectedScene", 0, nullptr);
            vt.setProperty("liveRevoice", false, nullptr);
            vt.setProperty("qualityRule", 0, nullptr);

            juce::ValueTree scenesNode{"Scenes"};
            juce::ValueTree scene0{"Scene"};
            scene0.setProperty("index", 0, nullptr);

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
            scene0.appendChild(degreeNode, nullptr);
            scenesNode.appendChild(scene0, nullptr);
            vt.appendChild(scenesNode, nullptr);

            HarmonyState state;
            REQUIRE(state.loadFromValueTree(vt));
            auto spec = state.getConfiguration().getSpec(0, 0);
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
