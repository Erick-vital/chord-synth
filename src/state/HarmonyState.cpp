#include "HarmonyState.h"
#include <algorithm>

namespace chordsynth::state {

namespace {

int sanitizeQualityRule(int raw) noexcept {
    if (raw < 0 || raw > 4)
        return static_cast<int>(music::QualityRule::diatonic);
    return raw;
}

int sanitizeShape(int raw) noexcept {
    if (raw < 0 || raw > 8)
        return static_cast<int>(music::ChordShape::triad);
    return raw;
}

int sanitizeExtension(int raw) noexcept {
    if (raw < 0 || raw > 1)
        return static_cast<int>(music::ChordExtension::triad);
    return raw;
}

int sanitizeStyle(int raw) noexcept {
    if (raw < 0 || raw > 2)
        return static_cast<int>(music::VoicingStyle::compact);
    return raw;
}

int sanitizeFifthPolicy(int raw) noexcept {
    if (raw < 0 || raw > 2)
        return static_cast<int>(music::FifthPolicy::automatic);
    return raw;
}

int sanitizeBassMode(int raw) noexcept {
    if (raw < 0 || raw > 2)
        return static_cast<int>(music::BassMode::none);
    return raw;
}

int sanitizeVoiceLeading(int raw) noexcept {
    if (raw < 0 || raw > 1)
        return static_cast<int>(music::VoiceLeadingMode::manual);
    return raw;
}

music::VoicingSpec getLegacyDefaultSpec(int sceneIndex) noexcept {
    music::VoicingSpec spec{};
    spec.baseOctave = 3;
    spec.qualityRule = music::QualityRule::diatonic;
    spec.fifthPolicy = music::FifthPolicy::automatic;
    spec.bassMode = music::BassMode::none;
    spec.slashDegree = 0;
    spec.voiceLeading = music::VoiceLeadingMode::manual;

    switch (sceneIndex) {
        case 0: // Legacy Scene A: triad, close, inv 0
            spec.shape = music::ChordShape::triad;
            spec.extension = music::ChordExtension::triad;
            spec.style = music::VoicingStyle::compact;
            spec.inversion = 0;
            break;
        case 1: // Legacy Scene B: seventh, close, inv 0
            spec.shape = music::ChordShape::seventh;
            spec.extension = music::ChordExtension::seventh;
            spec.style = music::VoicingStyle::compact;
            spec.inversion = 0;
            break;
        case 2: // Legacy Scene C: triad, open, inv 0
            spec.shape = music::ChordShape::triad;
            spec.extension = music::ChordExtension::triad;
            spec.style = music::VoicingStyle::open;
            spec.inversion = 0;
            break;
        case 3: // Legacy Scene D: triad, close, inv 1
            spec.shape = music::ChordShape::triad;
            spec.extension = music::ChordExtension::triad;
            spec.style = music::VoicingStyle::compact;
            spec.inversion = 1;
            break;
        default:
            spec.shape = music::ChordShape::triad;
            spec.extension = music::ChordExtension::triad;
            spec.style = music::VoicingStyle::compact;
            spec.inversion = 0;
            break;
    }
    return spec;
}

} // namespace

void HarmonyState::resetToLegacyDefaults() noexcept {
    selectedScene = 0;
    liveRevoice = false;
    qualityRule = music::QualityRule::diatonic;
    for (int s = 0; s < 4; ++s) {
        for (int d = 0; d < 7; ++d) {
            configuration.setSpec(s, d, getLegacyDefaultSpec(s));
        }
    }
}

juce::ValueTree HarmonyState::toValueTree() const {
    juce::ValueTree vt{stateTag};
    vt.setProperty("version", currentVersion, nullptr);
    vt.setProperty("selectedScene", selectedScene, nullptr);
    vt.setProperty("liveRevoice", liveRevoice, nullptr);
    vt.setProperty("qualityRule", static_cast<int>(qualityRule), nullptr);

    juce::ValueTree scenesNode{"Scenes"};
    for (int sceneIdx = 0; sceneIdx < 4; ++sceneIdx) {
        juce::ValueTree sceneNode{"Scene"};
        sceneNode.setProperty("index", sceneIdx, nullptr);

        for (int degIdx = 0; degIdx < 7; ++degIdx) {
            const auto spec = configuration.getSpec(sceneIdx, degIdx);
            juce::ValueTree degNode{"Degree"};
            degNode.setProperty("index", degIdx, nullptr);
            degNode.setProperty("shape", static_cast<int>(spec.shape), nullptr);
            degNode.setProperty("extension", static_cast<int>(spec.extension), nullptr);
            degNode.setProperty("inversion", spec.inversion, nullptr);
            degNode.setProperty("style", static_cast<int>(spec.style), nullptr);
            degNode.setProperty("baseOctave", spec.baseOctave, nullptr);
            degNode.setProperty("qualityRule", static_cast<int>(spec.qualityRule), nullptr);
            degNode.setProperty("fifthPolicy", static_cast<int>(spec.fifthPolicy), nullptr);
            degNode.setProperty("bassMode", static_cast<int>(spec.bassMode), nullptr);
            degNode.setProperty("slashDegree", spec.slashDegree, nullptr);
            degNode.setProperty("voiceLeading", static_cast<int>(spec.voiceLeading), nullptr);
            sceneNode.appendChild(degNode, nullptr);
        }
        scenesNode.appendChild(sceneNode, nullptr);
    }
    vt.appendChild(scenesNode, nullptr);

    return vt;
}

bool HarmonyState::loadFromValueTree(const juce::ValueTree& vt) noexcept {
    if (!vt.isValid() || vt.getType().toString() != stateTag)
        return false;

    if (!vt.hasProperty("version"))
        return false;

    const int version = vt.getProperty("version");
    if (version < 1 || version > currentVersion)
        return false;

    if (version == 1) {
        resetToLegacyDefaults();
    } else {
        resetToDefaults();
    }

    if (vt.hasProperty("selectedScene")) {
        const int s = vt.getProperty("selectedScene");
        selectedScene = std::clamp(s, 0, 3);
    }

    if (vt.hasProperty("liveRevoice")) {
        liveRevoice = static_cast<bool>(vt.getProperty("liveRevoice"));
    }

    if (vt.hasProperty("qualityRule")) {
        const int q = sanitizeQualityRule(static_cast<int>(vt.getProperty("qualityRule")));
        qualityRule = static_cast<music::QualityRule>(q);
    }

    auto scenesNode = vt.getChildWithName("Scenes");
    if (scenesNode.isValid()) {
        for (const auto& sceneNode : scenesNode) {
            if (!sceneNode.hasType("Scene"))
                continue;

            const int sceneIdx = sceneNode.getProperty("index", -1);
            if (!music::HarmonyConfiguration::isValidScene(sceneIdx))
                continue;

            for (const auto& degNode : sceneNode) {
                if (!degNode.hasType("Degree"))
                    continue;

                const int degIdx = degNode.getProperty("index", -1);
                if (!music::HarmonyConfiguration::isValidDegree(degIdx))
                    continue;

                music::VoicingSpec spec;

                if (version == 1) {
                    const int rawExt = degNode.getProperty("extension", static_cast<int>(music::ChordExtension::triad));
                    const int rawInv = degNode.getProperty("inversion", 0);
                    const int rawStyle = degNode.getProperty("style", static_cast<int>(music::VoicingStyle::compact));
                    const int rawOctave = degNode.getProperty("baseOctave", 3);
                    const int rawQual = degNode.getProperty("qualityRule", static_cast<int>(music::QualityRule::diatonic));

                    const auto ext = static_cast<music::ChordExtension>(sanitizeExtension(rawExt));
                    spec.extension = ext;
                    spec.shape = (ext == music::ChordExtension::seventh) ? music::ChordShape::seventh : music::ChordShape::triad;
                    spec.inversion = std::clamp(rawInv, 0, 5);
                    spec.style = static_cast<music::VoicingStyle>(sanitizeStyle(rawStyle));
                    spec.baseOctave = std::clamp(rawOctave, 2, 4);
                    spec.qualityRule = static_cast<music::QualityRule>(sanitizeQualityRule(rawQual));
                    spec.fifthPolicy = music::FifthPolicy::automatic;
                    spec.bassMode = music::BassMode::none;
                    spec.slashDegree = 0;
                    spec.voiceLeading = music::VoiceLeadingMode::manual;
                } else {
                    const int rawShape = degNode.getProperty("shape", static_cast<int>(music::ChordShape::triad));
                    const int rawExt = degNode.getProperty("extension", static_cast<int>(music::ChordExtension::triad));
                    const int rawInv = degNode.getProperty("inversion", 0);
                    const int rawStyle = degNode.getProperty("style", static_cast<int>(music::VoicingStyle::compact));
                    const int rawOctave = degNode.getProperty("baseOctave", 3);
                    const int rawQual = degNode.getProperty("qualityRule", static_cast<int>(music::QualityRule::diatonic));
                    const int rawFifth = degNode.getProperty("fifthPolicy", static_cast<int>(music::FifthPolicy::automatic));
                    const int rawBass = degNode.getProperty("bassMode", static_cast<int>(music::BassMode::none));
                    const int rawSlash = degNode.getProperty("slashDegree", 0);
                    const int rawLeading = degNode.getProperty("voiceLeading", static_cast<int>(music::VoiceLeadingMode::manual));

                    spec.shape = static_cast<music::ChordShape>(sanitizeShape(rawShape));
                    spec.extension = static_cast<music::ChordExtension>(sanitizeExtension(rawExt));
                    spec.inversion = std::clamp(rawInv, 0, 5);
                    spec.style = static_cast<music::VoicingStyle>(sanitizeStyle(rawStyle));
                    spec.baseOctave = std::clamp(rawOctave, 2, 4);
                    spec.qualityRule = static_cast<music::QualityRule>(sanitizeQualityRule(rawQual));
                    spec.fifthPolicy = static_cast<music::FifthPolicy>(sanitizeFifthPolicy(rawFifth));
                    spec.bassMode = static_cast<music::BassMode>(sanitizeBassMode(rawBass));
                    spec.slashDegree = std::clamp(rawSlash, 0, 6);
                    spec.voiceLeading = static_cast<music::VoiceLeadingMode>(sanitizeVoiceLeading(rawLeading));
                }

                configuration.setSpec(sceneIdx, degIdx, spec);
            }
        }
    }

    return true;
}

} // namespace chordsynth::state
