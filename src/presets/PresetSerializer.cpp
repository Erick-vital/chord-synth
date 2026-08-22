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
    paramsObj->setProperty("chorus_mix", preset.parameters.chorusMix);
    paramsObj->setProperty("chorus_rate_hz", preset.parameters.chorusRateHz);
    paramsObj->setProperty("chorus_depth", preset.parameters.chorusDepth);
    paramsObj->setProperty("delay_mix", preset.parameters.delayMix);
    paramsObj->setProperty("delay_feedback", preset.parameters.delayFeedback);
    paramsObj->setProperty("delay_time_ms", preset.parameters.delayTimeMs);
    paramsObj->setProperty("delay_sync", preset.parameters.delaySync);
    paramsObj->setProperty("delay_sync_rate", preset.parameters.delaySyncRate);
    paramsObj->setProperty("reverb_mix", preset.parameters.reverbMix);
    paramsObj->setProperty("reverb_room_size", preset.parameters.reverbRoomSize);
    paramsObj->setProperty("reverb_damping", preset.parameters.reverbDamping);
    paramsObj->setProperty("reverb_width", preset.parameters.reverbWidth);
    paramsObj->setProperty("arp_enabled", preset.parameters.arpEnabled);
    paramsObj->setProperty("arp_mode", preset.parameters.arpMode);
    paramsObj->setProperty("arp_rate", preset.parameters.arpRate);
    paramsObj->setProperty("arp_gate", preset.parameters.arpGate);
    paramsObj->setProperty("master_gain_db", preset.parameters.masterGainDb);

    rootObj->setProperty("parameters", juce::var(paramsObj));

    // Harmony section (schema version 2)
    auto* harmonyObj = new juce::DynamicObject();
    harmonyObj->setProperty("selected_scene", preset.harmony.getSelectedScene());
    harmonyObj->setProperty("live_revoice", preset.harmony.getLiveRevoice());
    harmonyObj->setProperty("quality_rule", static_cast<int>(preset.harmony.getQualityRule()));

    juce::Array<juce::var> scenesArray;
    const auto& config = preset.harmony.getConfiguration();
    for (int sceneIdx = 0; sceneIdx < 4; ++sceneIdx) {
        auto* sceneObj = new juce::DynamicObject();
        sceneObj->setProperty("index", sceneIdx);

        juce::Array<juce::var> degreesArray;
        for (int degIdx = 0; degIdx < 7; ++degIdx) {
            const auto spec = config.getSpec(sceneIdx, degIdx);
            auto* degObj = new juce::DynamicObject();
            degObj->setProperty("index", degIdx);
            degObj->setProperty("extension", static_cast<int>(spec.extension));
            degObj->setProperty("inversion", spec.inversion);
            degObj->setProperty("style", static_cast<int>(spec.style));
            degObj->setProperty("base_octave", spec.baseOctave);
            degObj->setProperty("quality_rule", static_cast<int>(spec.qualityRule));
            degreesArray.add(juce::var(degObj));
        }
        sceneObj->setProperty("degrees", juce::var(degreesArray));
        scenesArray.add(juce::var(sceneObj));
    }
    harmonyObj->setProperty("scenes", juce::var(scenesArray));

    rootObj->setProperty("harmony", juce::var(harmonyObj));

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
    if (version != 1 && version != 2)
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

    if (paramsObj->hasProperty("chorus_mix")) {
        float val = static_cast<float>(paramsObj->getProperty("chorus_mix"));
        preset.parameters.chorusMix = std::clamp(val, 0.0f, 1.0f);
    }

    if (paramsObj->hasProperty("chorus_rate_hz")) {
        float val = static_cast<float>(paramsObj->getProperty("chorus_rate_hz"));
        preset.parameters.chorusRateHz = std::clamp(val, 0.1f, 10.0f);
    }

    if (paramsObj->hasProperty("chorus_depth")) {
        float val = static_cast<float>(paramsObj->getProperty("chorus_depth"));
        preset.parameters.chorusDepth = std::clamp(val, 0.0f, 1.0f);
    }

    if (paramsObj->hasProperty("delay_mix")) {
        float val = static_cast<float>(paramsObj->getProperty("delay_mix"));
        preset.parameters.delayMix = std::clamp(val, 0.0f, 1.0f);
    }

    if (paramsObj->hasProperty("delay_feedback")) {
        float val = static_cast<float>(paramsObj->getProperty("delay_feedback"));
        preset.parameters.delayFeedback = std::clamp(val, 0.0f, 0.95f);
    }

    if (paramsObj->hasProperty("delay_time_ms")) {
        float val = static_cast<float>(paramsObj->getProperty("delay_time_ms"));
        preset.parameters.delayTimeMs = std::clamp(val, 10.0f, 2000.0f);
    }

    if (paramsObj->hasProperty("delay_sync")) {
        preset.parameters.delaySync = static_cast<bool>(paramsObj->getProperty("delay_sync"));
    }

    if (paramsObj->hasProperty("delay_sync_rate")) {
        int val = static_cast<int>(paramsObj->getProperty("delay_sync_rate"));
        preset.parameters.delaySyncRate = std::clamp(val, 0, 2);
    }

    if (paramsObj->hasProperty("reverb_mix")) {
        float val = static_cast<float>(paramsObj->getProperty("reverb_mix"));
        preset.parameters.reverbMix = std::clamp(val, 0.0f, 1.0f);
    }

    if (paramsObj->hasProperty("reverb_room_size")) {
        float val = static_cast<float>(paramsObj->getProperty("reverb_room_size"));
        preset.parameters.reverbRoomSize = std::clamp(val, 0.0f, 1.0f);
    }

    if (paramsObj->hasProperty("reverb_damping")) {
        float val = static_cast<float>(paramsObj->getProperty("reverb_damping"));
        preset.parameters.reverbDamping = std::clamp(val, 0.0f, 1.0f);
    }

    if (paramsObj->hasProperty("reverb_width")) {
        float val = static_cast<float>(paramsObj->getProperty("reverb_width"));
        preset.parameters.reverbWidth = std::clamp(val, 0.0f, 1.0f);
    }

    if (paramsObj->hasProperty("arp_enabled")) {
        preset.parameters.arpEnabled = static_cast<bool>(paramsObj->getProperty("arp_enabled"));
    }

    if (paramsObj->hasProperty("arp_mode")) {
        int val = static_cast<int>(paramsObj->getProperty("arp_mode"));
        preset.parameters.arpMode = std::clamp(val, 0, 3);
    }

    if (paramsObj->hasProperty("arp_rate")) {
        int val = static_cast<int>(paramsObj->getProperty("arp_rate"));
        preset.parameters.arpRate = std::clamp(val, 0, 2);
    }

    if (paramsObj->hasProperty("arp_gate")) {
        float val = static_cast<float>(paramsObj->getProperty("arp_gate"));
        preset.parameters.arpGate = std::clamp(val, 0.1f, 1.0f);
    }

    if (paramsObj->hasProperty("master_gain_db")) {
        float val = static_cast<float>(paramsObj->getProperty("master_gain_db"));
        preset.parameters.masterGainDb = std::clamp(val, -60.0f, 12.0f);
    }

    // Parse harmony section if present
    preset.harmony.resetToDefaults();
    if (rootObj->hasProperty("harmony")) {
        auto harmonyVar = rootObj->getProperty("harmony");
        if (harmonyVar.isObject()) {
            auto* harmObj = harmonyVar.getDynamicObject();
            if (harmObj != nullptr) {
                if (harmObj->hasProperty("selected_scene")) {
                    int sc = harmObj->getProperty("selected_scene");
                    preset.harmony.setSelectedScene(std::clamp(sc, 0, 3));
                }
                if (harmObj->hasProperty("live_revoice")) {
                    preset.harmony.setLiveRevoice(static_cast<bool>(harmObj->getProperty("live_revoice")));
                }
                if (harmObj->hasProperty("quality_rule")) {
                    int q = harmObj->getProperty("quality_rule");
                    if (q >= 0 && q <= 3) {
                        preset.harmony.setQualityRule(static_cast<music::QualityRule>(q));
                    }
                }
                if (harmObj->hasProperty("scenes")) {
                    auto scenesVar = harmObj->getProperty("scenes");
                    if (scenesVar.isArray()) {
                        auto* scenesArr = scenesVar.getArray();
                        for (const auto& sceneVar : *scenesArr) {
                            if (!sceneVar.isObject()) continue;
                            auto* scObj = sceneVar.getDynamicObject();
                            if (scObj == nullptr) continue;
                            int sceneIdx = scObj->getProperty("index");
                            if (!music::HarmonyConfiguration::isValidScene(sceneIdx)) continue;

                            if (scObj->hasProperty("degrees")) {
                                auto degreesVar = scObj->getProperty("degrees");
                                if (degreesVar.isArray()) {
                                    auto* degreesArr = degreesVar.getArray();
                                    for (const auto& degVar : *degreesArr) {
                                        if (!degVar.isObject()) continue;
                                        auto* degObj = degVar.getDynamicObject();
                                        if (degObj == nullptr) continue;
                                        int degIdx = degObj->getProperty("index");
                                        if (!music::HarmonyConfiguration::isValidDegree(degIdx)) continue;

                                        int rawExt = degObj->getProperty("extension");
                                        int rawInv = degObj->getProperty("inversion");
                                        int rawStyle = degObj->getProperty("style");
                                        int rawOctave = degObj->getProperty("base_octave");
                                        int rawQual = degObj->getProperty("quality_rule");

                                        music::VoicingSpec spec;
                                        spec.extension = (rawExt == 1) ? music::ChordExtension::seventh : music::ChordExtension::triad;
                                        spec.inversion = std::clamp(rawInv, 0, 2);
                                        spec.style = (rawStyle == 1) ? music::VoicingStyle::open : music::VoicingStyle::close;
                                        spec.baseOctave = std::clamp(rawOctave, 2, 4);
                                        if (rawQual >= 0 && rawQual <= 3) {
                                            spec.qualityRule = static_cast<music::QualityRule>(rawQual);
                                        }

                                        preset.harmony.getConfiguration().setSpec(sceneIdx, degIdx, spec);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return preset;
}

Preset PresetSerializer::fromAPVTS(const parameters::AudioProcessorValueTreeState& apvts, const juce::String& name) {
    state::HarmonyState defaultHarmony;
    return fromProcessorState(apvts, defaultHarmony, name);
}

Preset PresetSerializer::fromProcessorState(
    const parameters::AudioProcessorValueTreeState& apvts,
    const state::HarmonyState& harmonyState,
    const juce::String& name) {
    Preset preset;
    preset.schemaVersion = 2;
    preset.name = name;
    preset.harmony = harmonyState;

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

    auto* chorusMixParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(parameters::ids::chorusMix));
    if (chorusMixParam != nullptr)
        preset.parameters.chorusMix = *chorusMixParam;

    auto* chorusRateParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(parameters::ids::chorusRate));
    if (chorusRateParam != nullptr)
        preset.parameters.chorusRateHz = *chorusRateParam;

    auto* chorusDepthParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(parameters::ids::chorusDepth));
    if (chorusDepthParam != nullptr)
        preset.parameters.chorusDepth = *chorusDepthParam;

    auto* delayMixParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(parameters::ids::delayMix));
    if (delayMixParam != nullptr)
        preset.parameters.delayMix = *delayMixParam;

    auto* delayFeedbackParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(parameters::ids::delayFeedback));
    if (delayFeedbackParam != nullptr)
        preset.parameters.delayFeedback = *delayFeedbackParam;

    auto* delayTimeMsParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(parameters::ids::delayTimeMs));
    if (delayTimeMsParam != nullptr)
        preset.parameters.delayTimeMs = *delayTimeMsParam;

    auto* delaySyncParam = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(parameters::ids::delaySync));
    if (delaySyncParam != nullptr)
        preset.parameters.delaySync = *delaySyncParam;

    auto* delaySyncRateParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(parameters::ids::delaySyncRate));
    if (delaySyncRateParam != nullptr)
        preset.parameters.delaySyncRate = delaySyncRateParam->getIndex();

    auto* reverbMixParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(parameters::ids::reverbMix));
    if (reverbMixParam != nullptr)
        preset.parameters.reverbMix = *reverbMixParam;

    auto* reverbRoomSizeParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(parameters::ids::reverbRoomSize));
    if (reverbRoomSizeParam != nullptr)
        preset.parameters.reverbRoomSize = *reverbRoomSizeParam;

    auto* reverbDampingParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(parameters::ids::reverbDamping));
    if (reverbDampingParam != nullptr)
        preset.parameters.reverbDamping = *reverbDampingParam;

    auto* reverbWidthParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(parameters::ids::reverbWidth));
    if (reverbWidthParam != nullptr)
        preset.parameters.reverbWidth = *reverbWidthParam;

    auto* arpEnabledParam = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(parameters::ids::arpEnabled));
    if (arpEnabledParam != nullptr)
        preset.parameters.arpEnabled = *arpEnabledParam;

    auto* arpModeParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(parameters::ids::arpMode));
    if (arpModeParam != nullptr)
        preset.parameters.arpMode = arpModeParam->getIndex();

    auto* arpRateParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(parameters::ids::arpRate));
    if (arpRateParam != nullptr)
        preset.parameters.arpRate = arpRateParam->getIndex();

    auto* arpGateParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(parameters::ids::arpGate));
    if (arpGateParam != nullptr)
        preset.parameters.arpGate = *arpGateParam;

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

    auto* chorusMixParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(parameters::ids::chorusMix));
    if (chorusMixParam != nullptr)
        *chorusMixParam = preset.parameters.chorusMix;

    auto* chorusRateParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(parameters::ids::chorusRate));
    if (chorusRateParam != nullptr)
        *chorusRateParam = preset.parameters.chorusRateHz;

    auto* chorusDepthParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(parameters::ids::chorusDepth));
    if (chorusDepthParam != nullptr)
        *chorusDepthParam = preset.parameters.chorusDepth;

    auto* delayMixParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(parameters::ids::delayMix));
    if (delayMixParam != nullptr)
        *delayMixParam = preset.parameters.delayMix;

    auto* delayFeedbackParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(parameters::ids::delayFeedback));
    if (delayFeedbackParam != nullptr)
        *delayFeedbackParam = preset.parameters.delayFeedback;

    auto* delayTimeMsParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(parameters::ids::delayTimeMs));
    if (delayTimeMsParam != nullptr)
        *delayTimeMsParam = preset.parameters.delayTimeMs;

    auto* delaySyncParam = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(parameters::ids::delaySync));
    if (delaySyncParam != nullptr)
        *delaySyncParam = preset.parameters.delaySync;

    auto* delaySyncRateParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(parameters::ids::delaySyncRate));
    if (delaySyncRateParam != nullptr)
        *delaySyncRateParam = preset.parameters.delaySyncRate;

    auto* reverbMixParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(parameters::ids::reverbMix));
    if (reverbMixParam != nullptr)
        *reverbMixParam = preset.parameters.reverbMix;

    auto* reverbRoomSizeParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(parameters::ids::reverbRoomSize));
    if (reverbRoomSizeParam != nullptr)
        *reverbRoomSizeParam = preset.parameters.reverbRoomSize;

    auto* reverbDampingParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(parameters::ids::reverbDamping));
    if (reverbDampingParam != nullptr)
        *reverbDampingParam = preset.parameters.reverbDamping;

    auto* reverbWidthParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(parameters::ids::reverbWidth));
    if (reverbWidthParam != nullptr)
        *reverbWidthParam = preset.parameters.reverbWidth;

    auto* arpEnabledParam = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(parameters::ids::arpEnabled));
    if (arpEnabledParam != nullptr)
        *arpEnabledParam = preset.parameters.arpEnabled;

    auto* arpModeParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(parameters::ids::arpMode));
    if (arpModeParam != nullptr)
        *arpModeParam = preset.parameters.arpMode;

    auto* arpRateParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(parameters::ids::arpRate));
    if (arpRateParam != nullptr)
        *arpRateParam = preset.parameters.arpRate;

    auto* arpGateParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(parameters::ids::arpGate));
    if (arpGateParam != nullptr)
        *arpGateParam = preset.parameters.arpGate;

    return true;
}

bool PresetSerializer::applyToProcessorState(
    const Preset& preset,
    parameters::AudioProcessorValueTreeState& apvts,
    state::HarmonyState& harmonyState) {
    if (!applyToAPVTS(preset, apvts))
        return false;
    harmonyState = preset.harmony;
    return true;
}

} // namespace chordsynth::presets
