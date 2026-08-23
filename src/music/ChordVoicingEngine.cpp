#include "ChordVoicingEngine.h"
#include <algorithm>
#include <array>

namespace chordsynth::music {

int ChordVoicingEngine::transposeBassToRange(int midiNote, int minNote, int maxNote) noexcept {
    if (minNote > maxNote || (maxNote - minNote < 11)) {
        return midiNote;
    }
    int note = midiNote;
    while (note < minNote) {
        note += 12;
    }
    while (note > maxNote) {
        note -= 12;
    }
    return note;
}

NoteSet ChordVoicingEngine::applyVoicing(
    const VoicingCandidateTable& candidates,
    const ChordRecipe& recipe,
    ChordShape activeShape,
    const VoicingSpec& spec) noexcept {

    // 1. Work on a local copy of candidates
    VoicingCandidateTable workingCandidates = candidates;

    const bool isTriad = (activeShape == ChordShape::triad || activeShape == ChordShape::sus2 || activeShape == ChordShape::sus4);
    const bool isLegacySeventh = (spec.shape == ChordShape::triad && spec.extension == ChordExtension::seventh);
    const bool isSeventhOrHigher = !isTriad || isLegacySeventh;

    // 2. Apply Rootless policy:
    // "rootless: omit root for seventh-or-higher recipes; retain 3rd, 7th and named tension; triads fall back to compact."
    if (spec.style == VoicingStyle::rootless && isSeventhOrHigher) {
        workingCandidates[0].active = false; // Root is candidate 0
    }

    // 3. Apply FifthPolicy:
    // automatic: include for triads/sus; omit first for 9/11/13 when capacity/register requires it.
    // include: explicit include (sanitized when impossible).
    // omit: explicit omit (omits fifth if present).
    if (spec.fifthPolicy == FifthPolicy::omit) {
        workingCandidates[2].active = false; // Fifth is candidate 2
    }

    // 4. In 13th chords, 11th is omitted from the harmony (due to chord definition / clashing with 3rd in dominant 13th)
    if (recipe.includeThirteenth && workingCandidates[6].active) {
        workingCandidates[6].active = false;
    }

    // Check active tone count and enforce 6-tone max capacity omission
    int activeCount = 0;
    for (const auto& tone : workingCandidates) {
        if (tone.active) {
            ++activeCount;
        }
    }

    if (activeCount > static_cast<int>(maxChordTones)) {
        if (workingCandidates[2].active && workingCandidates[2].isFifth) {
            workingCandidates[2].active = false;
            --activeCount;
        }
    }

    // 5. Gather remaining tones in ascending pitch order
    // Candidate indices: root(0), 3rd/sus(1), 5th(2), 6th/13th(4), 7th(3), 9th(5), 11th(6)
    const std::array<std::size_t, 7> roleOrder = (!recipe.includeThirteenth)
        ? std::array<std::size_t, 7>{0, 1, 2, 4, 3, 5, 6}
        : std::array<std::size_t, 7>{0, 1, 2, 3, 5, 6, 4};

    NoteStorage rawNotes{};
    int noteCount = 0;

    for (std::size_t roleIdx : roleOrder) {
        const auto& tone = workingCandidates[roleIdx];
        if (tone.active && noteCount < static_cast<int>(maxChordTones)) {
            rawNotes[static_cast<std::size_t>(noteCount++)] = tone.midiNote;
        }
    }

    // 6. Apply open voicing style (drop-2 / spread)
    // "open: deterministic drop-2/spread; preserve current open triad/seventh outputs as regressions."
    if (spec.style == VoicingStyle::open) {
        const int rootMidi = candidates[0].midiNote;
        const int thirdMidi = candidates[1].midiNote;
        const int fifthMidi = candidates[2].midiNote;
        const int seventhMidi = candidates[3].midiNote;

        if (isLegacySeventh || (activeShape == ChordShape::seventh && noteCount == 4)) {
            rawNotes[0] = rootMidi;
            rawNotes[1] = fifthMidi;
            rawNotes[2] = seventhMidi;
            rawNotes[3] = thirdMidi + 12;
        } else if (noteCount == 3 && isTriad) {
            rawNotes[0] = rootMidi;
            rawNotes[1] = fifthMidi;
            rawNotes[2] = thirdMidi + 12;
        } else if (noteCount >= 4) {
            // General drop-2 for extended chords: drop 2nd note from top down an octave
            // rawNotes is sorted ascending: rawNotes[noteCount - 2] -= 12
            rawNotes[static_cast<std::size_t>(noteCount - 2)] -= 12;
            std::sort(rawNotes.begin(), rawNotes.begin() + noteCount);
        }
    }

    // 7. Apply Inversions:
    // Inversion k (0..noteCount-1): move the lowest note up an octave k times, preserving ascending order.
    if (noteCount > 0) {
        const int clampedInversion = std::clamp(spec.inversion, 0, noteCount - 1);
        for (int inv = 0; inv < clampedInversion; ++inv) {
            auto minIt = std::min_element(rawNotes.begin(), rawNotes.begin() + noteCount);
            *minIt += 12;
        }
        std::sort(rawNotes.begin(), rawNotes.begin() + noteCount);
    }

    // 8. Apply Safe Register Constraints:
    // - Harmonic chord floor: MIDI 48 (C3) for recipes with five/six tones.
    // - Rootless/open floor: MIDI 52 (E3) for non-bass tones (for rootless 7th+ or open).
    // - Harmonic ceiling: MIDI 96 (C7).
    // - Preserve pitch classes and ascending uniqueness.
    // - If a legal arrangement cannot be found, return a deterministic safe compact voicing.
    if (noteCount > 0) {
        int targetFloor = 0;
        if (noteCount >= 5) {
            targetFloor = denseChordFloor; // 48
        }
        if (spec.style == VoicingStyle::rootless && isSeventhOrHigher) {
            targetFloor = std::max(targetFloor, rootlessOpenFloor); // 52
        }

        // Transpose up by octaves if any note is below targetFloor
        while (rawNotes[0] < targetFloor && rawNotes[static_cast<std::size_t>(noteCount - 1)] + 12 <= harmonicCeiling) {
            for (int i = 0; i < noteCount; ++i) {
                rawNotes[static_cast<std::size_t>(i)] += 12;
            }
        }

        // Transpose down by octaves if highest note exceeds harmonicCeiling
        while (rawNotes[static_cast<std::size_t>(noteCount - 1)] > harmonicCeiling && rawNotes[0] - 12 >= targetFloor) {
            for (int i = 0; i < noteCount; ++i) {
                rawNotes[static_cast<std::size_t>(i)] -= 12;
            }
        }

        // Check if now valid
        bool valid = (rawNotes[0] >= targetFloor && rawNotes[static_cast<std::size_t>(noteCount - 1)] <= harmonicCeiling);

        if (!valid) {
            // Fall back to deterministic safe compact voicing:
            // Re-collect compact notes from candidate tones
            NoteStorage fallbackNotes{};
            int fallbackCount = 0;
            for (std::size_t roleIdx : roleOrder) {
                const auto& tone = workingCandidates[roleIdx];
                if (tone.active && fallbackCount < static_cast<int>(maxChordTones)) {
                    fallbackNotes[static_cast<std::size_t>(fallbackCount++)] = tone.midiNote;
                }
            }
            if (fallbackCount > 0) {
                int compactFloor = (fallbackCount >= 5) ? denseChordFloor : 0;
                if (spec.style == VoicingStyle::rootless && isSeventhOrHigher) {
                    compactFloor = std::max(compactFloor, rootlessOpenFloor);
                }
                while (fallbackNotes[0] < compactFloor && fallbackNotes[static_cast<std::size_t>(fallbackCount - 1)] + 12 <= 127) {
                    for (int i = 0; i < fallbackCount; ++i) {
                        fallbackNotes[static_cast<std::size_t>(i)] += 12;
                    }
                }
                while (fallbackNotes[static_cast<std::size_t>(fallbackCount - 1)] > harmonicCeiling && fallbackNotes[0] - 12 >= compactFloor) {
                    for (int i = 0; i < fallbackCount; ++i) {
                        fallbackNotes[static_cast<std::size_t>(i)] -= 12;
                    }
                }
                rawNotes = fallbackNotes;
                noteCount = fallbackCount;
            }
        }
    }

    return NoteSet(rawNotes, noteCount);
}

} // namespace chordsynth::music
