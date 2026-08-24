#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <juce_audio_basics/juce_audio_basics.h>
#include "music/HarmonyConfiguration.h"
#include "music/DiatonicChordVoicer.h"
#include "music/VoiceLeadingResolver.h"
#include "interaction/ChordTransform.h"

namespace chordsynth::interaction {

class MidiPerformanceMapper {
public:
    static constexpr std::size_t realtimeMidiBufferBytes = 64 * 1024;
    static constexpr int minDegreeNote = 36; // C2 -> degree 0 (I)
    static constexpr int maxDegreeNote = 42; // F#2 -> degree 6 (VII)
    static constexpr int minTransformCC = 20;
    static constexpr int maxTransformCC = 27;

    struct Context {
        int tonic{0}; // 0 = C
        music::Scale scale{music::Scale::major};
        bool diatonicMode{true};
        TransformPalette palette{TransformPalette::loFi};
    };

    MidiPerformanceMapper(
        const music::HarmonyConfiguration& harmonyConfig,
        const music::DiatonicChordVoicer& chordVoicer);

    ~MidiPerformanceMapper() = default;

    void setEnabled(bool enabled) noexcept { isEnabled = enabled; }
    [[nodiscard]] bool getEnabled() const noexcept { return isEnabled; }

    void setContext(const Context& newContext) noexcept;
    [[nodiscard]] const Context& getContext() const noexcept { return context; }

    // Realtime mapping: transforms inputBuffer into outputBuffer or processes in-place
    void processBlock(
        const juce::MidiBuffer& inputMidi,
        juce::MidiBuffer& outputMidi,
        int numSamples) noexcept;

    // Direct in-place overload
    void processBlock(juce::MidiBuffer& midiBuffer, int numSamples) noexcept;

    void reset() noexcept;

    [[nodiscard]] bool hasActiveChord() const noexcept { return activeDegree.has_value(); }
    [[nodiscard]] std::optional<int> getActiveDegree() const noexcept { return activeDegree; }
    [[nodiscard]] std::optional<TransformSlot> getActiveTransformSlot() const noexcept { return activeTransformSlot; }
    [[nodiscard]] const music::NoteSet& getActiveHarmonicNotes() const noexcept { return activeHarmonicNotes; }

private:
    void handleDegreeNoteOn(int degree, float velocity, int sampleOffset, juce::MidiBuffer& outputMidi) noexcept;
    void handleDegreeNoteOff(int degree, int sampleOffset, juce::MidiBuffer& outputMidi) noexcept;
    void handleTransformCC(int ccNumber, int ccValue, int sampleOffset, juce::MidiBuffer& outputMidi) noexcept;
    void emitAllMappedNotesOff(int sampleOffset, juce::MidiBuffer& outputMidi) noexcept;

    void sendDifferentialVoicing(
        const music::RealtimeVoicedChord& newVoiced,
        float velocity,
        int sampleOffset,
        juce::MidiBuffer& outputMidi) noexcept;

    [[nodiscard]] music::RealtimeVoicedChord computeCurrentVoicing(int degree) const noexcept;

    const music::HarmonyConfiguration& config;
    const music::DiatonicChordVoicer& voicer;

    bool isEnabled{false};
    Context context{};

    // Active state tracking
    std::optional<int> activeDegree{std::nullopt};
    std::optional<TransformSlot> activeTransformSlot{std::nullopt};
    float activeVelocity{0.8f};
    std::array<std::uint8_t, 7> heldDegreeCounts{};
    std::array<std::uint64_t, 7> degreePressOrder{};
    std::array<float, 7> degreeVelocities{0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f};
    std::uint64_t nextPressOrder{1};

    // Currently sounding mapped notes
    music::NoteSet activeHarmonicNotes{};
    std::optional<int> activeBassMidi{std::nullopt};

    // Scratch buffer for in-place swaps to avoid heap allocation
    juce::MidiBuffer scratchBuffer{};
};

} // namespace chordsynth::interaction
