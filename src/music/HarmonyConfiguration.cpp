#include "HarmonyConfiguration.h"
#include <algorithm>
#include <cstddef>

namespace chordsynth::music {

VoicingSpec HarmonyConfiguration::defaultSpecForDegree(int degreeIndex) noexcept {
    (void)degreeIndex;
    VoicingSpec spec{};
    spec.shape = ChordShape::triad;
    spec.extension = ChordExtension::triad;
    spec.inversion = 0;
    spec.style = VoicingStyle::compact;
    spec.fifthPolicy = FifthPolicy::include;
    spec.bassMode = BassMode::none;
    spec.voiceLeading = VoiceLeadingMode::manual;
    spec.baseOctave = 3;
    spec.qualityRule = QualityRule::diatonic;
    return spec;
}

HarmonyConfiguration::HarmonyConfiguration() noexcept {
    resetAll();
}

void HarmonyConfiguration::resetAll() noexcept {
    for (int d = 0; d < 7; ++d) {
        degrees[static_cast<std::size_t>(d)] = defaultSpecForDegree(d);
    }
}

VoicingSpec HarmonyConfiguration::getSpec(int degreeIndex) const noexcept {
    if (!isValidDegree(degreeIndex)) {
        return defaultSpecForDegree(0);
    }
    return degrees[static_cast<std::size_t>(degreeIndex)];
}

bool HarmonyConfiguration::setSpec(int degreeIndex, const VoicingSpec& spec) noexcept {
    if (!isValidDegree(degreeIndex)) {
        return false;
    }

    VoicingSpec sanitizedSpec = spec;
    sanitizedSpec.baseOctave = std::clamp(sanitizedSpec.baseOctave, 2, 4);
    sanitizedSpec.inversion = std::clamp(sanitizedSpec.inversion, 0, 5);
    sanitizedSpec.slashDegree = std::clamp(sanitizedSpec.slashDegree, 0, 6);

    degrees[static_cast<std::size_t>(degreeIndex)] = sanitizedSpec;
    return true;
}

bool HarmonyConfiguration::resetDegree(int degreeIndex) noexcept {
    if (!isValidDegree(degreeIndex)) {
        return false;
    }
    degrees[static_cast<std::size_t>(degreeIndex)] = defaultSpecForDegree(degreeIndex);
    return true;
}

HarmonyConfiguration HarmonyConfiguration::makeDiatonic() noexcept {
    HarmonyConfiguration config;
    for (int d = 0; d < 7; ++d) {
        VoicingSpec spec{};
        spec.shape = ChordShape::triad;
        spec.extension = ChordExtension::triad;
        spec.inversion = 0;
        spec.style = VoicingStyle::compact;
        spec.fifthPolicy = FifthPolicy::include;
        spec.bassMode = BassMode::none;
        spec.voiceLeading = VoiceLeadingMode::manual;
        spec.baseOctave = 3;
        spec.qualityRule = QualityRule::diatonic;
        config.setSpec(d, spec);
    }
    return config;
}

HarmonyConfiguration HarmonyConfiguration::makeSevenths() noexcept {
    HarmonyConfiguration config;
    for (int d = 0; d < 7; ++d) {
        VoicingSpec spec{};
        spec.shape = ChordShape::seventh;
        spec.extension = ChordExtension::seventh;
        spec.inversion = 0;
        spec.style = VoicingStyle::compact;
        spec.fifthPolicy = FifthPolicy::automatic;
        spec.bassMode = BassMode::none;
        spec.voiceLeading = VoiceLeadingMode::nearest;
        spec.baseOctave = 3;
        spec.qualityRule = QualityRule::diatonic;
        config.setSpec(d, spec);
    }
    return config;
}

HarmonyConfiguration HarmonyConfiguration::makeLofiWarm() noexcept {
    HarmonyConfiguration config;
    constexpr std::array<ChordShape, 7> sceneCShapes = {
        ChordShape::ninth,       // I (0)
        ChordShape::ninth,       // ii (1)
        ChordShape::seventh,     // iii (2)
        ChordShape::ninth,       // IV (3)
        ChordShape::thirteenth,  // V (4)
        ChordShape::ninth,       // vi (5)
        ChordShape::seventh      // vii (6)
    };

    for (int d = 0; d < 7; ++d) {
        VoicingSpec spec{};
        spec.shape = sceneCShapes[static_cast<std::size_t>(d)];
        spec.extension = (spec.shape == ChordShape::triad) ? ChordExtension::triad : ChordExtension::seventh;
        spec.inversion = 0;
        spec.style = VoicingStyle::open;
        spec.fifthPolicy = FifthPolicy::automatic;
        spec.bassMode = BassMode::root;
        spec.voiceLeading = VoiceLeadingMode::nearest;
        spec.baseOctave = 3;
        spec.qualityRule = QualityRule::diatonic;
        config.setSpec(d, spec);
    }
    return config;
}

HarmonyConfiguration HarmonyConfiguration::makeJazzTension() noexcept {
    HarmonyConfiguration config;
    constexpr std::array<ChordShape, 7> sceneDShapes = {
        ChordShape::sixNine,     // I (0)
        ChordShape::eleventh,    // ii (1)
        ChordShape::ninth,       // iii (2)
        ChordShape::ninth,       // IV (3)
        ChordShape::thirteenth,  // V (4)
        ChordShape::eleventh,    // vi (5)
        ChordShape::seventh      // vii (6)
    };

    for (int d = 0; d < 7; ++d) {
        VoicingSpec spec{};
        spec.shape = sceneDShapes[static_cast<std::size_t>(d)];
        spec.extension = (spec.shape == ChordShape::triad) ? ChordExtension::triad : ChordExtension::seventh;
        spec.inversion = 0;
        spec.style = VoicingStyle::rootless;
        spec.fifthPolicy = FifthPolicy::automatic;
        spec.bassMode = BassMode::root;
        spec.voiceLeading = VoiceLeadingMode::nearest;
        spec.baseOctave = 3;
        spec.qualityRule = QualityRule::diatonic;
        config.setSpec(d, spec);
    }
    return config;
}

} // namespace chordsynth::music
