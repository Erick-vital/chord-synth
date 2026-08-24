#include <catch2/catch_test_macros.hpp>
#include "presets/Preset.h"
#include "presets/PresetSerializer.h"

using namespace chordsynth;
using namespace chordsynth::presets;
using namespace chordsynth::music;

TEST_CASE("SceneFreePresetSerializer schema 5 and legacy migration", "[presets][scenefree]") {
    SECTION("Legacy schema version 3 JSON with selected_scene=2 migrates scene 2 into degrees") {
        juce::String v3Json = R"({
            "schema_version": 3,
            "name": "Legacy V3 Scene 2 Preset",
            "parameters": {
                "key": 0
            },
            "harmony": {
                "selected_scene": 2,
                "live_revoice": true,
                "quality_rule": 1,
                "scenes": [
                    {
                        "index": 0,
                        "degrees": [
                            {
                                "index": 0,
                                "shape": 0,
                                "style": 0
                            }
                        ]
                    },
                    {
                        "index": 2,
                        "degrees": [
                            {
                                "index": 0,
                                "shape": 4,
                                "style": 2,
                                "bass_mode": 1,
                                "base_octave": 4
                            }
                        ]
                    }
                ]
            }
        })";

        auto result = PresetSerializer::fromJson(v3Json);
        REQUIRE(result.has_value());
        const auto& p = *result;

        const auto& spec0 = p.harmony.getConfiguration().getSpec(0);
        REQUIRE(spec0.shape == ChordShape::thirteenth);
        REQUIRE(spec0.style == VoicingStyle::rootless);
        REQUIRE(spec0.bassMode == BassMode::root);
        REQUIRE(spec0.baseOctave == 4);

        // Serialization must emit schema_version 5 with degrees and no selected_scene / scenes
        auto outputJson = PresetSerializer::toJson(p);
        REQUIRE_FALSE(outputJson.contains("selected_scene"));
        REQUIRE_FALSE(outputJson.contains("\"scenes\""));
        REQUIRE(outputJson.contains("\"degrees\""));
        REQUIRE(outputJson.contains("\"schema_version\": 5"));
    }
}
