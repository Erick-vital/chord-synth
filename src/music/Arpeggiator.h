#pragma once

#include "MusicalClock.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <random>
#include <vector>

namespace chordsynth::music {

enum class ArpMode {
    up = 0,
    down = 1,
    upDown = 2,
    random = 3
};

[[nodiscard]] inline ArpMode arpModeFromRawChoice(float rawChoice) noexcept {
    int choice = 0;
    if (std::isfinite(rawChoice)) {
        if (rawChoice >= 3.0f)
            choice = 3;
        else if (rawChoice > 0.0f)
            choice = static_cast<int>(rawChoice + 0.5f);
    }
    switch (choice) {
        case 1: return ArpMode::down;
        case 2: return ArpMode::upDown;
        case 3: return ArpMode::random;
        case 0:
        default: return ArpMode::up;
    }
}

// Realtime-safe Arpeggiator emitting MIDI events boundedly based on tempo and input notes.
class Arpeggiator final {
public:
    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    void setEnabled(bool isEnabled) noexcept;
    [[nodiscard]] bool isEnabled() const noexcept { return enabled; }

    void setMode(ArpMode mode) noexcept;
    [[nodiscard]] ArpMode getMode() const noexcept { return currentMode; }

    void setRate(ArpRate rate) noexcept;
    [[nodiscard]] ArpRate getRate() const noexcept { return clock.getRate(); }

    void setGate(float gateRatio) noexcept; // 0.1 to 1.0
    [[nodiscard]] float getGate() const noexcept { return gate; }

    void setSeed(unsigned int seed) noexcept;

    void noteOn(int midiNoteNumber, float velocity = 1.0f) noexcept;
    void noteOff(int midiNoteNumber) noexcept;
    void allNotesOff() noexcept;

    // Process incoming midi, step the arpeggiator clock and emit timed MIDI notes into outBuffer.
    void processBlock(juce::MidiBuffer& midiMessages, int numSamples, double bpm) noexcept;

private:
    void updatePattern() noexcept;
    void advanceStep(juce::MidiBuffer& outBuffer, int sampleOffset) noexcept;

    MusicalClock clock;
    bool enabled{false};
    ArpMode currentMode{ArpMode::up};
    float gate{0.8f};

    struct HeldNote {
        int noteNumber{0};
        float velocity{1.0f};
    };

    std::vector<HeldNote> heldNotes;
    std::vector<int> patternNotes;
    size_t patternIndex{0};

    int sampleCounter{0};
    int currentPlayingNote{-1};
    int noteOffSampleRemaining{-1};

    std::mt19937 rng{1337};
};

} // namespace chordsynth::music
