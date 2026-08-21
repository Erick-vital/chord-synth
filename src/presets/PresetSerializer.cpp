#include "PresetSerializer.h"
#include "parameters/ParameterIds.h"
#include <algorithm>
#include <cmath>

namespace chordsynth::presets {

namespace {

juce::String waveformToString(int index) {
    switch (index) {
        case 0: return "sine";
        case 1: return "saw";
        case 2: return "square";
        case 3: return "triangle";
        default: return "sine";
    }
}

int waveformToIndex(const juce::String& name) {
    auto lower = name.toLowerCase().trim();
    if (lower == "sine") return 0;
    if (lower == "saw") return 1;
    if (lower == "square") return 2;
    if (lower == "triangle") return 3;
    return 0; // fallback
}

} // namespace

juce::String PresetSerializer::toJson(const Preset& preset) {
    auto* rootObj = new juce::DynamicObject();
    rootObj->setProperty("schema_version", preset.schemaVersion);
    rootObj->setProperty("name", preset.name);

    auto* paramsObj = new juce::DynamicObject();
    paramsObj->setProperty("key", preset.parameters.key);
    paramsObj->setProperty("waveform", preset.parameters.waveform);
    paramsObj->setProperty("attack_ms", preset.parameters.attackMs);
    paramsObj->setProperty("decay_ms", preset.parameters.decayMs);
    paramsObj->setProperty("sustain", preset.parameters.sustain);
    paramsObj->setProperty("release_ms", preset.parameters.releaseMs);
    paramsObj->setProperty("cutoff_hz", preset.parameters.cutoffHz);
    paramsObj->setProperty("resonance", preset.parameters.resonance);
    paramsObj->setProperty("detune_cents", preset.parameters.detuneCents);
    paramsObj->setProperty("master_gain_db", preset.parameters.masterGainDb);

    rootObj->setProperty("parameters", juce::var(paramsObj));

    return juce::JSON::toString(juce::var(rootObj));
}

std::optional<Preset> PresetSerializer::fromJson(const juce::String& jsonString) {
    auto parsedVar = juce::JSON::parse(jsonString);
    if (!parsedVar.isObject())
        return std::nullopt;

    auto* rootObj = parsedVar.getDynamicObject();
    if (rootObj == nullptr)
        return std::nullopt;

    if (!rootObj->hasProperty("schema_version") || !rootObj->hasProperty("parameters"))
        return std::nullopt;

    int version = rootObj->getProperty("schema_version");
    if (version != 1)
        return std::nullopt;

    Preset preset;
    preset.schemaVersion = version;
    preset.name = rootObj->getProperty("name").toString();
    if (preset.name.isEmpty())
        preset.name = "Preset";

    auto paramsVar = rootObj->getProperty("parameters");
    if (!paramsVar.isObject())
        return std::nullopt;

    auto* paramsObj = paramsVar.getDynamicObject();
    if (paramsObj == nullptr)
        return std::nullopt;

    // Parse parameters with bounds clamping and sanitization
    if (paramsObj->hasProperty("key")) {
        int key = paramsObj->getProperty("key");
        preset.parameters.key = std::clamp(key, 0, 11);
    }

    if (paramsObj->hasProperty("waveform")) {
        juce::String waveStr = paramsObj->getProperty("waveform").toString();
        int idx = waveformToIndex(waveStr);
        preset.parameters.waveform = waveformToString(idx);
    }

    if (paramsObj->hasProperty("attack_ms")) {
        float val = static_cast<float>(paramsObj->getProperty("attack_ms"));
        preset.parameters.attackMs = std::clamp(val, 0.1f, 5000.0f);
    }

    if (paramsObj->hasProperty("decay_ms")) {
        float val = static_cast<float>(paramsObj->getProperty("decay_ms"));
        preset.parameters.decayMs = std::clamp(val, 1.0f, 5000.0f);
    }

    if (paramsObj->hasProperty("sustain")) {
        float val = static_cast<float>(paramsObj->getProperty("sustain"));
        preset.parameters.sustain = std::clamp(val, 0.0f, 1.0f);
    }

    if (paramsObj->hasProperty("release_ms")) {
        float val = static_cast<float>(paramsObj->getProperty("release_ms"));
        preset.parameters.releaseMs = std::clamp(val, 1.0f, 5000.0f);
    }

    if (paramsObj->hasProperty("cutoff_hz")) {
        float val = static_cast<float>(paramsObj->getProperty("cutoff_hz"));
        preset.parameters.cutoffHz = std::clamp(val, 20.0f, 20000.0f);
    }

    if (paramsObj->hasProperty("resonance")) {
        float val = static_cast<float>(paramsObj->getProperty("resonance"));
        preset.parameters.resonance = std::clamp(val, 0.1f, 2.0f);
    }

    if (paramsObj->hasProperty("detune_cents")) {
        float val = static_cast<float>(paramsObj->getProperty("detune_cents"));
        preset.parameters.detuneCents = std::clamp(val, 0.0f, 20.0f);
    }

    if (paramsObj->hasProperty("master_gain_db")) {
        float val = static_cast<float>(paramsObj->getProperty("master_gain_db"));
        preset.parameters.masterGainDb = std::clamp(val, -60.0f, 12.0f);
    }

    return preset;
}

Preset PresetSerializer::fromAPVTS(const parameters::AudioProcessorValueTreeState& apvts, const juce::String& name) {
    Preset preset;
    preset.name = name;

    auto* keyParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(parameters::ids::key));
    if (keyParam != nullptr)
        preset.parameters.key = keyParam->getIndex();

    auto* waveParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(parameters::ids::waveform));
    if (waveParam != nullptr)
        preset.parameters.waveform = waveformToString(waveParam->getIndex());

    auto* cutoffParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(parameters::ids::cutoff));
    if (cutoffParam != nullptr)
        preset.parameters.cutoffHz = *cutoffParam;

    auto* resParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(parameters::ids::resonance));
    if (resParam != nullptr)
        preset.parameters.resonance = *resParam;

    auto* detuneParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(parameters::ids::detune));
    if (detuneParam != nullptr)
        preset.parameters.detuneCents = *detuneParam;

    return preset;
}

bool PresetSerializer::applyToAPVTS(const Preset& preset, parameters::AudioProcessorValueTreeState& apvts) {
    auto* keyParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(parameters::ids::key));
    if (keyParam != nullptr)
        *keyParam = preset.parameters.key;

    auto* waveParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(parameters::ids::waveform));
    if (waveParam != nullptr)
        *waveParam = waveformToIndex(preset.parameters.waveform);

    auto* cutoffParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(parameters::ids::cutoff));
    if (cutoffParam != nullptr)
        *cutoffParam = preset.parameters.cutoffHz;

    auto* resParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(parameters::ids::resonance));
    if (resParam != nullptr)
        *resParam = preset.parameters.resonance;

    auto* detuneParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(parameters::ids::detune));
    if (detuneParam != nullptr)
        *detuneParam = preset.parameters.detuneCents;

    return true;
}

} // namespace chordsynth::presets
