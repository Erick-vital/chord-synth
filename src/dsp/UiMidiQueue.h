#pragma once

#include <array>
#include <span>
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

namespace chordsynth::dsp {

class UiMidiQueue {
public:
    UiMidiQueue() = default;
    ~UiMidiQueue() = default;

    bool push(const juce::MidiMessage& message) noexcept;
    bool tryPushBatch(std::span<const juce::MidiMessage> messages) noexcept;
    bool tryPop(juce::MidiMessage& destination) noexcept;
    void drainTo(juce::MidiBuffer& destination, int sampleOffset = 0) noexcept;

private:
    static constexpr int capacity = 256;
    // AbstractFifo manages bufferSize - 1 items max, so buffer size needs to be capacity + 1
    // to store up to `capacity` items.
    juce::AbstractFifo fifo{capacity + 1};
    std::array<juce::MidiMessage, capacity + 1> events{};
};

} // namespace chordsynth::dsp
