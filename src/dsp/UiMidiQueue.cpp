#include "UiMidiQueue.h"

namespace chordsynth::dsp {

bool UiMidiQueue::push(const juce::MidiMessage& message) noexcept {
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

void UiMidiQueue::drainTo(juce::MidiBuffer& destination, int sampleOffset) noexcept {
    const int numReady = fifo.getNumReady();
    if (numReady <= 0) {
        return;
    }

    int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
    fifo.prepareToRead(numReady, start1, size1, start2, size2);

    for (int i = 0; i < size1; ++i) {
        destination.addEvent(events[static_cast<size_t>(start1 + i)], sampleOffset);
    }

    for (int i = 0; i < size2; ++i) {
        destination.addEvent(events[static_cast<size_t>(start2 + i)], sampleOffset);
    }

    fifo.finishedRead(size1 + size2);
}

} // namespace chordsynth::dsp
