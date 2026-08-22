#include "HarmonyState.h"
#include <algorithm>

namespace chordsynth::state {

namespace {

int sanitizeQualityRule(int raw) noexcept {
    if (raw < 0 || raw > 3)
        return static_cast<int>(music::QualityRule::diatonic);
    return raw;
}

int sanitizeExtension(int raw) noexcept {
    if (raw < 0 || raw > 1)
        return static_cast<int>(music::ChordExtension::triad);
    return raw;
}

int sanitizeStyle(int raw) noexcept {
    if (raw < 0 || raw > 1)
        return static_cast<int>(music::VoicingStyle::close);
    return raw;
}

} // namespace

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
            degNode.setProperty("extension", static_cast<int>(spec.extension), nullptr);
            degNode.setProperty("inversion", spec.inversion, nullptr);
            degNode.setProperty("style", static_cast<int>(spec.style), nullptr);
            degNode.setProperty("baseOctave", spec.baseOctave, nullptr);
            degNode.setProperty("qualityRule", static_cast<int>(spec.qualityRule), nullptr);
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
    if (version != 1)
        return false;

    resetToDefaults();

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

                const int rawExt = degNode.getProperty("extension", static_cast<int>(music::ChordExtension::triad));
                const int rawInv = degNode.getProperty("inversion", 0);
                const int rawStyle = degNode.getProperty("style", static_cast<int>(music::VoicingStyle::close));
                const int rawOctave = degNode.getProperty("baseOctave", 3);
                const int rawQual = degNode.getProperty("qualityRule", static_cast<int>(music::QualityRule::diatonic));

                music::VoicingSpec spec;
                spec.extension = static_cast<music::ChordExtension>(sanitizeExtension(rawExt));
                spec.inversion = std::clamp(rawInv, 0, 2);
                spec.style = static_cast<music::VoicingStyle>(sanitizeStyle(rawStyle));
                spec.baseOctave = std::clamp(rawOctave, 2, 4);
                spec.qualityRule = static_cast<music::QualityRule>(sanitizeQualityRule(rawQual));

                configuration.setSpec(sceneIdx, degIdx, spec);
            }
        }
    }

    return true;
}

} // namespace chordsynth::state
