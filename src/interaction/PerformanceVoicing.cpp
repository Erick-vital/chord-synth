#include "interaction/PerformanceVoicing.h"

namespace chordsynth::interaction {

music::VoicingSpec resolvePerformanceVoicingSpec(
    const music::HarmonyConfiguration& config,
    const PerformanceVoicingContext& context,
    int degree,
    std::optional<TransformSelection> transform) noexcept
{
    auto spec = config.getSpec(context.sceneIndex, degree);
    if (context.diatonicMode) {
        spec.qualityRule = music::QualityRule::diatonic;
    }

    if (transform.has_value()) {
        spec = applyChordTransform(
            transform->palette,
            transform->slot,
            spec,
            context.scale,
            degree).spec;
    }

    return spec;
}

} // namespace chordsynth::interaction
