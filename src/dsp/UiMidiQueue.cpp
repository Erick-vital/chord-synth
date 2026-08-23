#include "UiMidiQueue.h"

namespace chordsynth::dsp {
namespace {

bool isRealtimeSafeMidiMessage(const juce::MidiMessage& message) noexcept
{
    return !message.isSysEx() && message.getRawDataSize() <= 3;
}

} // namespace

bool UiMidiQueue::push(const juce::MidiMessage& message) noexcept {
    if (!isRealtimeSafeMidiMessage(message)) {
        return false;
    }
    int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
    fifo.prepareToWrite(1, start1, size1, start2, size2);

    if (size1 + size2 < 1) {
        return false;
    }

    if (size1 > 0) {
        events[static_cast<size_t>(start1)] = message;
    } else if (size2 > 0) {
        events[static_cast<size_t>(start2)] = message;
    }

    fifo.finishedWrite(1);
    return true;
}

bool UiMidiQueue::tryPushBatch(std::span<const juce::MidiMessage> messages) noexcept {
    const int count = static_cast<int>(messages.size());
    if (count <= 0) {
        return true;
    }
    for (const auto& message : messages) {
        if (!isRealtimeSafeMidiMessage(message)) {
            return false;
        }
    }

    int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
    fifo.prepareToWrite(count, start1, size1, start2, size2);

    if (size1 + size2 < count) {
        return false;
    }

    for (int i = 0; i < size1; ++i) {
        events[static_cast<size_t>(start1 + i)] = messages[static_cast<size_t>(i)];
    }

    for (int i = 0; i < size2; ++i) {
        events[static_cast<size_t>(start2 + i)] = messages[static_cast<size_t>(size1 + i)];
    }

    fifo.finishedWrite(count);
    return true;
}

void UiMidiQueue::drainTo(juce::MidiBuffer& destination, int sampleOffset) noexcept {
    juce::MidiMessage message;
    while (tryPop(message)) {
        destination.addEvent(message, sampleOffset);
    }
}

bool UiMidiQueue::tryPop(juce::MidiMessage& destination) noexcept {
    int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
    fifo.prepareToRead(1, start1, size1, start2, size2);
    if (size1 + size2 == 0) {
        return false;
    }

    destination = size1 > 0 ? events[static_cast<size_t>(start1)]
                            : events[static_cast<size_t>(start2)];
    fifo.finishedRead(1);
    return true;
}

} // namespace chordsynth::dsp
