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

    SECTION("Schema version 4 JSON with scenes extracts selected_scene and migrates to schema 5") {
        juce::String v4Json = R"({
            "schema_version": 4,
            "name": "Legacy V4 Preset",
            "parameters": {
                "key": 2
            },
            "harmony": {
                "selected_scene": 1,
                "live_revoice": false,
                "scenes": [
                    {
                        "index": 1,
                        "degrees": [
                            {
                                "index": 3,
                                "shape": 3,
                                "style": 1,
                                "fifth_policy": 2,
                                "slash_degree": 2
                            }
                        ]
                    }
                ]
            }
        })";

        auto result = PresetSerializer::fromJson(v4Json);
        REQUIRE(result.has_value());
        const auto& p = *result;

        const auto& spec3 = p.harmony.getConfiguration().getSpec(3);
        REQUIRE(spec3.shape == ChordShape::eleventh);
        REQUIRE(spec3.style == VoicingStyle::open);
        REQUIRE(spec3.fifthPolicy == FifthPolicy::omit);
        REQUIRE(spec3.slashDegree == 2);

        auto outputJson = PresetSerializer::toJson(p);
        REQUIRE_FALSE(outputJson.contains("selected_scene"));
        REQUIRE_FALSE(outputJson.contains("\"scenes\""));
        REQUIRE(outputJson.contains("\"degrees\""));
        REQUIRE(outputJson.contains("\"schema_version\": 5"));
    }

    SECTION("Schema version 5 JSON round-trip preserves all 7 degrees without legacy fields") {
        Preset original;
        original.schemaVersion = 5;
        original.name = "Full V5 Preset";
        original.harmony.setLiveRevoice(true);
        original.harmony.setQualityRule(QualityRule::minor);

        VoicingSpec s0;
        s0.shape = ChordShape::ninth;
        s0.style = VoicingStyle::open;
        s0.bassMode = BassMode::root;
        original.harmony.getConfiguration().setSpec(0, s0);

        VoicingSpec s6;
        s6.shape = ChordShape::thirteenth;
        s6.style = VoicingStyle::rootless;
        s6.voiceLeading = VoiceLeadingMode::nearest;
        original.harmony.getConfiguration().setSpec(6, s6);

        auto json = PresetSerializer::toJson(original);
        REQUIRE_FALSE(json.contains("selected_scene"));
        REQUIRE_FALSE(json.contains("\"scenes\""));
        REQUIRE(json.contains("\"degrees\""));

        auto restored = PresetSerializer::fromJson(json);
        REQUIRE(restored.has_value());
        REQUIRE(restored->schemaVersion == 5);
        REQUIRE(restored->name == "Full V5 Preset");
        REQUIRE(restored->harmony.getLiveRevoice() == true);
        REQUIRE(restored->harmony.getQualityRule() == QualityRule::minor);

        const auto& r0 = restored->harmony.getConfiguration().getSpec(0);
        REQUIRE(r0.shape == ChordShape::ninth);
        REQUIRE(r0.style == VoicingStyle::open);
        REQUIRE(r0.bassMode == BassMode::root);

        const auto& r6 = restored->harmony.getConfiguration().getSpec(6);
        REQUIRE(r6.shape == ChordShape::thirteenth);
        REQUIRE(r6.style == VoicingStyle::rootless);
        REQUIRE(r6.voiceLeading == VoiceLeadingMode::nearest);
    }

    SECTION("Malformed enum values in schema 5 degrees are sanitized safely") {
        juce::String malformedV5 = R"({
            "schema_version": 5,
            "name": "Malformed V5",
            "parameters": {
                "key": 0
            },
            "harmony": {
                "live_revoice": true,
                "quality_rule": 99,
                "degrees": [
                    {
                        "index": 0,
                        "shape": 999,
                        "inversion": -10,
                        "style": -5,
                        "base_octave": 10,
                        "quality_rule": 88,
                        "fifth_policy": 77,
                        "bass_mode": 66,
                        "slash_degree": -3,
                        "voice_leading": 55
                    }
                ]
            }
        })";

        auto result = PresetSerializer::fromJson(malformedV5);
        REQUIRE(result.has_value());
        const auto& p = *result;
        REQUIRE(p.harmony.getQualityRule() == QualityRule::diatonic);

        const auto& spec0 = p.harmony.getConfiguration().getSpec(0);
        REQUIRE(spec0.shape == ChordShape::triad);
        REQUIRE(spec0.inversion == 0);
        REQUIRE(spec0.style == VoicingStyle::compact);
        REQUIRE(spec0.baseOctave == 4);
        REQUIRE(spec0.qualityRule == QualityRule::diatonic);
        REQUIRE(spec0.fifthPolicy == FifthPolicy::automatic);
        REQUIRE(spec0.bassMode == BassMode::none);
        REQUIRE(spec0.slashDegree == 0);
        REQUIRE(spec0.voiceLeading == VoiceLeadingMode::manual);
    }
}
