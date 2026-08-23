#include "DiatonicChordVoicer.h"
#include "Chord.h"
#include "ChordRecipe.h"
#include <algorithm>
#include <array>
#include <stdexcept>
#include <string_view>

namespace chordsynth::music {

namespace {

constexpr std::array<int, 7> majorScaleSemitones{0, 2, 4, 5, 7, 9, 11};
constexpr std::array<int, 7> naturalMinorScaleSemitones{0, 2, 3, 5, 7, 8, 10};

} // namespace

VoicedChord DiatonicChordVoicer::voiceChord(
    int tonicPitchClass,
    int degree,
    const VoicingSpec& spec,
    Scale scale) const {
    if (degree < 0 || degree > 6) {
        throw std::out_of_range("Degree must be between 0 and 6");
    }

    const int normalizedTonic = ((tonicPitchClass % 12) + 12) % 12;
    const int baseMidi = 12 * (spec.baseOctave + 1) + normalizedTonic;
    const auto degreeIndex = static_cast<std::size_t>(degree);
    const auto& scaleSemitones = scale == Scale::naturalMinor
        ? naturalMinorScaleSemitones : majorScaleSemitones;
    const int rootOffset = scaleSemitones[degreeIndex];
    const int rootMidi = baseMidi + rootOffset;

    // Determine chord recipe & quality
    const ChordShape activeShape = (spec.shape != ChordShape::triad)
        ? spec.shape
        : (spec.extension == ChordExtension::seventh ? ChordShape::seventh : ChordShape::triad);
    const ChordRecipe recipe = resolveChordRecipe(scale, degree, activeShape, spec.qualityRule);

    ChordQuality quality = ChordQuality::major;
    if (recipe.quality == ResolvedQuality::minor) {
        quality = ChordQuality::minor;
    } else if (recipe.quality == ResolvedQuality::diminished) {
        quality = ChordQuality::diminished;
    } else if (recipe.quality == ResolvedQuality::dominant) {
        quality = ChordQuality::major;
    }

    int thirdOffset = 4;
    int fifthOffset = 7;
    int seventhOffset = 11;

    switch (quality) {
        case ChordQuality::major:
            thirdOffset = 4;
            fifthOffset = 7;
            seventhOffset = (recipe.seventh == SeventhKind::major) ? 11 : 10;
            break;
        case ChordQuality::minor:
            thirdOffset = 3;
            fifthOffset = 7;
            seventhOffset = (recipe.seventh == SeventhKind::major) ? 11 : 10;
            break;
        case ChordQuality::diminished:
            thirdOffset = 3;
            fifthOffset = 6;
            seventhOffset = (recipe.seventh == SeventhKind::halfDiminished) ? 10 : 9;
            break;
    }

    if (recipe.sus2) {
        thirdOffset = 2;
    } else if (recipe.sus4) {
        thirdOffset = 5;
    }

    const int thirdMidi = rootMidi + thirdOffset;
    const int fifthMidi = rootMidi + fifthOffset;
    const int seventhMidi = rootMidi + seventhOffset;
    const int ninthMidi = rootMidi + 14;
    const int eleventhMidi = rootMidi + 17;
    const int sixthOrThirteenthMidi = recipe.includeThirteenth ? (rootMidi + 21) : (rootMidi + 9);

    // Build role-based candidate table:
    // Roles: root, third/sus, fifth, seventh, sixth/13th, ninth, eleventh
    struct RoleTone {
        int midiNote{0};
        bool active{false};
        bool isFifth{false};
        bool isSeventh{false};
        bool isThird{false};
        bool isHighestTension{false};
    };

    std::array<RoleTone, 7> candidateTones{};
    candidateTones[0] = RoleTone{.midiNote = rootMidi, .active = true};
    candidateTones[1] = RoleTone{.midiNote = thirdMidi, .active = true, .isThird = true};
    candidateTones[2] = RoleTone{.midiNote = fifthMidi, .active = true, .isFifth = true};
    if (recipe.seventh != SeventhKind::none) {
        candidateTones[3] = RoleTone{.midiNote = seventhMidi, .active = true, .isSeventh = true};
    }
    if (recipe.includeSixth || recipe.includeThirteenth) {
        candidateTones[4] = RoleTone{
            .midiNote = sixthOrThirteenthMidi,
            .active = true,
            .isHighestTension = recipe.includeThirteenth
        };
    }
    if (recipe.includeNinth) {
        candidateTones[5] = RoleTone{
            .midiNote = ninthMidi,
            .active = true,
            .isHighestTension = (activeShape == ChordShape::ninth || activeShape == ChordShape::add9 || activeShape == ChordShape::sixNine)
        };
    }
    if (recipe.includeEleventh) {
        candidateTones[6] = RoleTone{
            .midiNote = eleventhMidi,
            .active = true,
            .isHighestTension = (activeShape == ChordShape::eleventh)
        };
    }

    // Count active tones
    int activeCount = 0;
    for (const auto& tone : candidateTones) {
        if (tone.active) {
            ++activeCount;
        }
    }

    // Omission priority when activeCount > 6:
    // Default omission priority when theoretical recipe exceeds six:
    // 1. In standard dominant/thirteenth chords (or chords with 13th), omit 11th before fifth (or fifth/11th).
    // Specifically: omit perfect fifth (or 11th) while never omitting 3rd, 7th of dominant chord, and retain highest tension (13th).
    // For 13th chord: 7 tones (Root, 3, 5, 7, 9, 11, 13) -> 6 tones omitting 11th (candidateTones[6]) or 5th.
    // Plan Task 5 rule: "G13: G B D F A E" -> notes: G (root), B (3rd), D (5th), F (7th), A (9th), E (13th). Notice 11th (C) is omitted!
    if (activeCount > 6) {
        if (candidateTones[6].active && recipe.includeThirteenth) {
            candidateTones[6].active = false;
            --activeCount;
        } else if (candidateTones[2].active && candidateTones[2].isFifth) {
            candidateTones[2].active = false;
            --activeCount;
        }
    }

    // Collect into rawNotes in ascending pitch order (roles: root, 3rd, 5th, 7th, 9th, 11th, 13th)
    NoteStorage rawNotes{};
    int noteCount = 0;
    // Ascending role order: root (0), 3rd/sus (1), 5th (2), 6th (4 if not 13th), 7th (3), 9th (5), 11th (6), 13th (4 if 13th)
    const std::array<std::size_t, 7> roleOrder = (!recipe.includeThirteenth)
        ? std::array<std::size_t, 7>{0, 1, 2, 4, 3, 5, 6}
        : std::array<std::size_t, 7>{0, 1, 2, 3, 5, 6, 4};

    for (std::size_t roleIdx : roleOrder) {
        const auto& tone = candidateTones[roleIdx];
        if (tone.active && noteCount < static_cast<int>(maxChordTones)) {
            rawNotes[static_cast<std::size_t>(noteCount++)] = tone.midiNote;
        }
    }

    const bool isLegacySeventh = (spec.shape == ChordShape::triad && spec.extension == ChordExtension::seventh);

    // Apply Voicing Style (close vs open)
    // Open voicing:
    // For triads: Root, 5th, 3rd + 12 (drop-2 / open spread: root, fifth, raised third) -> [root, fifth, third+12]
    // For sevenths: drop-2 -> [root, fifth, seventh, third+12]
    if (spec.style == VoicingStyle::open) {
        if (isLegacySeventh || (activeShape == ChordShape::seventh && noteCount == 4)) {
            rawNotes[0] = rootMidi;
            rawNotes[1] = fifthMidi;
            rawNotes[2] = seventhMidi;
            rawNotes[3] = thirdMidi + 12;
        } else if (noteCount == 3) {
            rawNotes[0] = rootMidi;
            rawNotes[1] = fifthMidi;
            rawNotes[2] = thirdMidi + 12;
        }
    }

    // Apply Inversions:
    // Inversion k (0..noteCount-1): move the lowest note up an octave k times, preserving ascending pitch order.
    const int clampedInversion = std::clamp(spec.inversion, 0, noteCount - 1);
    for (int inv = 0; inv < clampedInversion; ++inv) {
        // Find current minimum element and transpose it up by 12 semitones
        auto minIt = std::min_element(rawNotes.begin(), rawNotes.begin() + noteCount);
        *minIt += 12;
    }
    std::sort(rawNotes.begin(), rawNotes.begin() + noteCount);

    // Boundary check for MIDI note numbers (0..127)
    for (int i = 0; i < noteCount; ++i) {
        if (rawNotes[static_cast<std::size_t>(i)] < 0 || rawNotes[static_cast<std::size_t>(i)] > 127) {
            throw std::out_of_range("Generated MIDI notes exceed range 0..127");
        }
    }

    // Generate Label
    const int rootPitchClass = ((rootMidi % 12) + 12) % 12;
    std::string label = resolveChordLabel(rootPitchClass, recipe, activeShape);

    return VoicedChord{
        .degree = degree,
        .rootMidi = rootMidi,
        .spec = spec,
        .notes = NoteSet(rawNotes, noteCount),
        .label = std::move(label),
    };
}

} // namespace chordsynth::music
