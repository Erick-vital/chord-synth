#include "MajorScaleChordMap.h"
#include <array>
#include <stdexcept>
#include <string_view>

namespace chordsynth::music {

namespace {

constexpr std::array<int, 7> scaleSemitones{0, 2, 4, 5, 7, 9, 11};

constexpr std::array<ChordQuality, 7> qualities{
    ChordQuality::major,
    ChordQuality::minor,
    ChordQuality::minor,
    ChordQuality::major,
    ChordQuality::major,
    ChordQuality::minor,
    ChordQuality::diminished,
};

constexpr std::array<std::string_view, 12> pitchNames{
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

} // namespace

Chord MajorScaleChordMap::chordForDegree(int tonicPitchClass, int baseOctave, int degree) const {
    if (degree < 0 || degree > 6) {
        throw std::out_of_range("Degree must be between 0 and 6");
    }

    // Normalize tonic pitch class to 0..11
    int normalizedTonic = ((tonicPitchClass % 12) + 12) % 12;

    // MIDI base note: octave -1 is MIDI note 0 for C
    // MIDI note for C4 is 60 (12 * (4 + 1))
    int baseMidi = 12 * (baseOctave + 1) + normalizedTonic;

    int rootOffset = scaleSemitones[static_cast<std::size_t>(degree)];
    int rootMidi = baseMidi + rootOffset;

    ChordQuality quality = qualities[static_cast<std::size_t>(degree)];

    int thirdOffset = 0;
    int fifthOffset = 0;

    switch (quality) {
        case ChordQuality::major:
            thirdOffset = 4;
            fifthOffset = 7;
            break;
        case ChordQuality::minor:
            thirdOffset = 3;
            fifthOffset = 7;
            break;
        case ChordQuality::diminished:
            thirdOffset = 3;
            fifthOffset = 6;
            break;
    }

    int thirdMidi = rootMidi + thirdOffset;
    int fifthMidi = rootMidi + fifthOffset;

    // Boundary check for MIDI note numbers (0..127)
    if (rootMidi < 0 || rootMidi > 127 ||
        thirdMidi < 0 || thirdMidi > 127 ||
        fifthMidi < 0 || fifthMidi > 127) {
        throw std::out_of_range("Generated MIDI notes exceed range 0..127");
    }

    // Generate label
    int rootPitchClass = ((rootMidi % 12) + 12) % 12;
    std::string label = std::string(pitchNames[static_cast<std::size_t>(rootPitchClass)]);
    if (quality == ChordQuality::minor) {
        label += "m";
    } else if (quality == ChordQuality::diminished) {
        label += "dim";
    }

    return Chord{
        .degree = degree,
        .rootMidi = rootMidi,
        .quality = quality,
        .midiNotes = {rootMidi, thirdMidi, fifthMidi},
        .label = std::move(label),
    };
}

} // namespace chordsynth::music
