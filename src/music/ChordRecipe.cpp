#include "ChordRecipe.h"
#include "VoicedChord.h"
#include <algorithm>
#include <array>

namespace chordsynth::music {

namespace {

constexpr std::array<ResolvedQuality, 7> majorDiatonicQualities{
    ResolvedQuality::major,
    ResolvedQuality::minor,
    ResolvedQuality::minor,
    ResolvedQuality::major,
    ResolvedQuality::major,
    ResolvedQuality::minor,
    ResolvedQuality::diminished,
};

constexpr std::array<ResolvedQuality, 7> naturalMinorDiatonicQualities{
    ResolvedQuality::minor,
    ResolvedQuality::diminished,
    ResolvedQuality::major,
    ResolvedQuality::minor,
    ResolvedQuality::minor,
    ResolvedQuality::major,
    ResolvedQuality::major,
};

constexpr std::array<SeventhKind, 7> majorDiatonicSevenths{
    SeventhKind::major,           // I: maj7
    SeventhKind::minor,           // ii: m7
    SeventhKind::minor,           // iii: m7
    SeventhKind::major,           // IV: maj7
    SeventhKind::minor,           // V: dom7 (minor 7th with major triad)
    SeventhKind::minor,           // vi: m7
    SeventhKind::halfDiminished,  // vii: m7b5
};

constexpr std::array<SeventhKind, 7> naturalMinorDiatonicSevenths{
    SeventhKind::minor,           // i: m7
    SeventhKind::halfDiminished,  // ii: m7b5
    SeventhKind::major,           // III: maj7
    SeventhKind::minor,           // iv: m7
    SeventhKind::minor,           // v: m7
    SeventhKind::major,           // VI: maj7
    SeventhKind::minor,           // VII: dom7 (Bb7 in C min)
};

constexpr std::array<std::string_view, 12> pitchNames{
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

} // namespace

ChordRecipe resolveChordRecipe(
    Scale scale,
    int degree,
    ChordShape shape,
    QualityRule qualityRule) noexcept {
    const int clampedDegree = std::clamp(degree, 0, 6);
    const auto degreeIdx = static_cast<std::size_t>(clampedDegree);

    ResolvedQuality quality = (scale == Scale::naturalMinor)
        ? naturalMinorDiatonicQualities[degreeIdx]
        : majorDiatonicQualities[degreeIdx];

    SeventhKind seventh = (scale == Scale::naturalMinor)
        ? naturalMinorDiatonicSevenths[degreeIdx]
        : majorDiatonicSevenths[degreeIdx];

    if (qualityRule != QualityRule::diatonic) {
        switch (qualityRule) {
            case QualityRule::major:
                quality = ResolvedQuality::major;
                seventh = SeventhKind::major;
                break;
            case QualityRule::minor:
                quality = ResolvedQuality::minor;
                seventh = SeventhKind::minor;
                break;
            case QualityRule::diminished:
                quality = ResolvedQuality::diminished;
                seventh = SeventhKind::diminished;
                break;
            case QualityRule::dominant:
                quality = ResolvedQuality::dominant;
                seventh = SeventhKind::minor;
                break;
            case QualityRule::diatonic:
                break;
        }
    }

    // Distinguish dominant in diatonic mode (e.g. V degree in Major)
    if (qualityRule == QualityRule::diatonic) {
        if (scale == Scale::major && clampedDegree == 4) {
            quality = ResolvedQuality::dominant;
        } else if (scale == Scale::naturalMinor && clampedDegree == 6) {
            quality = ResolvedQuality::dominant;
        }
    }

    return createChordRecipe(shape, quality, seventh);
}

std::string resolveChordLabel(
    int rootPitchClass,
    const ChordRecipe& recipe,
    ChordShape shape) noexcept {
    const int normalizedRoot = ((rootPitchClass % 12) + 12) % 12;
    std::string label = std::string(pitchNames[static_cast<std::size_t>(normalizedRoot)]);

    switch (shape) {
        case ChordShape::triad:
            if (recipe.quality == ResolvedQuality::minor) {
                label += "m";
            } else if (recipe.quality == ResolvedQuality::diminished) {
                label += "dim";
            }
            break;

        case ChordShape::seventh:
            if (recipe.quality == ResolvedQuality::major) {
                if (recipe.seventh == SeventhKind::major) {
                    label += "maj7";
                } else {
                    label += "7";
                }
            } else if (recipe.quality == ResolvedQuality::dominant) {
                label += "7";
            } else if (recipe.quality == ResolvedQuality::minor) {
                if (recipe.seventh == SeventhKind::major) {
                    label += "m(maj7)";
                } else {
                    label += "m7";
                }
            } else if (recipe.quality == ResolvedQuality::diminished) {
                if (recipe.seventh == SeventhKind::halfDiminished) {
                    label += "m7b5";
                } else if (recipe.seventh == SeventhKind::minor) {
                    label += "m7b5";
                } else {
                    label += "dim7";
                }
            }
            break;

        case ChordShape::ninth:
            if (recipe.quality == ResolvedQuality::major) {
                label += "maj9";
            } else if (recipe.quality == ResolvedQuality::dominant) {
                label += "9";
            } else if (recipe.quality == ResolvedQuality::minor) {
                label += "m9";
            } else if (recipe.quality == ResolvedQuality::diminished) {
                label += "m9b5";
            }
            break;

        case ChordShape::eleventh:
            if (recipe.quality == ResolvedQuality::major) {
                label += "maj11";
            } else if (recipe.quality == ResolvedQuality::dominant) {
                label += "11";
            } else if (recipe.quality == ResolvedQuality::minor) {
                label += "m11";
            } else if (recipe.quality == ResolvedQuality::diminished) {
                label += "m11b5";
            }
            break;

        case ChordShape::thirteenth:
            if (recipe.quality == ResolvedQuality::major) {
                label += "maj13";
            } else if (recipe.quality == ResolvedQuality::dominant) {
                label += "13";
            } else if (recipe.quality == ResolvedQuality::minor) {
                label += "m13";
            } else if (recipe.quality == ResolvedQuality::diminished) {
                label += "dim13";
            }
            break;

        case ChordShape::add9:
            if (recipe.quality == ResolvedQuality::minor) {
                label += "m(add9)";
            } else if (recipe.quality == ResolvedQuality::diminished) {
                label += "dim(add9)";
            } else {
                label += "add9";
            }
            break;

        case ChordShape::sixNine:
            if (recipe.quality == ResolvedQuality::minor) {
                label += "m6/9";
            } else if (recipe.quality == ResolvedQuality::diminished) {
                label += "dim6/9";
            } else {
                label += "6/9";
            }
            break;

        case ChordShape::sus2:
            label += "sus2";
            break;

        case ChordShape::sus4:
            if (recipe.seventh == SeventhKind::major) {
                label += "maj7sus4";
            } else if (recipe.seventh != SeventhKind::none) {
                label += "7sus4";
            } else {
                label += "sus4";
            }
            break;
    }

    return label;
}

} // namespace chordsynth::music
