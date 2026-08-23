#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>
#include "ChordRecipe.h"

namespace chordsynth::music {

inline constexpr std::size_t maxChordTones = 6;
using NoteStorage = std::array<int, maxChordTones>;

class NoteSet {
public:
    constexpr NoteSet() = default;

    constexpr NoteSet(const NoteStorage& rawNotes, int noteCount) noexcept
        : notes(rawNotes), count(std::clamp(noteCount, 0, static_cast<int>(maxChordTones))) {}

    constexpr NoteSet(std::initializer_list<int> initNotes, int noteCount) noexcept {
        count = std::clamp(noteCount, 0, static_cast<int>(maxChordTones));
        std::size_t idx = 0;
        for (int note : initNotes) {
            if (idx >= maxChordTones) {
                break;
            }
            notes[idx++] = note;
        }
    }

    static constexpr int capacity() noexcept { return static_cast<int>(maxChordTones); }

    [[nodiscard]] constexpr int size() const noexcept { return count; }
    [[nodiscard]] constexpr bool empty() const noexcept { return count == 0; }

    [[nodiscard]] constexpr int operator[](std::size_t index) const noexcept {
        return notes[index];
    }

    [[nodiscard]] constexpr const int* begin() const noexcept { return notes.data(); }
    [[nodiscard]] constexpr const int* end() const noexcept { return notes.data() + count; }

    [[nodiscard]] constexpr const NoteStorage& data() const noexcept { return notes; }

    constexpr bool operator==(const NoteSet& other) const noexcept {
        if (count != other.count) {
            return false;
        }
        for (int i = 0; i < count; ++i) {
            if (notes[static_cast<std::size_t>(i)] != other.notes[static_cast<std::size_t>(i)]) {
                return false;
            }
        }
        return true;
    }

    constexpr bool operator!=(const NoteSet& other) const noexcept {
        return !(*this == other);
    }

private:
    NoteStorage notes{};
    int count{0};
};

enum class ChordExtension : std::uint8_t {
    triad,
    seventh
};

enum class VoicingStyle : std::uint8_t {
    close,
    open
};

enum class QualityRule : std::uint8_t {
    diatonic = 0,
    major = 1,
    minor = 2,
    diminished = 3,
    dominant = 4
};

enum class Scale : std::uint8_t {
    major,
    naturalMinor
};

struct VoicingSpec {
    ChordShape shape{ChordShape::triad};
    ChordExtension extension{ChordExtension::triad};
    int inversion{0};
    VoicingStyle style{VoicingStyle::close};
    FifthPolicy fifthPolicy{FifthPolicy::automatic};
    BassMode bassMode{BassMode::none};
    int slashDegree{0};
    VoiceLeadingMode voiceLeading{VoiceLeadingMode::manual};
    int baseOctave{3};
    QualityRule qualityRule{QualityRule::diatonic};

    constexpr bool operator==(const VoicingSpec& other) const noexcept = default;
};

struct VoicedChord {
    int degree{0};
    int rootMidi{0};
    VoicingSpec spec{};
    NoteSet notes{};
    std::string label{};
};

} // namespace chordsynth::music
