#include <catch2/catch_test_macros.hpp>
#include "state/HarmonyState.h"
#include <juce_data_structures/juce_data_structures.h>

using namespace chordsynth::state;
using namespace chordsynth::music;

TEST_CASE("HarmonyState ValueTree serialization and round-trip", "[state][harmony]") {
    SECTION("Default HarmonyState serializes and deserializes cleanly") {
        HarmonyState state;
        auto vt = state.toValueTree();
        REQUIRE(vt.isValid());
        REQUIRE(vt.getType().toString() == "HarmonyState");
        REQUIRE(static_cast<int>(vt.getProperty("version")) == 1);
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

    SECTION("Custom HarmonyState preserves non-default overrides and settings across round-trip") {
        HarmonyState original;
        original.setSelectedScene(2);
        original.setLiveRevoice(true);
        original.setQualityRule(QualityRule::major);

        VoicingSpec customSpec{
            .extension = ChordExtension::seventh,
            .inversion = 2,
            .style = VoicingStyle::open,
            .baseOctave = 4,
            .qualityRule = QualityRule::minor
        };
        original.getConfiguration().setSpec(2, 1, customSpec); // Scene 2, Degree 1 (ii)

        auto vt = original.toValueTree();
        REQUIRE(vt.isValid());

        HarmonyState restored;
        REQUIRE(restored.loadFromValueTree(vt));
        REQUIRE(restored.getSelectedScene() == 2);
        REQUIRE(restored.getLiveRevoice() == true);
        REQUIRE(restored.getQualityRule() == QualityRule::major);
        REQUIRE(restored.getConfiguration().getSpec(2, 1) == customSpec);
        REQUIRE(restored.getConfiguration() == original.getConfiguration());
    }

    SECTION("Out of range or malformed properties are sanitized or rejected") {
        SECTION("Invalid type tag fails") {
            juce::ValueTree invalidTag{"WrongTag"};
            HarmonyState state;
            REQUIRE_FALSE(state.loadFromValueTree(invalidTag));
        }

        SECTION("Unsupported future version fails safely") {
            juce::ValueTree futureState{"HarmonyState"};
            futureState.setProperty("version", 99, nullptr);
            HarmonyState state;
            REQUIRE_FALSE(state.loadFromValueTree(futureState));
        }

        SECTION("Out-of-range top-level values are clamped safely") {
            juce::ValueTree vt{"HarmonyState"};
            vt.setProperty("version", 1, nullptr);
            vt.setProperty("selectedScene", 10, nullptr); // clamped to 3
            vt.setProperty("liveRevoice", true, nullptr);
            vt.setProperty("qualityRule", 99, nullptr); // clamped/fallback to diatonic

            HarmonyState state;
            REQUIRE(state.loadFromValueTree(vt));
            REQUIRE(state.getSelectedScene() == 3);
            REQUIRE(state.getLiveRevoice() == true);
            REQUIRE(state.getQualityRule() == QualityRule::diatonic);
        }

        SECTION("Malformed specs are sanitized to safe bounds") {
            juce::ValueTree vt{"HarmonyState"};
            vt.setProperty("version", 1, nullptr);
            vt.setProperty("selectedScene", 0, nullptr);
            vt.setProperty("liveRevoice", false, nullptr);
            vt.setProperty("qualityRule", 0, nullptr);

            juce::ValueTree scenesNode{"Scenes"};
            juce::ValueTree scene0{"Scene"};
            scene0.setProperty("index", 0, nullptr);

            juce::ValueTree degreeNode{"Degree"};
            degreeNode.setProperty("index", 0, nullptr);
            degreeNode.setProperty("extension", 99, nullptr); // invalid -> fallback triad
            degreeNode.setProperty("inversion", 10, nullptr);  // clamped to 2
            degreeNode.setProperty("style", 99, nullptr);      // fallback close
            degreeNode.setProperty("baseOctave", 10, nullptr); // clamped to 4
            degreeNode.setProperty("qualityRule", 99, nullptr); // fallback diatonic
            scene0.appendChild(degreeNode, nullptr);
            scenesNode.appendChild(scene0, nullptr);
            vt.appendChild(scenesNode, nullptr);

            HarmonyState state;
            REQUIRE(state.loadFromValueTree(vt));
            auto spec = state.getConfiguration().getSpec(0, 0);
            REQUIRE(spec.extension == ChordExtension::triad);
            REQUIRE(spec.inversion == 2);
            REQUIRE(spec.style == VoicingStyle::close);
            REQUIRE(spec.baseOctave == 4);
            REQUIRE(spec.qualityRule == QualityRule::diatonic);
        }
    }
}
