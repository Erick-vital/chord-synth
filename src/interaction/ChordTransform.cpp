#include "ChordTransform.h"
#include "music/ChordRecipe.h"
#include <algorithm>
#include <array>

namespace chordsynth::interaction {

namespace {

constexpr std::array<music::ResolvedQuality, 7> majorDiatonicQualities{
    music::ResolvedQuality::major,
    music::ResolvedQuality::minor,
    music::ResolvedQuality::minor,
    music::ResolvedQuality::major,
    music::ResolvedQuality::major,
    music::ResolvedQuality::minor,
    music::ResolvedQuality::diminished,
};

constexpr std::array<music::ResolvedQuality, 7> naturalMinorDiatonicQualities{
    music::ResolvedQuality::minor,
    music::ResolvedQuality::diminished,
    music::ResolvedQuality::major,
    music::ResolvedQuality::minor,
    music::ResolvedQuality::minor,
    music::ResolvedQuality::major,
    music::ResolvedQuality::major,
};

[[nodiscard]] music::ResolvedQuality getBaseDiatonicQuality(
    music::Scale scale,
    int degree,
    music::QualityRule qualityRule) noexcept {
    const int clampedDegree = std::clamp(degree, 0, 6);
    const auto degreeIdx = static_cast<std::size_t>(clampedDegree);

    if (qualityRule != music::QualityRule::diatonic) {
        switch (qualityRule) {
            case music::QualityRule::major: return music::ResolvedQuality::major;
            case music::QualityRule::minor: return music::ResolvedQuality::minor;
            case music::QualityRule::diminished: return music::ResolvedQuality::diminished;
            case music::QualityRule::dominant: return music::ResolvedQuality::dominant;
            case music::QualityRule::diatonic: break;
        }
    }

    if (scale == music::Scale::major && clampedDegree == 4) {
        return music::ResolvedQuality::dominant;
    }
    if (scale == music::Scale::naturalMinor && clampedDegree == 6) {
        return music::ResolvedQuality::dominant;
    }

    return (scale == music::Scale::naturalMinor)
        ? naturalMinorDiatonicQualities[degreeIdx]
        : majorDiatonicQualities[degreeIdx];
}

[[nodiscard]] TransformResult applyBasicTransform(
    TransformSlot slot,
    const music::VoicingSpec& base,
    music::Scale scale,
    int degree) noexcept {
    music::VoicingSpec spec = base;
    std::string_view label = "";

    const auto baseQual = getBaseDiatonicQuality(scale, degree, base.qualityRule);

    switch (slot) {
        case TransformSlot::one: // Major/Minor flip
            if (baseQual == music::ResolvedQuality::minor) {
                spec.qualityRule = music::QualityRule::major;
                label = "Mayor (Flip)";
            } else {
                spec.qualityRule = music::QualityRule::minor;
                label = "Menor (Flip)";
            }
            break;

        case TransformSlot::two: // Dominant 7
            spec.shape = music::ChordShape::seventh;
            spec.extension = music::ChordExtension::seventh;
            spec.qualityRule = music::QualityRule::dominant;
            label = "Dominante 7";
            break;

        case TransformSlot::three: // Seventh color (maj7 or m7 or m7b5 depending on current quality)
            spec.shape = music::ChordShape::seventh;
            spec.extension = music::ChordExtension::seventh;
            label = "Séptima Color";
            break;

        case TransformSlot::four: // add9
            spec.shape = music::ChordShape::add9;
            label = "add9";
            break;

        case TransformSlot::five: // sus4
            spec.shape = music::ChordShape::sus4;
            label = "sus4";
            break;

        case TransformSlot::six: // sus2
            spec.shape = music::ChordShape::sus2;
            label = "sus2";
            break;

        case TransformSlot::seven: // 6 (represented via sixNine in v1 recipe set)
            spec.shape = music::ChordShape::sixNine;
            label = "6/9";
            break;

        case TransformSlot::eight: // Diminished
            spec.qualityRule = music::QualityRule::diminished;
            label = "Disminuido";
            break;
    }

    return {spec, label};
}

[[nodiscard]] TransformResult applyLoFiTransform(
    TransformSlot slot,
    const music::VoicingSpec& base,
    music::Scale /*scale*/,
    int /*degree*/) noexcept {
    music::VoicingSpec spec = base;
    std::string_view label = "";

    switch (slot) {
        case TransformSlot::one: // maj/min 9
            spec.shape = music::ChordShape::ninth;
            label = "Novena (9)";
            break;

        case TransformSlot::two: // add9
            spec.shape = music::ChordShape::add9;
            label = "add9";
            break;

        case TransformSlot::three: // 6/9
            spec.shape = music::ChordShape::sixNine;
            label = "6/9";
            break;

        case TransformSlot::four: // min11 / add11 (eleventh shape)
            spec.shape = music::ChordShape::eleventh;
            label = "Oncena (11)";
            break;

        case TransformSlot::five: // open 9
            spec.shape = music::ChordShape::ninth;
            spec.style = music::VoicingStyle::open;
            label = "Open 9";
            break;

        case TransformSlot::six: // rootless 7
            spec.shape = music::ChordShape::seventh;
            spec.extension = music::ChordExtension::seventh;
            spec.style = music::VoicingStyle::rootless;
            label = "Rootless 7";
            break;

        case TransformSlot::seven: // warm 13 (thirteenth shape, open/nearest)
            spec.shape = music::ChordShape::thirteenth;
            spec.style = music::VoicingStyle::open;
            spec.fifthPolicy = music::FifthPolicy::omit;
            label = "Warm 13";
            break;

        case TransformSlot::eight: // nearest-open (preserves shape, changes style to open and leading to nearest)
            spec.style = music::VoicingStyle::open;
            spec.voiceLeading = music::VoiceLeadingMode::nearest;
            label = "Nearest Open";
            break;
    }

    return {spec, label};
}

[[nodiscard]] TransformResult applySpiceTransform(
    TransformSlot slot,
    const music::VoicingSpec& base,
    music::Scale /*scale*/,
    int /*degree*/) noexcept {
    music::VoicingSpec spec = base;
    std::string_view label = "";

    switch (slot) {
        case TransformSlot::one: // Dominant 7
            spec.shape = music::ChordShape::seventh;
            spec.extension = music::ChordExtension::seventh;
            spec.qualityRule = music::QualityRule::dominant;
            label = "Dominante 7";
            break;

        case TransformSlot::two: // Diminished 7 / Half-dim
            spec.shape = music::ChordShape::seventh;
            spec.extension = music::ChordExtension::seventh;
            spec.qualityRule = music::QualityRule::diminished;
            label = "Disminuido 7";
            break;

        case TransformSlot::three: // sus4 + 7
            spec.shape = music::ChordShape::sus4;
            // sus4 in v1 is triad-like sus4
            label = "sus4 Tensión";
            break;

        case TransformSlot::four: // Dominant 9
            spec.shape = music::ChordShape::ninth;
            spec.qualityRule = music::QualityRule::dominant;
            label = "Dominante 9";
            break;

        case TransformSlot::five: // Dominant 13
            spec.shape = music::ChordShape::thirteenth;
            spec.qualityRule = music::QualityRule::dominant;
            spec.fifthPolicy = music::FifthPolicy::omit;
            label = "Dominante 13";
            break;

        case TransformSlot::six: // Minor-major color fallback / Minor 9
            spec.shape = music::ChordShape::ninth;
            spec.qualityRule = music::QualityRule::minor;
            label = "Menor 9 Tensión";
            break;

        case TransformSlot::seven: // Rootless tension (rootless 9)
            spec.shape = music::ChordShape::ninth;
            spec.style = music::VoicingStyle::rootless;
            label = "Rootless 9";
            break;

        case TransformSlot::eight: // Octave/open tension (open 11)
            spec.shape = music::ChordShape::eleventh;
            spec.style = music::VoicingStyle::open;
            label = "Open 11";
            break;
    }

    return {spec, label};
}

} // namespace

TransformResult applyChordTransform(
    TransformPalette palette,
    TransformSlot slot,
    const music::VoicingSpec& base,
    music::Scale scale,
    int degree) noexcept {
    switch (palette) {
        case TransformPalette::basic:
            return applyBasicTransform(slot, base, scale, degree);
        case TransformPalette::loFi:
            return applyLoFiTransform(slot, base, scale, degree);
        case TransformPalette::spice:
            return applySpiceTransform(slot, base, scale, degree);
    }
    return {base, ""};
}

} // namespace chordsynth::interaction
