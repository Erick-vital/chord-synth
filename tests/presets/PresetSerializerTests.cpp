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
        original.schemaVersion = 1;
        original.name = "Warm C";
        original.parameters.key = 0;
        original.parameters.waveform = "saw";
        original.parameters.attackMs = 5.0f;
        original.parameters.decayMs = 80.0f;
        original.parameters.sustain = 0.8f;
        original.parameters.releaseMs = 120.0f;
        original.parameters.cutoffHz = 8000.0f;
        original.parameters.resonance = 0.2f;
        original.parameters.detuneCents = 7.0f;
        original.parameters.masterGainDb = -12.0f;

        auto jsonString = PresetSerializer::toJson(original);
        REQUIRE_FALSE(jsonString.isEmpty());

        auto result = PresetSerializer::fromJson(jsonString);
        REQUIRE(result.has_value());
        const auto& restored = *result;
        REQUIRE(restored.schemaVersion == 1);
        REQUIRE(restored.name == "Warm C");
        REQUIRE(restored.parameters.key == 0);
        REQUIRE(restored.parameters.waveform == "saw");
        REQUIRE(restored.parameters.attackMs == Catch::Approx(5.0f));
        REQUIRE(restored.parameters.decayMs == Catch::Approx(80.0f));
        REQUIRE(restored.parameters.sustain == Catch::Approx(0.8f));
        REQUIRE(restored.parameters.releaseMs == Catch::Approx(120.0f));
        REQUIRE(restored.parameters.cutoffHz == Catch::Approx(8000.0f));
        REQUIRE(restored.parameters.resonance == Catch::Approx(0.2f));
        REQUIRE(restored.parameters.detuneCents == Catch::Approx(7.0f));
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
    auto* waveParam = dynamic_cast<juce::AudioParameterChoice*>(
        processor.getAPVTS().getParameter(parameters::ids::waveform));
    auto* cutoffParam = dynamic_cast<juce::AudioParameterFloat*>(
        processor.getAPVTS().getParameter(parameters::ids::cutoff));
    auto* resParam = dynamic_cast<juce::AudioParameterFloat*>(
        processor.getAPVTS().getParameter(parameters::ids::resonance));
    auto* detuneParam = dynamic_cast<juce::AudioParameterFloat*>(
        processor.getAPVTS().getParameter(parameters::ids::detune));

    REQUIRE(keyParam != nullptr);
    REQUIRE(waveParam != nullptr);
    REQUIRE(cutoffParam != nullptr);
    REQUIRE(resParam != nullptr);
    REQUIRE(detuneParam != nullptr);

    *keyParam = 5;       // F
    *waveParam = 2;      // Square
    *cutoffParam = 3500.0f;
    *resParam = 0.8f;
    *detuneParam = 12.0f;

    Preset preset = PresetSerializer::fromAPVTS(processor.getAPVTS(), "My Custom Preset");
    REQUIRE(preset.name == "My Custom Preset");
    REQUIRE(preset.parameters.key == 5);
    REQUIRE(preset.parameters.waveform == "square");
    REQUIRE(preset.parameters.cutoffHz == Catch::Approx(3500.0f));
    REQUIRE(preset.parameters.resonance == Catch::Approx(0.8f));
    REQUIRE(preset.parameters.detuneCents == Catch::Approx(12.0f));

    ChordSynthAudioProcessor targetProcessor;
    bool applied = PresetSerializer::applyToAPVTS(preset, targetProcessor.getAPVTS());
    REQUIRE(applied);

    auto* targetKey = dynamic_cast<juce::AudioParameterChoice*>(
        targetProcessor.getAPVTS().getParameter(parameters::ids::key));
    auto* targetWave = dynamic_cast<juce::AudioParameterChoice*>(
        targetProcessor.getAPVTS().getParameter(parameters::ids::waveform));
    auto* targetCutoff = dynamic_cast<juce::AudioParameterFloat*>(
        targetProcessor.getAPVTS().getParameter(parameters::ids::cutoff));
    auto* targetRes = dynamic_cast<juce::AudioParameterFloat*>(
        targetProcessor.getAPVTS().getParameter(parameters::ids::resonance));
    auto* targetDetune = dynamic_cast<juce::AudioParameterFloat*>(
        targetProcessor.getAPVTS().getParameter(parameters::ids::detune));

    REQUIRE(targetKey->getIndex() == 5);
    REQUIRE(targetWave->getIndex() == 2);
    REQUIRE(static_cast<float>(*targetCutoff) == Catch::Approx(3500.0f));
    REQUIRE(static_cast<float>(*targetRes) == Catch::Approx(0.8f));
    REQUIRE(static_cast<float>(*targetDetune) == Catch::Approx(12.0f));
}
