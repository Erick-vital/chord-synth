#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include "presets/Preset.h"
#include "presets/PresetSerializer.h"
#include "parameters/ParameterIds.h"
#include "plugin/PluginProcessor.h"

using namespace chordsynth;
using namespace chordsynth::presets;

TEST_CASE("PresetSerializer round-trip, validation, and versioning", "[presets]") {
    SECTION("Valid preset serializes to JSON and deserializes correctly") {
        Preset original;
        original.schemaVersion = 5;
        original.name = "Warm C";
        original.parameters.key = 0;
        original.parameters.scale = 1;
        original.parameters.waveform = "saw";
        original.parameters.attackMs = 5.0f;
        original.parameters.decayMs = 80.0f;
        original.parameters.sustain = 0.8f;
        original.parameters.releaseMs = 120.0f;
        original.parameters.cutoffHz = 8000.0f;
        original.parameters.resonance = 0.2f;
        original.parameters.detuneCents = 7.0f;
        original.parameters.chorusMix = 0.4f;
        original.parameters.chorusRateHz = 2.0f;
        original.parameters.chorusDepth = 0.6f;
        original.parameters.delayMix = 0.5f;
        original.parameters.delayFeedback = 0.4f;
        original.parameters.delayTimeMs = 300.0f;
        original.parameters.delaySync = true;
        original.parameters.delaySyncRate = 1;
        original.parameters.reverbMix = 0.35f;
        original.parameters.reverbRoomSize = 0.7f;
        original.parameters.reverbDamping = 0.4f;
        original.parameters.reverbWidth = 0.9f;
        original.parameters.arpEnabled = true;
        original.parameters.arpMode = 2; // Up/Down
        original.parameters.arpRate = 1; // 1/8
        original.parameters.arpGate = 0.75f;
        original.parameters.masterGainDb = -12.0f;

        auto jsonString = PresetSerializer::toJson(original);
        REQUIRE_FALSE(jsonString.isEmpty());

        auto result = PresetSerializer::fromJson(jsonString);
        REQUIRE(result.has_value());
        const auto& restored = *result;
        REQUIRE(restored.schemaVersion == 5);
        REQUIRE(restored.name == "Warm C");
        REQUIRE(restored.parameters.key == 0);
        REQUIRE(restored.parameters.scale == 1);
        REQUIRE(restored.parameters.waveform == "saw");
        REQUIRE(restored.parameters.attackMs == Catch::Approx(5.0f));
        REQUIRE(restored.parameters.decayMs == Catch::Approx(80.0f));
        REQUIRE(restored.parameters.sustain == Catch::Approx(0.8f));
        REQUIRE(restored.parameters.releaseMs == Catch::Approx(120.0f));
        REQUIRE(restored.parameters.cutoffHz == Catch::Approx(8000.0f));
        REQUIRE(restored.parameters.resonance == Catch::Approx(0.2f));
        REQUIRE(restored.parameters.detuneCents == Catch::Approx(7.0f));
        REQUIRE(restored.parameters.chorusMix == Catch::Approx(0.4f));
        REQUIRE(restored.parameters.chorusRateHz == Catch::Approx(2.0f));
        REQUIRE(restored.parameters.chorusDepth == Catch::Approx(0.6f));
        REQUIRE(restored.parameters.delayMix == Catch::Approx(0.5f));
        REQUIRE(restored.parameters.delayFeedback == Catch::Approx(0.4f));
        REQUIRE(restored.parameters.delayTimeMs == Catch::Approx(300.0f));
        REQUIRE(restored.parameters.delaySync == true);
        REQUIRE(restored.parameters.delaySyncRate == 1);
        REQUIRE(restored.parameters.reverbMix == Catch::Approx(0.35f));
        REQUIRE(restored.parameters.reverbRoomSize == Catch::Approx(0.7f));
        REQUIRE(restored.parameters.reverbDamping == Catch::Approx(0.4f));
        REQUIRE(restored.parameters.reverbWidth == Catch::Approx(0.9f));
        REQUIRE(restored.parameters.arpEnabled == true);
        REQUIRE(restored.parameters.arpMode == 2);
        REQUIRE(restored.parameters.arpRate == 1);
        REQUIRE(restored.parameters.arpGate == Catch::Approx(0.75f));
        REQUIRE(restored.parameters.masterGainDb == Catch::Approx(-12.0f));
    }

    SECTION("Missing required fields or wrong root fails gracefully") {
        juce::String invalidJson = R"({ "schema_version": 1, "name": "Broken" })";
        auto result = PresetSerializer::fromJson(invalidJson);
        REQUIRE_FALSE(result.has_value());
    }

    SECTION("Out-of-range values are clamped or sanitized safely") {
        juce::String outOfBoundsJson = R"({
            "schema_version": 1,
            "name": "Extreme",
            "parameters": {
                "key": 99,
                "waveform": "invalid_shape",
                "attack_ms": -50.0,
                "decay_ms": 99999.0,
                "sustain": 2.5,
                "release_ms": -10.0,
                "cutoff_hz": 50000.0,
                "resonance": 10.0,
                "detune_cents": 50.0,
                "master_gain_db": 100.0
            }
        })";
        auto result = PresetSerializer::fromJson(outOfBoundsJson);
        REQUIRE(result.has_value());
        const auto& p = *result;
        REQUIRE(p.parameters.key == 11); // clamped to 0..11
        REQUIRE(p.parameters.waveform == "sine"); // fallback for invalid waveform
        REQUIRE(p.parameters.cutoffHz == Catch::Approx(20000.0f));
        REQUIRE(p.parameters.resonance == Catch::Approx(2.0f));
        REQUIRE(p.parameters.detuneCents == Catch::Approx(20.0f));
    }

    SECTION("Legacy schema version 1 preset deserializes with default harmony settings") {
        juce::String v1Json = R"({
            "schema_version": 1,
            "name": "Legacy V1",
            "parameters": {
                "key": 4,
                "waveform": "saw",
                "cutoff_hz": 4000.0
            }
        })";
        auto result = PresetSerializer::fromJson(v1Json);
        REQUIRE(result.has_value());
        const auto& p = *result;
        REQUIRE(p.schemaVersion == 1);
        REQUIRE(p.name == "Legacy V1");
        REQUIRE(p.parameters.key == 4);
        REQUIRE(p.parameters.scale == 0);
        REQUIRE(p.parameters.waveform == "saw");
        REQUIRE(p.parameters.cutoffHz == Catch::Approx(4000.0f));
        REQUIRE(p.harmony.getLiveRevoice() == false);
        REQUIRE(p.harmony.getQualityRule() == music::QualityRule::diatonic);
        REQUIRE(p.harmony.getConfiguration().getSpec(0).shape == music::ChordShape::triad);
        REQUIRE(p.harmony.getConfiguration().getSpec(0).style == music::VoicingStyle::compact);
    }

    SECTION("Legacy schema version 2 preset serializes and deserializes harmony state faithfully") {
        Preset original;
        original.schemaVersion = 5;
        original.name = "Custom Harmony Preset";
        original.parameters.key = 2; // D
        original.parameters.waveform = "triangle";
        original.harmony.setLiveRevoice(true);
        original.harmony.setQualityRule(music::QualityRule::major);

        music::VoicingSpec customSpec{
            .extension = music::ChordExtension::seventh,
            .inversion = 1,
            .style = music::VoicingStyle::open,
            .baseOctave = 4,
            .qualityRule = music::QualityRule::minor
        };
        original.harmony.getConfiguration().setSpec(1, customSpec); // Degree ii

        // Manual v2 JSON construct to test v2 deserialization
        juce::String v2Json = R"({
            "schema_version": 2,
            "name": "Custom Harmony Preset",
            "parameters": {
                "key": 2,
                "waveform": "triangle"
            },
            "harmony": {
                "selected_scene": 2,
                "live_revoice": true,
                "quality_rule": 1,
                "scenes": [
                    {
                        "index": 2,
                        "degrees": [
                            {
                                "index": 1,
                                "extension": 1,
                                "inversion": 1,
                                "style": 1,
                                "base_octave": 4,
                                "quality_rule": 2
                            }
                        ]
                    }
                ]
            }
        })";

        auto result = PresetSerializer::fromJson(v2Json);
        REQUIRE(result.has_value());
        const auto& restored = *result;
        REQUIRE(restored.schemaVersion == 2);
        REQUIRE(restored.name == "Custom Harmony Preset");
        REQUIRE(restored.parameters.key == 2);
        REQUIRE(restored.parameters.waveform == "triangle");
        REQUIRE(restored.harmony.getLiveRevoice() == true);
        REQUIRE(restored.harmony.getQualityRule() == music::QualityRule::major);
        // Degree 1 was extracted from selected_scene=2
        REQUIRE(restored.harmony.getConfiguration().getSpec(1).shape == music::ChordShape::seventh);
        REQUIRE(restored.harmony.getConfiguration().getSpec(1).style == music::VoicingStyle::open);
    }

    SECTION("Schema version 5 preset serializes and deserializes all extended harmony fields faithfully") {
        Preset original;
        original.schemaVersion = 5;
        original.name = "Extended Harmony V5";
        original.parameters.key = 5; // F
        original.parameters.waveform = "saw";
        original.parameters.performanceMidiEnabled = true;
        original.parameters.transformPalette = 2;
        original.harmony.setLiveRevoice(true);
        original.harmony.setQualityRule(music::QualityRule::dominant);

        music::VoicingSpec customSpec{
            .shape = music::ChordShape::thirteenth,
            .extension = music::ChordExtension::seventh,
            .inversion = 2,
            .style = music::VoicingStyle::rootless,
            .fifthPolicy = music::FifthPolicy::omit,
            .bassMode = music::BassMode::slashDegree,
            .slashDegree = 4,
            .voiceLeading = music::VoiceLeadingMode::nearest,
            .baseOctave = 4,
            .qualityRule = music::QualityRule::dominant
        };
        original.harmony.getConfiguration().setSpec(4, customSpec);

        auto jsonString = PresetSerializer::toJson(original);
        REQUIRE_FALSE(jsonString.isEmpty());

        auto result = PresetSerializer::fromJson(jsonString);
        REQUIRE(result.has_value());
        const auto& restored = *result;
        REQUIRE(restored.schemaVersion == 5);
        REQUIRE(restored.name == "Extended Harmony V5");
        REQUIRE(restored.parameters.key == 5);
        REQUIRE(restored.parameters.waveform == "saw");
        REQUIRE(restored.parameters.performanceMidiEnabled);
        REQUIRE(restored.parameters.transformPalette == 2);
        REQUIRE(restored.harmony.getLiveRevoice() == true);
        REQUIRE(restored.harmony.getQualityRule() == music::QualityRule::dominant);

        const auto& readSpec = restored.harmony.getConfiguration().getSpec(4);
        REQUIRE(readSpec.shape == music::ChordShape::thirteenth);
        REQUIRE(readSpec.inversion == 2);
        REQUIRE(readSpec.style == music::VoicingStyle::rootless);
        REQUIRE(readSpec.baseOctave == 4);
        REQUIRE(readSpec.qualityRule == music::QualityRule::dominant);
        REQUIRE(readSpec.fifthPolicy == music::FifthPolicy::omit);
        REQUIRE(readSpec.bassMode == music::BassMode::slashDegree);
        REQUIRE(readSpec.slashDegree == 4);
        REQUIRE(readSpec.voiceLeading == music::VoiceLeadingMode::nearest);
    }

    SECTION("Legacy schema version 2 JSON maps extension/style and populates compatibility defaults") {
        juce::String v2Json = R"({
            "schema_version": 2,
            "name": "Legacy V2 Preset",
            "parameters": {
                "key": 3
            },
            "harmony": {
                "selected_scene": 1,
                "live_revoice": false,
                "quality_rule": 1,
                "scenes": [
                    {
                        "index": 1,
                        "degrees": [
                            {
                                "index": 2,
                                "extension": 1,
                                "inversion": 1,
                                "style": 1,
                                "base_octave": 3,
                                "quality_rule": 2
                            }
                        ]
                    }
                ]
            }
        })";

        auto result = PresetSerializer::fromJson(v2Json);
        REQUIRE(result.has_value());
        const auto& p = *result;
        REQUIRE(p.schemaVersion == 2);
        REQUIRE(p.name == "Legacy V2 Preset");
        REQUIRE(p.harmony.getQualityRule() == music::QualityRule::major);

        const auto& spec = p.harmony.getConfiguration().getSpec(2);
        REQUIRE(spec.shape == music::ChordShape::seventh);
        REQUIRE(spec.extension == music::ChordExtension::seventh);
        REQUIRE(spec.inversion == 1);
        REQUIRE(spec.style == music::VoicingStyle::open);
        REQUIRE(spec.baseOctave == 3);
        REQUIRE(spec.qualityRule == music::QualityRule::minor);
        REQUIRE(spec.fifthPolicy == music::FifthPolicy::automatic);
        REQUIRE(spec.bassMode == music::BassMode::none);
        REQUIRE(spec.slashDegree == 0);
        REQUIRE(spec.voiceLeading == music::VoiceLeadingMode::manual);
    }

    SECTION("Schema version 3 with malformed enum/degree values sanitizes safely") {
        juce::String v3Malformed = R"({
            "schema_version": 3,
            "name": "Malformed Enums",
            "parameters": {
                "key": 0
            },
            "harmony": {
                "selected_scene": 99,
                "quality_rule": 88,
                "scenes": [
                    {
                        "index": 3,
                        "degrees": [
                            {
                                "index": 0,
                                "shape": 99,
                                "inversion": -5,
                                "style": 42,
                                "base_octave": 10,
                                "quality_rule": 77,
                                "fifth_policy": 88,
                                "bass_mode": 99,
                                "slash_degree": -1,
                                "voice_leading": 12
                            }
                        ]
                    }
                ]
            }
        })";

        auto result = PresetSerializer::fromJson(v3Malformed);
        REQUIRE(result.has_value());
        const auto& p = *result;
        REQUIRE(p.harmony.getQualityRule() == music::QualityRule::diatonic); // fallback

        const auto& spec = p.harmony.getConfiguration().getSpec(0);
        REQUIRE(spec.shape == music::ChordShape::triad);
        REQUIRE(spec.inversion == 0);
        REQUIRE(spec.style == music::VoicingStyle::compact);
        REQUIRE(spec.baseOctave == 4); // clamped to 2..4
        REQUIRE(spec.qualityRule == music::QualityRule::diatonic);
        REQUIRE(spec.fifthPolicy == music::FifthPolicy::automatic);
        REQUIRE(spec.bassMode == music::BassMode::none);
        REQUIRE(spec.slashDegree == 0);
        REQUIRE(spec.voiceLeading == music::VoiceLeadingMode::manual);
    }

    SECTION("Unsupported schema version 6 fails deserialization") {
        juce::String futureV6 = R"({ "schema_version": 6, "name": "Future V6", "parameters": {} })";
        auto result = PresetSerializer::fromJson(futureV6);
        REQUIRE_FALSE(result.has_value());
    }

    SECTION("Schema version 2 with malformed or missing harmony section falls back to defaults safely") {
        juce::String v2MissingHarmony = R"({
            "schema_version": 2,
            "name": "Partial V2",
            "parameters": {
                "key": 0
            }
        })";
        auto result = PresetSerializer::fromJson(v2MissingHarmony);
        REQUIRE(result.has_value());
        REQUIRE(result->harmony.getLiveRevoice() == false);
    }

    SECTION("Unsupported schema version fails deserialization") {
        juce::String futureJson = R"({ "schema_version": 999, "name": "Future", "parameters": {} })";
        auto result = PresetSerializer::fromJson(futureJson);
        REQUIRE_FALSE(result.has_value());
    }

    SECTION("Malformed JSON string fails deserialization without throwing or crashing") {
        juce::String malformedJson = "{ schema_version: 1, name: unclosed... ";
        auto result = PresetSerializer::fromJson(malformedJson);
        REQUIRE_FALSE(result.has_value());
    }
}

TEST_CASE("PresetSerializer can load and store APVTS state", "[presets][apvts]") {
    ChordSynthAudioProcessor processor;
    auto* keyParam = dynamic_cast<juce::AudioParameterChoice*>(
        processor.getAPVTS().getParameter(parameters::ids::key));
    auto* scaleParam = dynamic_cast<juce::AudioParameterChoice*>(
        processor.getAPVTS().getParameter(parameters::ids::scale));
    auto* waveParam = dynamic_cast<juce::AudioParameterChoice*>(
        processor.getAPVTS().getParameter(parameters::ids::waveform));
    auto* cutoffParam = dynamic_cast<juce::AudioParameterFloat*>(
        processor.getAPVTS().getParameter(parameters::ids::cutoff));
    auto* resParam = dynamic_cast<juce::AudioParameterFloat*>(
        processor.getAPVTS().getParameter(parameters::ids::resonance));
    auto* detuneParam = dynamic_cast<juce::AudioParameterFloat*>(
        processor.getAPVTS().getParameter(parameters::ids::detune));

    auto* chorusMix = dynamic_cast<juce::AudioParameterFloat*>(
        processor.getAPVTS().getParameter(parameters::ids::chorusMix));
    auto* chorusRate = dynamic_cast<juce::AudioParameterFloat*>(
        processor.getAPVTS().getParameter(parameters::ids::chorusRate));
    auto* chorusDepth = dynamic_cast<juce::AudioParameterFloat*>(
        processor.getAPVTS().getParameter(parameters::ids::chorusDepth));

    auto* delayMix = dynamic_cast<juce::AudioParameterFloat*>(
        processor.getAPVTS().getParameter(parameters::ids::delayMix));
    auto* delayFeedback = dynamic_cast<juce::AudioParameterFloat*>(
        processor.getAPVTS().getParameter(parameters::ids::delayFeedback));
    auto* delayTimeMs = dynamic_cast<juce::AudioParameterFloat*>(
        processor.getAPVTS().getParameter(parameters::ids::delayTimeMs));
    auto* delaySync = dynamic_cast<juce::AudioParameterBool*>(
        processor.getAPVTS().getParameter(parameters::ids::delaySync));
    auto* delaySyncRate = dynamic_cast<juce::AudioParameterChoice*>(
        processor.getAPVTS().getParameter(parameters::ids::delaySyncRate));

    auto* reverbMix = dynamic_cast<juce::AudioParameterFloat*>(
        processor.getAPVTS().getParameter(parameters::ids::reverbMix));
    auto* reverbRoomSize = dynamic_cast<juce::AudioParameterFloat*>(
        processor.getAPVTS().getParameter(parameters::ids::reverbRoomSize));
    auto* reverbDamping = dynamic_cast<juce::AudioParameterFloat*>(
        processor.getAPVTS().getParameter(parameters::ids::reverbDamping));
    auto* reverbWidth = dynamic_cast<juce::AudioParameterFloat*>(
        processor.getAPVTS().getParameter(parameters::ids::reverbWidth));

    auto* arpEnabled = dynamic_cast<juce::AudioParameterBool*>(
        processor.getAPVTS().getParameter(parameters::ids::arpEnabled));
    auto* arpMode = dynamic_cast<juce::AudioParameterChoice*>(
        processor.getAPVTS().getParameter(parameters::ids::arpMode));
    auto* arpRate = dynamic_cast<juce::AudioParameterChoice*>(
        processor.getAPVTS().getParameter(parameters::ids::arpRate));
    auto* arpGate = dynamic_cast<juce::AudioParameterFloat*>(
        processor.getAPVTS().getParameter(parameters::ids::arpGate));

    REQUIRE(keyParam != nullptr);
    REQUIRE(scaleParam != nullptr);
    REQUIRE(waveParam != nullptr);
    REQUIRE(cutoffParam != nullptr);
    REQUIRE(resParam != nullptr);
    REQUIRE(detuneParam != nullptr);
    REQUIRE(chorusMix != nullptr);
    REQUIRE(chorusRate != nullptr);
    REQUIRE(chorusDepth != nullptr);
    REQUIRE(delayMix != nullptr);
    REQUIRE(delayFeedback != nullptr);
    REQUIRE(delayTimeMs != nullptr);
    REQUIRE(delaySync != nullptr);
    REQUIRE(delaySyncRate != nullptr);
    REQUIRE(reverbMix != nullptr);
    REQUIRE(reverbRoomSize != nullptr);
    REQUIRE(reverbDamping != nullptr);
    REQUIRE(reverbWidth != nullptr);
    REQUIRE(arpEnabled != nullptr);
    REQUIRE(arpMode != nullptr);
    REQUIRE(arpRate != nullptr);
    REQUIRE(arpGate != nullptr);

    *keyParam = 2; // D
    *scaleParam = 1; // Natural minor
    *waveParam = 2; // Square
    *cutoffParam = 3500.0f;
    *resParam = 0.8f;
    *detuneParam = 12.0f;
    *chorusMix = 0.65f;
    *chorusRate = 3.0f;
    *chorusDepth = 0.7f;
    *delayMix = 0.45f;
    *delayFeedback = 0.55f;
    *delayTimeMs = 400.0f;
    *delaySync = false;
    *delaySyncRate = 2;
    *reverbMix = 0.4f;
    *reverbRoomSize = 0.8f;
    *reverbDamping = 0.3f;
    *reverbWidth = 0.85f;
    *arpEnabled = true;
    *arpMode = 1; // Down
    *arpRate = 0; // 1/4
    *arpGate = 0.65f;

    auto preset = PresetSerializer::fromAPVTS(processor.getAPVTS(), "Exported");
    REQUIRE(preset.name == "Exported");
    REQUIRE(preset.parameters.key == 2);
    REQUIRE(preset.parameters.scale == 1);
    REQUIRE(preset.parameters.waveform == "square");
    REQUIRE(preset.parameters.cutoffHz == Catch::Approx(3500.0f));
    REQUIRE(preset.parameters.resonance == Catch::Approx(0.8f));
    REQUIRE(preset.parameters.detuneCents == Catch::Approx(12.0f));
    REQUIRE(preset.parameters.chorusMix == Catch::Approx(0.65f));
    REQUIRE(preset.parameters.chorusRateHz == Catch::Approx(3.0f));
    REQUIRE(preset.parameters.chorusDepth == Catch::Approx(0.7f));
    REQUIRE(preset.parameters.delayMix == Catch::Approx(0.45f));
    REQUIRE(preset.parameters.delayFeedback == Catch::Approx(0.55f));
    REQUIRE(preset.parameters.delayTimeMs == Catch::Approx(400.0f));
    REQUIRE(preset.parameters.delaySync == false);
    REQUIRE(preset.parameters.delaySyncRate == 2);
    REQUIRE(preset.parameters.reverbMix == Catch::Approx(0.4f));
    REQUIRE(preset.parameters.reverbRoomSize == Catch::Approx(0.8f));
    REQUIRE(preset.parameters.reverbDamping == Catch::Approx(0.3f));
    REQUIRE(preset.parameters.reverbWidth == Catch::Approx(0.85f));
    REQUIRE(preset.parameters.arpEnabled == true);
    REQUIRE(preset.parameters.arpMode == 1);
    REQUIRE(preset.parameters.arpRate == 0);
    REQUIRE(preset.parameters.arpGate == Catch::Approx(0.65f));

    ChordSynthAudioProcessor targetProcessor;
    bool applied = PresetSerializer::applyToAPVTS(preset, targetProcessor.getAPVTS());
    REQUIRE(applied);

    auto* targetKey = dynamic_cast<juce::AudioParameterChoice*>(
        targetProcessor.getAPVTS().getParameter(parameters::ids::key));
    auto* targetScale = dynamic_cast<juce::AudioParameterChoice*>(
        targetProcessor.getAPVTS().getParameter(parameters::ids::scale));
    auto* targetWave = dynamic_cast<juce::AudioParameterChoice*>(
        targetProcessor.getAPVTS().getParameter(parameters::ids::waveform));
    auto* targetCutoff = dynamic_cast<juce::AudioParameterFloat*>(
        targetProcessor.getAPVTS().getParameter(parameters::ids::cutoff));
    auto* targetRes = dynamic_cast<juce::AudioParameterFloat*>(
        targetProcessor.getAPVTS().getParameter(parameters::ids::resonance));
    auto* targetDetune = dynamic_cast<juce::AudioParameterFloat*>(
        targetProcessor.getAPVTS().getParameter(parameters::ids::detune));
    auto* targetMix = dynamic_cast<juce::AudioParameterFloat*>(
        targetProcessor.getAPVTS().getParameter(parameters::ids::chorusMix));
    auto* targetRate = dynamic_cast<juce::AudioParameterFloat*>(
        targetProcessor.getAPVTS().getParameter(parameters::ids::chorusRate));
    auto* targetDepth = dynamic_cast<juce::AudioParameterFloat*>(
        targetProcessor.getAPVTS().getParameter(parameters::ids::chorusDepth));
    auto* targetDelayMix = dynamic_cast<juce::AudioParameterFloat*>(
        targetProcessor.getAPVTS().getParameter(parameters::ids::delayMix));
    auto* targetDelayFeedback = dynamic_cast<juce::AudioParameterFloat*>(
        targetProcessor.getAPVTS().getParameter(parameters::ids::delayFeedback));
    auto* targetDelayTimeMs = dynamic_cast<juce::AudioParameterFloat*>(
        targetProcessor.getAPVTS().getParameter(parameters::ids::delayTimeMs));
    auto* targetDelaySync = dynamic_cast<juce::AudioParameterBool*>(
        targetProcessor.getAPVTS().getParameter(parameters::ids::delaySync));
    auto* targetDelaySyncRate = dynamic_cast<juce::AudioParameterChoice*>(
        targetProcessor.getAPVTS().getParameter(parameters::ids::delaySyncRate));
    auto* targetReverbMix = dynamic_cast<juce::AudioParameterFloat*>(
        targetProcessor.getAPVTS().getParameter(parameters::ids::reverbMix));
    auto* targetReverbRoomSize = dynamic_cast<juce::AudioParameterFloat*>(
        targetProcessor.getAPVTS().getParameter(parameters::ids::reverbRoomSize));
    auto* targetReverbDamping = dynamic_cast<juce::AudioParameterFloat*>(
        targetProcessor.getAPVTS().getParameter(parameters::ids::reverbDamping));
    auto* targetReverbWidth = dynamic_cast<juce::AudioParameterFloat*>(
        targetProcessor.getAPVTS().getParameter(parameters::ids::reverbWidth));
    auto* targetArpEnabled = dynamic_cast<juce::AudioParameterBool*>(
        targetProcessor.getAPVTS().getParameter(parameters::ids::arpEnabled));
    auto* targetArpMode = dynamic_cast<juce::AudioParameterChoice*>(
        targetProcessor.getAPVTS().getParameter(parameters::ids::arpMode));
    auto* targetArpRate = dynamic_cast<juce::AudioParameterChoice*>(
        targetProcessor.getAPVTS().getParameter(parameters::ids::arpRate));
    auto* targetArpGate = dynamic_cast<juce::AudioParameterFloat*>(
        targetProcessor.getAPVTS().getParameter(parameters::ids::arpGate));

    REQUIRE(targetKey != nullptr);
    REQUIRE(targetScale != nullptr);
    REQUIRE(targetWave != nullptr);
    REQUIRE(targetCutoff != nullptr);
    REQUIRE(targetRes != nullptr);
    REQUIRE(targetDetune != nullptr);
    REQUIRE(targetMix != nullptr);
    REQUIRE(targetRate != nullptr);
    REQUIRE(targetDepth != nullptr);
    REQUIRE(targetDelayMix != nullptr);
    REQUIRE(targetDelayFeedback != nullptr);
    REQUIRE(targetDelayTimeMs != nullptr);
    REQUIRE(targetDelaySync != nullptr);
    REQUIRE(targetDelaySyncRate != nullptr);
    REQUIRE(targetReverbMix != nullptr);
    REQUIRE(targetReverbRoomSize != nullptr);
    REQUIRE(targetReverbDamping != nullptr);
    REQUIRE(targetReverbWidth != nullptr);
    REQUIRE(targetArpEnabled != nullptr);
    REQUIRE(targetArpMode != nullptr);
    REQUIRE(targetArpRate != nullptr);
    REQUIRE(targetArpGate != nullptr);

    REQUIRE(targetKey->getIndex() == 2);
    REQUIRE(targetScale->getIndex() == 1);
    REQUIRE(targetWave->getIndex() == 2);
    REQUIRE(static_cast<float>(*targetCutoff) == Catch::Approx(3500.0f));
    REQUIRE(static_cast<float>(*targetRes) == Catch::Approx(0.8f));
    REQUIRE(static_cast<float>(*targetDetune) == Catch::Approx(12.0f));
    REQUIRE(static_cast<float>(*targetMix) == Catch::Approx(0.65f));
    REQUIRE(static_cast<float>(*targetRate) == Catch::Approx(3.0f));
    REQUIRE(static_cast<float>(*targetDepth) == Catch::Approx(0.7f));
    REQUIRE(static_cast<float>(*targetDelayMix) == Catch::Approx(0.45f));
    REQUIRE(static_cast<float>(*targetDelayFeedback) == Catch::Approx(0.55f));
    REQUIRE(static_cast<float>(*targetDelayTimeMs) == Catch::Approx(400.0f));
    REQUIRE(*targetDelaySync == false);
    REQUIRE(targetDelaySyncRate->getIndex() == 2);
    REQUIRE(static_cast<float>(*targetReverbMix) == Catch::Approx(0.4f));
    REQUIRE(static_cast<float>(*targetReverbRoomSize) == Catch::Approx(0.8f));
    REQUIRE(static_cast<float>(*targetReverbDamping) == Catch::Approx(0.3f));
    REQUIRE(static_cast<float>(*targetReverbWidth) == Catch::Approx(0.85f));
    REQUIRE(*targetArpEnabled == true);
    REQUIRE(targetArpMode->getIndex() == 1);
    REQUIRE(targetArpRate->getIndex() == 0);
    SECTION("PresetSerializer supports processor harmony state load and store") {
        ChordSynthAudioProcessor srcProcessor;
        srcProcessor.getHarmonyState().setLiveRevoice(true);
        srcProcessor.getHarmonyState().setQualityRule(music::QualityRule::diminished);

        music::VoicingSpec spec{
            .extension = music::ChordExtension::seventh,
            .inversion = 2,
            .style = music::VoicingStyle::open,
            .baseOctave = 2,
            .qualityRule = music::QualityRule::major
        };
        srcProcessor.getHarmonyState().getConfiguration().setSpec(4, spec);

        auto preset = PresetSerializer::fromProcessorState(
            srcProcessor.getAPVTS(),
            srcProcessor.getHarmonyState(),
            "HarmonyPreset");

        REQUIRE(preset.schemaVersion == 5);
        REQUIRE(preset.name == "HarmonyPreset");
        REQUIRE(preset.harmony.getLiveRevoice() == true);
        REQUIRE(preset.harmony.getQualityRule() == music::QualityRule::diminished);
        REQUIRE(preset.harmony.getConfiguration().getSpec(4) == spec);

        ChordSynthAudioProcessor dstProcessor;
        bool applied = PresetSerializer::applyToProcessorState(
            preset,
            dstProcessor.getAPVTS(),
            dstProcessor.getHarmonyState());

        REQUIRE(applied);
        REQUIRE(dstProcessor.getHarmonyState().getLiveRevoice() == true);
        REQUIRE(dstProcessor.getHarmonyState().getQualityRule() == music::QualityRule::diminished);
        REQUIRE(dstProcessor.getHarmonyState().getConfiguration().getSpec(4) == spec);
    }

    SECTION("Built-in musical presets demonstrate factory configurations and state synchronization") {
        ChordSynthAudioProcessor processor;

        // Preset 1: Default (Init) -> Diatonic compact triads
        Preset initPreset;
        initPreset.name = "Default (Init)";
        initPreset.parameters.key = 0;
        initPreset.parameters.waveform = "sine";
        initPreset.parameters.cutoffHz = 8000.0f;
        initPreset.parameters.resonance = 0.2f;
        initPreset.parameters.detuneCents = 7.0f;
        initPreset.parameters.masterGainDb = -12.0f;
        initPreset.harmony.setLiveRevoice(false);
        initPreset.harmony.setConfiguration(music::HarmonyConfiguration::makeDiatonic());

        REQUIRE(PresetSerializer::applyToProcessorState(initPreset, processor.getAPVTS(), processor.getHarmonyState()));
        REQUIRE_FALSE(processor.getHarmonyState().getLiveRevoice());
        for (int deg = 0; deg < 7; ++deg) {
            const auto& spec = processor.getHarmonyState().getConfiguration().getSpec(deg);
            REQUIRE(spec.shape == music::ChordShape::triad);
            REQUIRE(spec.style == music::VoicingStyle::compact);
            REQUIRE(spec.bassMode == music::BassMode::none);
        }

        // Preset 2: Warm Saw Chords -> Lo-Fi Warm recipe
        Preset warmPreset;
        warmPreset.name = "Warm Saw Chords";
        warmPreset.parameters.key = 0;
        warmPreset.parameters.waveform = "saw";
        warmPreset.parameters.cutoffHz = 3200.0f;
        warmPreset.parameters.resonance = 0.4f;
        warmPreset.parameters.detuneCents = 12.0f;
        warmPreset.parameters.chorusMix = 0.4f;
        warmPreset.parameters.reverbMix = 0.3f;
        warmPreset.parameters.masterGainDb = -12.0f;
        warmPreset.harmony.setLiveRevoice(false);
        warmPreset.harmony.setConfiguration(music::HarmonyConfiguration::makeLofiWarm());

        REQUIRE(PresetSerializer::applyToProcessorState(warmPreset, processor.getAPVTS(), processor.getHarmonyState()));
        REQUIRE_FALSE(processor.getHarmonyState().getLiveRevoice());
        // Verify characteristic Lo-Fi Warm per-degree voicing specs
        const auto& warmConfig = processor.getHarmonyState().getConfiguration();
        REQUIRE(warmConfig.getSpec(0).shape == music::ChordShape::ninth);
        REQUIRE(warmConfig.getSpec(0).style == music::VoicingStyle::open);
        REQUIRE(warmConfig.getSpec(0).bassMode == music::BassMode::root);
        REQUIRE(warmConfig.getSpec(3).shape == music::ChordShape::ninth); // IV
        REQUIRE(warmConfig.getSpec(3).style == music::VoicingStyle::open);
        REQUIRE(warmConfig.getSpec(4).shape == music::ChordShape::thirteenth); // V
        REQUIRE(warmConfig.getSpec(4).style == music::VoicingStyle::open);

        // Preset 3: Ambient Open Keys -> Lo-Fi Warm with Live Revoice
        Preset ambientPreset;
        ambientPreset.name = "Ambient Open Keys";
        ambientPreset.parameters.key = 0;
        ambientPreset.parameters.waveform = "triangle";
        ambientPreset.parameters.cutoffHz = 4500.0f;
        ambientPreset.parameters.resonance = 0.3f;
        ambientPreset.parameters.detuneCents = 8.0f;
        ambientPreset.parameters.delayMix = 0.35f;
        ambientPreset.parameters.delayFeedback = 0.4f;
        ambientPreset.parameters.reverbMix = 0.5f;
        ambientPreset.parameters.masterGainDb = -12.0f;
        ambientPreset.harmony.setLiveRevoice(true);
        ambientPreset.harmony.setConfiguration(music::HarmonyConfiguration::makeLofiWarm());

        REQUIRE(PresetSerializer::applyToProcessorState(ambientPreset, processor.getAPVTS(), processor.getHarmonyState()));
        REQUIRE(processor.getHarmonyState().getLiveRevoice());
        REQUIRE(processor.getHarmonyState().getConfiguration() == music::HarmonyConfiguration::makeLofiWarm());

        // Preset 4: Arp Plucks -> Diatonic sevenths with Arp enabled
        Preset arpPreset;
        arpPreset.name = "Arp Plucks";
        arpPreset.parameters.key = 0;
        arpPreset.parameters.waveform = "square";
        arpPreset.parameters.cutoffHz = 2400.0f;
        arpPreset.parameters.resonance = 0.8f;
        arpPreset.parameters.detuneCents = 5.0f;
        arpPreset.parameters.arpEnabled = true;
        arpPreset.parameters.arpMode = 0;
        arpPreset.parameters.arpRate = 1;
        arpPreset.parameters.arpGate = 0.7f;
        arpPreset.parameters.delayMix = 0.25f;
        arpPreset.parameters.masterGainDb = -12.0f;
        arpPreset.harmony.setLiveRevoice(false);
        arpPreset.harmony.setConfiguration(music::HarmonyConfiguration::makeSevenths());

        REQUIRE(PresetSerializer::applyToProcessorState(arpPreset, processor.getAPVTS(), processor.getHarmonyState()));
        REQUIRE_FALSE(processor.getHarmonyState().getLiveRevoice());
        for (int deg = 0; deg < 7; ++deg) {
            const auto& spec = processor.getHarmonyState().getConfiguration().getSpec(deg);
            REQUIRE(spec.shape == music::ChordShape::seventh);
            REQUIRE(spec.style == music::VoicingStyle::compact);
        }

        // Preset 5: Jazz Tension -> Jazz Tension recipe with Live Revoice
        Preset jazzPreset;
        jazzPreset.name = "Jazz Tension";
        jazzPreset.parameters.key = 0;
        jazzPreset.parameters.waveform = "triangle";
        jazzPreset.parameters.cutoffHz = 3800.0f;
        jazzPreset.parameters.resonance = 0.35f;
        jazzPreset.parameters.detuneCents = 9.0f;
        jazzPreset.parameters.chorusMix = 0.25f;
        jazzPreset.parameters.reverbMix = 0.35f;
        jazzPreset.parameters.masterGainDb = -12.0f;
        jazzPreset.harmony.setLiveRevoice(true);
        jazzPreset.harmony.setConfiguration(music::HarmonyConfiguration::makeJazzTension());

        REQUIRE(PresetSerializer::applyToProcessorState(jazzPreset, processor.getAPVTS(), processor.getHarmonyState()));
        REQUIRE(processor.getHarmonyState().getLiveRevoice());
        // Verify characteristic Jazz Tension per-degree voicing specs
        const auto& jazzConfig = processor.getHarmonyState().getConfiguration();
        REQUIRE(jazzConfig.getSpec(0).shape == music::ChordShape::sixNine);
        REQUIRE(jazzConfig.getSpec(0).style == music::VoicingStyle::rootless);
        REQUIRE(jazzConfig.getSpec(0).bassMode == music::BassMode::root);
        REQUIRE(jazzConfig.getSpec(1).shape == music::ChordShape::eleventh); // ii
        REQUIRE(jazzConfig.getSpec(1).style == music::VoicingStyle::rootless);
        REQUIRE(jazzConfig.getSpec(4).shape == music::ChordShape::thirteenth); // V
        REQUIRE(jazzConfig.getSpec(4).style == music::VoicingStyle::rootless);
    }
}
