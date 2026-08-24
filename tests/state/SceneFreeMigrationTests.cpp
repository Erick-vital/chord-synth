#include <catch2/catch_test_macros.hpp>
#include "state/HarmonyState.h"
#include <juce_data_structures/juce_data_structures.h>

using namespace chordsynth::state;
using namespace chordsynth::music;

TEST_CASE("SceneFreeHarmonyState migration and serialization", "[state][harmony][scenefree]") {
    SECTION("Legacy v2 state with selectedScene=2 extracts scene 2 specs and drops other scenes on v3 save") {
        juce::ValueTree v2Tree{"HarmonyState"};
        v2Tree.setProperty("version", 2, nullptr);
        v2Tree.setProperty("selectedScene", 2, nullptr);
        v2Tree.setProperty("liveRevoice", true, nullptr);
        v2Tree.setProperty("qualityRule", static_cast<int>(QualityRule::dominant), nullptr);

        juce::ValueTree scenesNode{"Scenes"};

        // Scene 0 has triad
        juce::ValueTree scene0{"Scene"};
        scene0.setProperty("index", 0, nullptr);
        juce::ValueTree deg0_0{"Degree"};
        deg0_0.setProperty("index", 0, nullptr);
        deg0_0.setProperty("shape", static_cast<int>(ChordShape::triad), nullptr);
        deg0_0.setProperty("style", static_cast<int>(VoicingStyle::compact), nullptr);
        deg0_0.setProperty("baseOctave", 3, nullptr);
        scene0.appendChild(deg0_0, nullptr);
        scenesNode.appendChild(scene0, nullptr);

        // Scene 2 has thirteenth, rootless, root bass (divergent from scene 0)
        juce::ValueTree scene2{"Scene"};
        scene2.setProperty("index", 2, nullptr);
        juce::ValueTree deg2_0{"Degree"};
        deg2_0.setProperty("index", 0, nullptr);
        deg2_0.setProperty("shape", static_cast<int>(ChordShape::thirteenth), nullptr);
        deg2_0.setProperty("style", static_cast<int>(VoicingStyle::rootless), nullptr);
        deg2_0.setProperty("bassMode", static_cast<int>(BassMode::root), nullptr);
        deg2_0.setProperty("baseOctave", 4, nullptr);
        scene2.appendChild(deg2_0, nullptr);
        scenesNode.appendChild(scene2, nullptr);

        v2Tree.appendChild(scenesNode, nullptr);

        HarmonyState restored;
        REQUIRE(restored.loadFromValueTree(v2Tree));
        REQUIRE(restored.getLiveRevoice() == true);
        REQUIRE(restored.getQualityRule() == QualityRule::dominant);

        // Assert migration picked scene 2 specs into degree 0
        const auto spec0 = restored.getConfiguration().getSpec(0);
        REQUIRE(spec0.shape == ChordShape::thirteenth);
        REQUIRE(spec0.style == VoicingStyle::rootless);
        REQUIRE(spec0.bassMode == BassMode::root);
        REQUIRE(spec0.baseOctave == 4);

        // Assert re-save emits version 3 and contains NO selectedScene / Scenes
        auto v3Tree = restored.toValueTree();
        REQUIRE(v3Tree.isValid());
        REQUIRE(static_cast<int>(v3Tree.getProperty("version")) == 3);
        REQUIRE_FALSE(v3Tree.hasProperty("selectedScene"));
        REQUIRE_FALSE(v3Tree.getChildWithName("Scenes").isValid());
        REQUIRE(v3Tree.getChildWithName("Degrees").isValid());
    }
}
