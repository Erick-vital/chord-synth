#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include "music/Arpeggiator.h"
#include <vector>

using chordsynth::music::Arpeggiator;
using chordsynth::music::ArpMode;
using chordsynth::music::ArpRate;

namespace {

std::vector<int> collectArpeggiatedNotes(Arpeggiator& arp, int numSteps, double bpm, double sampleRate) {
    std::vector<int> notes;
    const int stepSamples = static_cast<int>(std::round((60.0 / bpm) * 0.5 * sampleRate)); // 1/8 note
    const int blockSize = 64;

    for (int s = 0; s < numSteps; ++s) {
        // Run samples up to stepSamples
        for (int sample = 0; sample < stepSamples; sample += blockSize) {
            juce::MidiBuffer outMidi;
            arp.processBlock(outMidi, blockSize, bpm);
            for (const auto meta : outMidi) {
                auto msg = meta.getMessage();
                if (msg.isNoteOn()) {
                    notes.push_back(msg.getNoteNumber());
                }
            }
        }
    }
    return notes;
}

} // namespace

TEST_CASE("Arpeggiator Up orders notes low to high", "[music][arp]") {
    Arpeggiator arp;
    arp.prepare(48000.0);
    arp.setMode(ArpMode::up);
    arp.setRate(ArpRate::eighth);
    arp.setEnabled(true);

    arp.noteOn(60);
    arp.noteOn(67);
    arp.noteOn(64); // chord notes C4, G4, E4 -> ordered 60, 64, 67

    auto notes = collectArpeggiatedNotes(arp, 6, 120.0, 48000.0);
    REQUIRE(notes.size() >= 6);
    REQUIRE(notes[0] == 60);
    REQUIRE(notes[1] == 64);
    REQUIRE(notes[2] == 67);
    REQUIRE(notes[3] == 60);
    REQUIRE(notes[4] == 64);
    REQUIRE(notes[5] == 67);
}

TEST_CASE("Arpeggiator Down orders notes high to low", "[music][arp]") {
    Arpeggiator arp;
    arp.prepare(48000.0);
    arp.setMode(ArpMode::down);
    arp.setRate(ArpRate::eighth);
    arp.setEnabled(true);

    arp.noteOn(60);
    arp.noteOn(67);
    arp.noteOn(64); // chord notes C4, G4, E4 -> down ordered 67, 64, 60

    auto notes = collectArpeggiatedNotes(arp, 6, 120.0, 48000.0);
    REQUIRE(notes.size() >= 6);
    REQUIRE(notes[0] == 67);
    REQUIRE(notes[1] == 64);
    REQUIRE(notes[2] == 60);
    REQUIRE(notes[3] == 67);
    REQUIRE(notes[4] == 64);
    REQUIRE(notes[5] == 60);
}

TEST_CASE("Arpeggiator Up/Down does not double extreme notes", "[music][arp]") {
    Arpeggiator arp;
    arp.prepare(48000.0);
    arp.setMode(ArpMode::upDown);
    arp.setRate(ArpRate::eighth);
    arp.setEnabled(true);

    arp.noteOn(60);
    arp.noteOn(64);
    arp.noteOn(67); // C4, E4, G4 -> Up/Down: 60, 64, 67, 64, 60, 64...

    auto notes = collectArpeggiatedNotes(arp, 6, 120.0, 48000.0);
    REQUIRE(notes.size() >= 6);
    REQUIRE(notes[0] == 60);
    REQUIRE(notes[1] == 64);
    REQUIRE(notes[2] == 67);
    REQUIRE(notes[3] == 64);
    REQUIRE(notes[4] == 60);
    REQUIRE(notes[5] == 64);
}

TEST_CASE("Arpeggiator Random is deterministic with seeded RNG", "[music][arp]") {
    Arpeggiator arp1;
    arp1.prepare(48000.0);
    arp1.setMode(ArpMode::random);
    arp1.setRate(ArpRate::eighth);
    arp1.setEnabled(true);
    arp1.setSeed(12345);

    Arpeggiator arp2;
    arp2.prepare(48000.0);
    arp2.setMode(ArpMode::random);
    arp2.setRate(ArpRate::eighth);
    arp2.setEnabled(true);
    arp2.setSeed(12345);

    arp1.noteOn(60);
    arp1.noteOn(64);
    arp1.noteOn(67);

    arp2.noteOn(60);
    arp2.noteOn(64);
    arp2.noteOn(67);

    auto notes1 = collectArpeggiatedNotes(arp1, 10, 120.0, 48000.0);
    auto notes2 = collectArpeggiatedNotes(arp2, 10, 120.0, 48000.0);

    REQUIRE(notes1 == notes2);
}

TEST_CASE("Arpeggiator emits note-off before new note-on to prevent hanging notes", "[music][arp]") {
    Arpeggiator arp;
    arp.prepare(48000.0);
    arp.setMode(ArpMode::up);
    arp.setRate(ArpRate::eighth);
    arp.setEnabled(true);

    arp.noteOn(60);
    arp.noteOn(64);

    const int stepSamples = 12000;
    juce::MidiBuffer outMidi;
    arp.processBlock(outMidi, stepSamples * 2 + 100, 120.0);

    int noteOnCount = 0;
    int noteOffCount = 0;
    for (const auto meta : outMidi) {
        auto msg = meta.getMessage();
        if (msg.isNoteOn()) ++noteOnCount;
        if (msg.isNoteOff()) ++noteOffCount;
    }

    REQUIRE(noteOnCount >= 2);
    REQUIRE(noteOffCount >= 1);
}
