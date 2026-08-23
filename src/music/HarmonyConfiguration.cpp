#include "HarmonyConfiguration.h"
#include <algorithm>
#include <cstddef>

namespace chordsynth::music {

VoicingSpec HarmonyConfiguration::defaultSpecForSceneAndDegree(int sceneIndex, int degreeIndex) noexcept {
    VoicingSpec spec{};
    spec.baseOctave = 3;
    spec.qualityRule = QualityRule::diatonic;
    spec.inversion = 0;

    switch (sceneIndex) {
        case 0: // Scene A · Diatónica: triad, compact, fifth included, no separate bass, manual leading
            spec.shape = ChordShape::triad;
            spec.extension = ChordExtension::triad;
            spec.style = VoicingStyle::compact;
            spec.fifthPolicy = FifthPolicy::include;
            spec.bassMode = BassMode::none;
            spec.voiceLeading = VoiceLeadingMode::manual;
            break;

        case 1: // Scene B · Séptimas: seventh, compact, automatic fifth, no separate bass, nearest leading
            spec.shape = ChordShape::seventh;
            spec.extension = ChordExtension::seventh;
            spec.style = VoicingStyle::compact;
            spec.fifthPolicy = FifthPolicy::automatic;
            spec.bassMode = BassMode::none;
            spec.voiceLeading = VoiceLeadingMode::nearest;
            break;

        case 2: { // Scene C · Lo‑Fi Warm: degree shapes [9, 9, 7, 9, 13, 9, 7], open, automatic fifth, root bass, nearest leading
            constexpr std::array<ChordShape, 7> sceneCShapes = {
                ChordShape::ninth,       // I (0)
                ChordShape::ninth,       // ii (1)
                ChordShape::seventh,     // iii (2)
                ChordShape::ninth,       // IV (3)
                ChordShape::thirteenth,  // V (4)
                ChordShape::ninth,       // vi (5)
                ChordShape::seventh      // vii (6)
            };
            const int clampedDeg = std::clamp(degreeIndex, 0, 6);
            spec.shape = sceneCShapes[static_cast<std::size_t>(clampedDeg)];
            spec.extension = (spec.shape == ChordShape::triad) ? ChordExtension::triad : ChordExtension::seventh;
            spec.style = VoicingStyle::open;
            spec.fifthPolicy = FifthPolicy::automatic;
            spec.bassMode = BassMode::root;
            spec.voiceLeading = VoiceLeadingMode::nearest;
            break;
        }

        case 3: { // Scene D · Jazz Tension: degree shapes [6/9, 11, 9, 9, 13, 11, 7], rootless, automatic fifth, root bass, nearest leading
            constexpr std::array<ChordShape, 7> sceneDShapes = {
                ChordShape::sixNine,     // I (0)
                ChordShape::eleventh,    // ii (1)
                ChordShape::ninth,       // iii (2)
                ChordShape::ninth,       // IV (3)
                ChordShape::thirteenth,  // V (4)
                ChordShape::eleventh,    // vi (5)
                ChordShape::seventh      // vii (6)
            };
            const int clampedDeg = std::clamp(degreeIndex, 0, 6);
            spec.shape = sceneDShapes[static_cast<std::size_t>(clampedDeg)];
            spec.extension = (spec.shape == ChordShape::triad) ? ChordExtension::triad : ChordExtension::seventh;
            spec.style = VoicingStyle::rootless;
            spec.fifthPolicy = FifthPolicy::automatic;
            spec.bassMode = BassMode::root;
            spec.voiceLeading = VoiceLeadingMode::nearest;
            break;
        }

        default:
            spec.shape = ChordShape::triad;
            spec.extension = ChordExtension::triad;
            spec.style = VoicingStyle::compact;
            spec.fifthPolicy = FifthPolicy::include;
            spec.bassMode = BassMode::none;
            spec.voiceLeading = VoiceLeadingMode::manual;
            break;
    }

    return spec;
}

HarmonyConfiguration::HarmonyConfiguration() noexcept {
    resetAll();
}

void HarmonyConfiguration::resetAll() noexcept {
    for (int s = 0; s < 4; ++s) {
        for (int d = 0; d < 7; ++d) {
            scenes[static_cast<std::size_t>(s)].degrees[static_cast<std::size_t>(d)] =
                defaultSpecForSceneAndDegree(s, d);
        }
    }
}

VoicingSpec HarmonyConfiguration::getSpec(int sceneIndex, int degreeIndex) const noexcept {
    if (!isValidScene(sceneIndex) || !isValidDegree(degreeIndex)) {
        return defaultSpecForSceneAndDegree(0, 0);
    }
    return scenes[static_cast<std::size_t>(sceneIndex)].degrees[static_cast<std::size_t>(degreeIndex)];
}

bool HarmonyConfiguration::setSpec(int sceneIndex, int degreeIndex, const VoicingSpec& spec) noexcept {
    if (!isValidScene(sceneIndex) || !isValidDegree(degreeIndex)) {
        return false;
    }

    VoicingSpec sanitizedSpec = spec;
    sanitizedSpec.baseOctave = std::clamp(sanitizedSpec.baseOctave, 2, 4);
    sanitizedSpec.inversion = std::clamp(sanitizedSpec.inversion, 0, 5);
    sanitizedSpec.slashDegree = std::clamp(sanitizedSpec.slashDegree, 0, 6);

    scenes[static_cast<std::size_t>(sceneIndex)].degrees[static_cast<std::size_t>(degreeIndex)] = sanitizedSpec;
    return true;
}

bool HarmonyConfiguration::resetDegree(int sceneIndex, int degreeIndex) noexcept {
    if (!isValidScene(sceneIndex) || !isValidDegree(degreeIndex)) {
        return false;
    }
    scenes[static_cast<std::size_t>(sceneIndex)].degrees[static_cast<std::size_t>(degreeIndex)] =
        defaultSpecForSceneAndDegree(sceneIndex, degreeIndex);
    return true;
}

const SceneConfiguration& HarmonyConfiguration::getScene(int sceneIndex) const noexcept {
    if (!isValidScene(sceneIndex)) {
        return scenes[0];
    }
    return scenes[static_cast<std::size_t>(sceneIndex)];
}

} // namespace chordsynth::music

