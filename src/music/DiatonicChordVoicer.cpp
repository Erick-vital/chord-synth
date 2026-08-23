#include "ChordVoicingEngine.h"
#include "DiatonicChordVoicer.h"
#include "Chord.h"
#include "ChordRecipe.h"
#include <algorithm>
#include <array>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace chordsynth::music {

namespace {

constexpr std::array<int, 7> majorScaleSemitones{0, 2, 4, 5, 7, 9, 11};
constexpr std::array<int, 7> naturalMinorScaleSemitones{0, 2, 3, 5, 7, 8, 10};
constexpr std::array<std::string_view, 12> pitchNames{
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

[[nodiscard]] ChordRecipe resolveSpecRecipe(
    Scale scale,
    int degree,
    ChordShape activeShape,
    const VoicingSpec& spec) noexcept {
    auto recipe = resolveChordRecipe(scale, degree, activeShape, spec.qualityRule);
    const bool suspendedSeventh =
        (activeShape == ChordShape::sus2 || activeShape == ChordShape::sus4) &&
        spec.extension == ChordExtension::seventh;
    if (suspendedSeventh) {
        const auto seventhRecipe = resolveChordRecipe(
            scale,
            degree,
            ChordShape::seventh,
            spec.qualityRule);
        recipe.seventh = seventhRecipe.seventh;
    }
    return recipe;
}

} // namespace

RealtimeVoicedChord DiatonicChordVoicer::voiceChordRealtime(
    int tonicPitchClass,
    int degree,
    const VoicingSpec& spec,
    Scale scale) const noexcept {
    const int safeDegree = std::clamp(degree, 0, 6);

    const int normalizedTonic = ((tonicPitchClass % 12) + 12) % 12;
    const int baseMidi = 12 * (spec.baseOctave + 1) + normalizedTonic;
    const auto degreeIndex = static_cast<std::size_t>(safeDegree);
    const auto& scaleSemitones = scale == Scale::naturalMinor
        ? naturalMinorScaleSemitones : majorScaleSemitones;
    const int rootOffset = scaleSemitones[degreeIndex];
    const int rootMidi = baseMidi + rootOffset;

    // Determine chord recipe & quality
    const ChordShape activeShape = (spec.shape != ChordShape::triad)
        ? spec.shape
        : (spec.extension == ChordExtension::seventh ? ChordShape::seventh : ChordShape::triad);
    const ChordRecipe recipe = resolveSpecRecipe(scale, safeDegree, activeShape, spec);

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
    VoicingCandidateTable candidateTones{};
    candidateTones[0] = VoicingCandidateTone{.midiNote = rootMidi, .active = true, .isRoot = true};
    candidateTones[1] = VoicingCandidateTone{.midiNote = thirdMidi, .active = true, .isThird = true};
    candidateTones[2] = VoicingCandidateTone{.midiNote = fifthMidi, .active = true, .isFifth = true};
    if (recipe.seventh != SeventhKind::none) {
        candidateTones[3] = VoicingCandidateTone{.midiNote = seventhMidi, .active = true, .isSeventh = true};
    }
    if (recipe.includeSixth || recipe.includeThirteenth) {
        candidateTones[4] = VoicingCandidateTone{
            .midiNote = sixthOrThirteenthMidi,
            .active = true,
            .isHighestTension = recipe.includeThirteenth
        };
    }
    if (recipe.includeNinth) {
        candidateTones[5] = VoicingCandidateTone{
            .midiNote = ninthMidi,
            .active = true,
            .isHighestTension = (activeShape == ChordShape::ninth || activeShape == ChordShape::add9 || activeShape == ChordShape::sixNine)
        };
    }
    if (recipe.includeEleventh) {
        candidateTones[6] = VoicingCandidateTone{
            .midiNote = eleventhMidi,
            .active = true,
            .isHighestTension = (activeShape == ChordShape::eleventh)
        };
    }

    NoteSet voicedNotes = ChordVoicingEngine::applyVoicing(candidateTones, recipe, activeShape, spec);

    // Calculate optional bassMidi based on spec.bassMode
    std::optional<int> bassMidi{};
    if (spec.bassMode == BassMode::root) {
        bassMidi = ChordVoicingEngine::transposeBassToRange(rootMidi);
    } else if (spec.bassMode == BassMode::slashDegree) {
        const int clampedSlashDegree = std::clamp(spec.slashDegree, 0, 6);
        const int slashOffset = scaleSemitones[static_cast<std::size_t>(clampedSlashDegree)];
        const int slashMidi = baseMidi + slashOffset;
        bassMidi = ChordVoicingEngine::transposeBassToRange(slashMidi);
    }

    return RealtimeVoicedChord{
        .degree = safeDegree,
        .rootMidi = rootMidi,
        .notes = voicedNotes,
        .bassMidi = bassMidi,
    };
}

VoicedChord DiatonicChordVoicer::voiceChord(
    int tonicPitchClass,
    int degree,
    const VoicingSpec& spec,
    Scale scale) const {
    if (degree < 0 || degree > 6) {
        throw std::out_of_range("Degree must be between 0 and 6");
    }

    const auto realtime = voiceChordRealtime(tonicPitchClass, degree, spec, scale);
    for (int i = 0; i < realtime.notes.size(); ++i) {
        if (realtime.notes[static_cast<std::size_t>(i)] < 0 || realtime.notes[static_cast<std::size_t>(i)] > 127) {
            throw std::out_of_range("Generated MIDI notes exceed range 0..127");
        }
    }

    const ChordShape activeShape = (spec.shape != ChordShape::triad)
        ? spec.shape
        : (spec.extension == ChordExtension::seventh ? ChordShape::seventh : ChordShape::triad);
    const ChordRecipe recipe = resolveSpecRecipe(scale, degree, activeShape, spec);
    const int rootPitchClass = ((realtime.rootMidi % 12) + 12) % 12;
    std::string label = resolveChordLabel(rootPitchClass, recipe, activeShape);

    if (spec.bassMode == BassMode::slashDegree) {
        const auto& scaleSemitones = scale == Scale::naturalMinor
            ? naturalMinorScaleSemitones : majorScaleSemitones;
        const int normalizedTonic = ((tonicPitchClass % 12) + 12) % 12;
        const int clampedSlashDegree = std::clamp(spec.slashDegree, 0, 6);
        const int bassPitchClass = (normalizedTonic + scaleSemitones[static_cast<std::size_t>(clampedSlashDegree)]) % 12;
        label += "/" + std::string(pitchNames[static_cast<std::size_t>(bassPitchClass)]);
    }

    return VoicedChord{
        .degree = realtime.degree,
        .rootMidi = realtime.rootMidi,
        .spec = spec,
        .notes = realtime.notes,
        .bassMidi = realtime.bassMidi,
        .label = std::move(label),
    };
}

} // namespace chordsynth::music
