#include <catch2/catch_test_macros.hpp>
#include "dsp/UiMidiQueue.h"
#include "plugin/PluginProcessor.h"

using namespace chordsynth;

TEST_CASE("UiMidiQueue operations and real-time thread safety", "[dsp][uimidi]") {
    dsp::UiMidiQueue queue;

    SECTION("Maintains FIFO order when pushing and draining events") {
        auto msg1 = juce::MidiMessage::noteOn(1, 60, 0.8f);
        auto msg2 = juce::MidiMessage::noteOn(1, 64, 0.7f);
        auto msg3 = juce::MidiMessage::noteOn(1, 67, 0.9f);

        REQUIRE(queue.push(msg1));
        REQUIRE(queue.push(msg2));
        REQUIRE(queue.push(msg3));

        juce::MidiBuffer dest;
        queue.drainTo(dest, 0);

        int count = 0;
        int expectedNotes[] = {60, 64, 67};
        for (const auto metadata : dest) {
            auto msg = metadata.getMessage();
            REQUIRE(msg.isNoteOn());
            REQUIRE(msg.getNoteNumber() == expectedNotes[count]);
            count++;
        }
        REQUIRE(count == 3);
    }

    SECTION("Drains events into destination buffer with proper sample offset") {
        auto msg = juce::MidiMessage::noteOn(1, 60, 1.0f);
        REQUIRE(queue.push(msg));

        juce::MidiBuffer dest;
        int sampleOffset = 16;
        queue.drainTo(dest, sampleOffset);

        REQUIRE(dest.getNumEvents() == 1);
        for (const auto metadata : dest) {
            REQUIRE(metadata.samplePosition == sampleOffset);
            REQUIRE(metadata.getMessage().isNoteOn());
            REQUIRE(metadata.getMessage().getNoteNumber() == 60);
        }

        // Subsequent drain on empty queue should produce no new events
        juce::MidiBuffer dest2;
        queue.drainTo(dest2, 0);
        REQUIRE(dest2.isEmpty());
    }

    SECTION("Reports overflow without allocation or blocking when capacity is exceeded") {
        // Capacity is 256
        for (int i = 0; i < 256; ++i) {
            auto msg = juce::MidiMessage::noteOn(1, (i % 128), 0.5f);
            REQUIRE(queue.push(msg));
        }

        // 257th push must fail (return false)
        auto overflowMsg = juce::MidiMessage::noteOn(1, 60, 1.0f);
        REQUIRE_FALSE(queue.push(overflowMsg));

        // Drain all 256 events
        juce::MidiBuffer dest;
        queue.drainTo(dest, 0);
        REQUIRE(dest.getNumEvents() == 256);

        // After drain, pushing succeeds again
        REQUIRE(queue.push(overflowMsg));
    }

    SECTION("Note-on and note-off survive push and drain cycle") {
        auto noteOn = juce::MidiMessage::noteOn(2, 72, (juce::uint8)100);
        auto noteOff = juce::MidiMessage::noteOff(2, 72, (juce::uint8)0);

        REQUIRE(queue.push(noteOn));
        REQUIRE(queue.push(noteOff));

        juce::MidiBuffer dest;
        queue.drainTo(dest, 0);

        auto it = dest.begin();
        REQUIRE(it != dest.end());
        auto msg1 = (*it).getMessage();
        REQUIRE(msg1.isNoteOn());
        REQUIRE(msg1.getChannel() == 2);
        REQUIRE(msg1.getNoteNumber() == 72);
        REQUIRE(msg1.getVelocity() == 100);

        ++it;
        REQUIRE(it != dest.end());
        auto msg2 = (*it).getMessage();
        REQUIRE(msg2.isNoteOff());
        REQUIRE(msg2.getChannel() == 2);
        REQUIRE(msg2.getNoteNumber() == 72);

        ++it;
        REQUIRE(it == dest.end());
    }

    SECTION("Processor mixes host MIDI and UI MIDI in the same block") {
        ChordSynthAudioProcessor processor;
        processor.prepareToPlay(44100.0, 256);

        // Push UI note
        auto uiNote = juce::MidiMessage::noteOn(1, 60, 0.8f);
        REQUIRE(processor.getUiMidiQueue().push(uiNote));

        // Host MIDI note
        juce::MidiBuffer hostMidi;
        hostMidi.addEvent(juce::MidiMessage::noteOn(1, 64, 0.8f), 0);

        juce::AudioBuffer<float> buffer(2, 256);
        buffer.clear();

        processor.processBlock(buffer, hostMidi);

        // Audio buffer should have audio rendered from both or either note
        REQUIRE(buffer.getMagnitude(0, 0, 256) > 0.01f);

        // The UI queue should now be empty after processBlock
        juce::MidiBuffer remainingUi;
        processor.getUiMidiQueue().drainTo(remainingUi, 0);
        REQUIRE(remainingUi.isEmpty());
    }

    SECTION("tryPushBatch enqueues events atomically or not at all") {
        std::vector<juce::MidiMessage> batch{
            juce::MidiMessage::noteOn(1, 60, 0.8f),
            juce::MidiMessage::noteOn(1, 64, 0.8f),
            juce::MidiMessage::noteOn(1, 67, 0.8f),
            juce::MidiMessage::noteOn(1, 71, 0.8f)
        };

        // 1. Order preservation in batch
        REQUIRE(queue.tryPushBatch(batch));

        juce::MidiBuffer dest;
        queue.drainTo(dest, 0);

        REQUIRE(dest.getNumEvents() == 4);
        int expectedNotes[] = {60, 64, 67, 71};
        int idx = 0;
        for (const auto meta : dest) {
            REQUIRE(meta.getMessage().getNoteNumber() == expectedNotes[idx++]);
        }

        // 2. Empty batch returns true and changes nothing
        REQUIRE(queue.tryPushBatch({}));
        juce::MidiBuffer emptyDest;
        queue.drainTo(emptyDest, 0);
        REQUIRE(emptyDest.isEmpty());

        // 3. Batch larger than entire capacity (256) returns false
        std::vector<juce::MidiMessage> hugeBatch(300, juce::MidiMessage::noteOn(1, 60, 0.5f));
        REQUIRE_FALSE(queue.tryPushBatch(hugeBatch));

        // 4. Atomic rejection when insufficient remaining capacity
        // Fill queue up to 254 items (capacity is 256, so 2 spots remain)
        for (int i = 0; i < 254; ++i) {
            REQUIRE(queue.push(juce::MidiMessage::noteOn(1, 50, 0.5f)));
        }

        // Attempting to push a batch of 4 notes when only 2 spots remain must fail
        // and must NOT write any of the 4 events
        REQUIRE_FALSE(queue.tryPushBatch(batch));

        // Drain the 254 items and verify no partial batch was pushed
        juce::MidiBuffer drainFull;
        queue.drainTo(drainFull, 0);
        REQUIRE(drainFull.getNumEvents() == 254);

        // Queue is now empty, pushing batch of 4 should succeed
        REQUIRE(queue.tryPushBatch(batch));
        juce::MidiBuffer finalDrain;
        queue.drainTo(finalDrain, 0);
        REQUIRE(finalDrain.getNumEvents() == 4);
    }
}
