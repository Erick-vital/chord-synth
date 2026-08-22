#include "DiatonicChordVoicer.h"
#include "Chord.h"
#include <algorithm>
#include <array>
#include <stdexcept>
#include <string_view>

namespace chordsynth::music {

namespace {

constexpr std::array<int, 7> diatonicScaleSemitones{0, 2, 4, 5, 7, 9, 11};

constexpr std::array<ChordQuality, 7> diatonicQualities{
    ChordQuality::major,
    ChordQuality::minor,
    ChordQuality::minor,
    ChordQuality::major,
    ChordQuality::major,
    ChordQuality::minor,
    ChordQuality::diminished,
};

// 7th interval relative to root for each diatonic degree (major: 11, minor: 10)
constexpr std::array<int, 7> diatonicSeventhIntervals{
    11, // I: maj7 (C - B)
    10, // ii: m7 (D - C)
    10, // iii: m7 (E - D)
    11, // IV: maj7 (F - E)
    10, // V: dom7 (G - F)
    10, // vi: m7 (A - G)
    10  // vii: m7b5 (B - A)
};

constexpr std::array<std::string_view, 12> pitchNames{
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

} // namespace

VoicedChord DiatonicChordVoicer::voiceChord(
    int tonicPitchClass,
    int degree,
    const VoicingSpec& spec) const {
    if (degree < 0 || degree > 6) {
        throw std::out_of_range("Degree must be between 0 and 6");
    }

    const int normalizedTonic = ((tonicPitchClass % 12) + 12) % 12;
    const int baseMidi = 12 * (spec.baseOctave + 1) + normalizedTonic;
    const int rootOffset = diatonicScaleSemitones[static_cast<std::size_t>(degree)];
    const int rootMidi = baseMidi + rootOffset;

    // Determine chord quality
    ChordQuality quality = diatonicQualities[static_cast<std::size_t>(degree)];
    if (spec.qualityRule != QualityRule::diatonic) {
        switch (spec.qualityRule) {
            case QualityRule::major:
                quality = ChordQuality::major;
                break;
            case QualityRule::minor:
                quality = ChordQuality::minor;
                break;
            case QualityRule::diminished:
                quality = ChordQuality::diminished;
                break;
            case QualityRule::diatonic:
                break;
        }
    }

    int thirdOffset = 4;
    int fifthOffset = 7;
    int seventhOffset = 11;

    switch (quality) {
        case ChordQuality::major:
            thirdOffset = 4;
            fifthOffset = 7;
            seventhOffset = (spec.qualityRule == QualityRule::diatonic)
                ? diatonicSeventhIntervals[static_cast<std::size_t>(degree)]
                : 11;
            break;
        case ChordQuality::minor:
            thirdOffset = 3;
            fifthOffset = 7;
            seventhOffset = (spec.qualityRule == QualityRule::diatonic)
                ? diatonicSeventhIntervals[static_cast<std::size_t>(degree)]
                : 10;
            break;
        case ChordQuality::diminished:
            thirdOffset = 3;
            fifthOffset = 6;
            seventhOffset = (spec.qualityRule == QualityRule::diatonic)
                ? diatonicSeventhIntervals[static_cast<std::size_t>(degree)]
                : 9;
            break;
    }

    const int thirdMidi = rootMidi + thirdOffset;
    const int fifthMidi = rootMidi + fifthOffset;
    const int seventhMidi = rootMidi + seventhOffset;

    const bool isSeventh = (spec.extension == ChordExtension::seventh);
    const int noteCount = isSeventh ? 4 : 3;

    // Build raw root position notes
    std::array<int, 4> rawNotes{};
    rawNotes[0] = rootMidi;
    rawNotes[1] = thirdMidi;
    rawNotes[2] = fifthMidi;
    if (isSeventh) {
        rawNotes[3] = seventhMidi;
    }

    // Apply Voicing Style (close vs open)
    // Open voicing:
    // For triads: Root, 5th, 3rd + 12 (drop-2 / open spread: root, fifth, raised third) -> [root, fifth, third+12]
    // For sevenths: drop-2 -> [root, fifth, seventh, third+12]
    if (spec.style == VoicingStyle::open) {
        if (isSeventh) {
            rawNotes[0] = rootMidi;
            rawNotes[1] = fifthMidi;
            rawNotes[2] = seventhMidi;
            rawNotes[3] = thirdMidi + 12;
        } else {
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
    std::string label = std::string(pitchNames[static_cast<std::size_t>(rootPitchClass)]);

    if (!isSeventh) {
        if (quality == ChordQuality::minor) {
            label += "m";
        } else if (quality == ChordQuality::diminished) {
            label += "dim";
        }
    } else {
        if (quality == ChordQuality::major) {
            if (seventhOffset == 11) {
                label += "maj7";
            } else {
                label += "7";
            }
        } else if (quality == ChordQuality::minor) {
            if (seventhOffset == 10) {
                label += "m7";
            } else if (seventhOffset == 11) {
                label += "m(maj7)";
            } else {
                label += "m7";
            }
        } else if (quality == ChordQuality::diminished) {
            if (seventhOffset == 10) {
                label += "m7b5";
            } else if (seventhOffset == 9) {
                label += "dim7";
            } else {
                label += "dim7";
            }
        }
    }

    return VoicedChord{
        .degree = degree,
        .rootMidi = rootMidi,
        .spec = spec,
        .notes = NoteSet(rawNotes, noteCount),
        .label = std::move(label),
    };
}

} // namespace chordsynth::music
