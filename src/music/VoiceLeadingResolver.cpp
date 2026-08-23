#include "VoiceLeadingResolver.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <limits>

namespace chordsynth::music {

namespace {

struct CandidateChord {
    NoteStorage notes{};
    int count{0};
    int cost{std::numeric_limits<int>::max()};
    int totalSpan{0};

    bool operator<(const CandidateChord& other) const noexcept {
        if (cost != other.cost) {
            return cost < other.cost;
        }
        if (totalSpan != other.totalSpan) {
            return totalSpan < other.totalSpan;
        }
        // Lexicographic comparison of notes
        for (int i = 0; i < std::min(count, other.count); ++i) {
            if (notes[static_cast<std::size_t>(i)] != other.notes[static_cast<std::size_t>(i)]) {
                return notes[static_cast<std::size_t>(i)] < other.notes[static_cast<std::size_t>(i)];
            }
        }
        return count < other.count;
    }
};

} // namespace

NoteSet VoiceLeadingResolver::resolveNearestVoiceLeading(
    const NoteSet& previousNotes,
    const NoteSet& targetDefaultNotes) noexcept {

    if (previousNotes.empty() || targetDefaultNotes.empty()) {
        return targetDefaultNotes;
    }

    const int targetCount = targetDefaultNotes.size();
    const int prevCount = previousNotes.size();

    // Extract pitch classes and baseline octave shifts for target tones.
    // For each target tone i in 0..targetCount-1, we generate candidate octave shifts in {-2, -1, 0, +1, +2}
    // such that midiNote is within [safeMinMidi, safeMaxMidi].
    //
    // Candidate evaluation:
    // Generate candidate arrangements of target tones across octaves.
    // For up to 6 voices, testing combinations of octave displacements per voice or systematic permutations:
    // To keep it computationally bounded and fast:
    // Each voice j in target chord has pitch class pc = targetDefaultNotes[j] % 12.
    // In safe register [48..96] (4 octaves: 48..59, 60..71, 72..83, 84..95, 96), each pitch class has ~4-5 valid octave instances.
    // But target chord maintains relative pitch classes without duplicates in the voicing.
    //
    // A standard, robust way to generate inversions/displacements:
    // Start with the target chord in root/default shape.
    // Generate all inversions k (0..targetCount-1) and octave shifts o in {-2, -1, 0, 1, 2}.
    // Additionally, allow moving individual voices by +/- 12 semitones if within bounds.

    constexpr std::size_t maxCandidates = 256;
    std::array<CandidateChord, maxCandidates> candidateList{};
    std::size_t candidateCount = 0;

    // Helper to evaluate and register a candidate arrangement
    auto addCandidate = [&](NoteStorage rawNotes, int count) {
        if (count <= 0) return;

        // 1. Sort ascending
        std::sort(rawNotes.begin(), rawNotes.begin() + count);

        // 2. Check strict bounds [safeMinMidi, safeMaxMidi] and no duplicate pitches
        if (rawNotes[0] < safeMinMidi || rawNotes[static_cast<std::size_t>(count - 1)] > safeMaxMidi) {
            return;
        }
        for (int i = 0; i < count - 1; ++i) {
            if (rawNotes[static_cast<std::size_t>(i)] == rawNotes[static_cast<std::size_t>(i + 1)]) {
                return; // duplicate pitch
            }
        }

        // 3. Compute cost:
        // cost = sum(abs(matchedVoiceDelta)) + 12 * abs(newVoiceCount - oldVoiceCount) + 2 * crossingPenalty
        int matchedDeltaSum = 0;
        const int minVoices = std::min(count, prevCount);
        for (int i = 0; i < minVoices; ++i) {
            matchedDeltaSum += std::abs(rawNotes[static_cast<std::size_t>(i)] - previousNotes[static_cast<std::size_t>(i)]);
        }

        const int voiceDiffPenalty = 12 * std::abs(count - prevCount);

        // Crossing penalty: check if voice movements cross direction
        int crossingPenalty = 0;
        for (int i = 0; i < minVoices; ++i) {
            for (int j = i + 1; j < minVoices; ++j) {
                const int prevDiff = previousNotes[static_cast<std::size_t>(j)] - previousNotes[static_cast<std::size_t>(i)];
                const int currDiff = rawNotes[static_cast<std::size_t>(j)] - rawNotes[static_cast<std::size_t>(i)];
                if ((prevDiff > 0 && currDiff < 0) || (prevDiff < 0 && currDiff > 0)) {
                    crossingPenalty += 5;
                }
            }
        }

        const int totalCost = matchedDeltaSum + voiceDiffPenalty + 2 * crossingPenalty;
        const int span = rawNotes[static_cast<std::size_t>(count - 1)] - rawNotes[0];

        // Deduplicate in candidateList
        for (std::size_t c = 0; c < candidateCount; ++c) {
            bool identical = true;
            for (int i = 0; i < count; ++i) {
                if (candidateList[c].notes[static_cast<std::size_t>(i)] != rawNotes[static_cast<std::size_t>(i)]) {
                    identical = false;
                    break;
                }
            }
            if (identical) {
                return;
            }
        }

        if (candidateCount < maxCandidates) {
            candidateList[candidateCount++] = CandidateChord{
                .notes = rawNotes,
                .count = count,
                .cost = totalCost,
                .totalSpan = span
            };
        }
    };

    // Generate candidates from inversions and octave transpositions of targetDefaultNotes
    for (int octShift = -2; octShift <= 2; ++octShift) {
        NoteStorage baseShifted{};
        for (int i = 0; i < targetCount; ++i) {
            baseShifted[static_cast<std::size_t>(i)] = targetDefaultNotes[static_cast<std::size_t>(i)] + (octShift * 12);
        }

        // Test all standard inversions of baseShifted
        NoteStorage invWorking = baseShifted;
        for (int inv = 0; inv < targetCount; ++inv) {
            addCandidate(invWorking, targetCount);

            // Also test single-voice +/-12 adjustments on each voice of this inversion
            for (int v = 0; v < targetCount; ++v) {
                NoteStorage singleShiftPlus = invWorking;
                singleShiftPlus[static_cast<std::size_t>(v)] += 12;
                addCandidate(singleShiftPlus, targetCount);

                NoteStorage singleShiftMinus = invWorking;
                singleShiftMinus[static_cast<std::size_t>(v)] -= 12;
                addCandidate(singleShiftMinus, targetCount);
            }

            // Invert lowest note up an octave for next iteration
            auto minIt = std::min_element(invWorking.begin(), invWorking.begin() + targetCount);
            *minIt += 12;
        }
    }

    if (candidateCount == 0) {
        // If no candidate was within safe bounds, fall back to targetDefaultNotes
        return targetDefaultNotes;
    }

    // Pick best candidate according to operator<
    auto bestIt = std::min_element(candidateList.begin(), candidateList.begin() + candidateCount);
    return NoteSet(bestIt->notes, bestIt->count);
}

} // namespace chordsynth::music
