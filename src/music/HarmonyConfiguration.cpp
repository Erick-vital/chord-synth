#include "HarmonyConfiguration.h"
#include <algorithm>
#include <cstddef>

namespace chordsynth::music {

VoicingSpec HarmonyConfiguration::defaultSpecForSceneAndDegree(int sceneIndex, int degreeIndex) noexcept {
    (void)degreeIndex;
    VoicingSpec spec{};
    spec.baseOctave = 3;
    spec.qualityRule = QualityRule::diatonic;

    switch (sceneIndex) {
        case 0: // Scene A: triads, root, close
            spec.extension = ChordExtension::triad;
            spec.inversion = 0;
            spec.style = VoicingStyle::close;
            break;
        case 1: // Scene B: sevenths, root, close
            spec.extension = ChordExtension::seventh;
            spec.inversion = 0;
            spec.style = VoicingStyle::close;
            break;
        case 2: // Scene C: triads, root, open
            spec.extension = ChordExtension::triad;
            spec.inversion = 0;
            spec.style = VoicingStyle::open;
            break;
        case 3: // Scene D: triads, 1st inversion, close
            spec.extension = ChordExtension::triad;
            spec.inversion = 1;
            spec.style = VoicingStyle::close;
            break;
        default:
            spec.extension = ChordExtension::triad;
            spec.inversion = 0;
            spec.style = VoicingStyle::close;
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

    const int maxInversion = (sanitizedSpec.extension == ChordExtension::seventh) ? 3 : 2;
    sanitizedSpec.inversion = std::clamp(sanitizedSpec.inversion, 0, maxInversion);

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
