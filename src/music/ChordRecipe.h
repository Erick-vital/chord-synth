#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace chordsynth::music {

enum class QualityRule : std::uint8_t;
enum class Scale : std::uint8_t;

enum class ChordShape : std::uint8_t {
    triad,
    seventh,
    ninth,
    eleventh,
    thirteenth,
    add9,
    sixNine,
    sus2,
    sus4
};

enum class ResolvedQuality : std::uint8_t {
    major,
    minor,
    diminished,
    dominant
};

enum class SeventhKind : std::uint8_t {
    none,
    major,
    minor,
    diminished,
    halfDiminished
};

enum class FifthPolicy : std::uint8_t {
    automatic,
    include,
    omit
};

enum class BassMode : std::uint8_t {
    none,
    root,
    slashDegree
};

enum class VoiceLeadingMode : std::uint8_t {
    manual,
    nearest
};

struct ChordRecipe {
    ResolvedQuality quality{ResolvedQuality::major};
    SeventhKind seventh{SeventhKind::none};
    bool includeSixth{false};
    bool includeNinth{false};
    bool includeEleventh{false};
    bool includeThirteenth{false};
    bool sus2{false};
    bool sus4{false};

    constexpr bool operator==(const ChordRecipe& other) const noexcept = default;
};

[[nodiscard]] constexpr ChordRecipe createChordRecipe(
    ChordShape shape,
    ResolvedQuality quality = ResolvedQuality::major,
    SeventhKind seventh = SeventhKind::none) noexcept {
    ChordRecipe recipe{};
    recipe.quality = quality;
    recipe.seventh = seventh;

    switch (shape) {
        case ChordShape::triad:
            recipe.seventh = SeventhKind::none;
            break;
        case ChordShape::seventh:
            // keeps provided seventh or defaults to none if none provided
            break;
        case ChordShape::ninth:
            recipe.includeNinth = true;
            break;
        case ChordShape::eleventh:
            recipe.includeNinth = true;
            recipe.includeEleventh = true;
            break;
        case ChordShape::thirteenth:
            recipe.includeNinth = true;
            recipe.includeEleventh = true;
            recipe.includeThirteenth = true;
            break;
        case ChordShape::add9:
            recipe.seventh = SeventhKind::none;
            recipe.includeNinth = true;
            break;
        case ChordShape::sixNine:
            recipe.seventh = SeventhKind::none;
            recipe.includeSixth = true;
            recipe.includeNinth = true;
            break;
        case ChordShape::sus2:
            recipe.seventh = SeventhKind::none;
            recipe.sus2 = true;
            break;
        case ChordShape::sus4:
            recipe.seventh = SeventhKind::none;
            recipe.sus4 = true;
            break;
    }

    return recipe;
}

[[nodiscard]] constexpr ChordRecipe sanitizeChordRecipe(const ChordRecipe& recipe) noexcept {
    ChordRecipe clean = recipe;
    if (clean.sus2 && clean.sus4) {
        clean.sus4 = false; // prioritize sus2 or clear one
    }
    return clean;
}

[[nodiscard]] ChordRecipe resolveChordRecipe(
    Scale scale,
    int degree,
    ChordShape shape,
    QualityRule qualityRule) noexcept;

[[nodiscard]] std::string resolveChordLabel(
    int rootPitchClass,
    const ChordRecipe& recipe,
    ChordShape shape) noexcept;

} // namespace chordsynth::music
