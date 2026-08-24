#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <vector>
#include "interaction/MidiPerformanceMapper.h"
#include "music/HarmonyConfiguration.h"
#include "music/DiatonicChordVoicer.h"
#include "music/VoiceLeadingResolver.h"

using namespace chordsynth;
using namespace chordsynth::interaction;
using namespace chordsynth::music;

namespace {

struct ParsedEvent {
    int channel{1};
    int noteNumber{0};
    bool isNoteOn{false};
    bool isNoteOff{false};
    bool isController{false};
    int ccNumber{0};
    int ccValue{0};
    float velocity{0.0f};
    int sampleOffset{0};
};

std::vector<ParsedEvent> parseMidiBuffer(const juce::MidiBuffer& buffer) {
    std::vector<ParsedEvent> events;
    for (const auto meta : buffer) {
        const auto msg = meta.getMessage();
        ParsedEvent ev;
        ev.channel = msg.getChannel();
        ev.sampleOffset = meta.samplePosition;
        if (msg.isNoteOn()) {
            ev.isNoteOn = true;
            ev.noteNumber = msg.getNoteNumber();
            ev.velocity = msg.getFloatVelocity();
        } else if (msg.isNoteOff()) {
            ev.isNoteOff = true;
            ev.noteNumber = msg.getNoteNumber();
            ev.velocity = msg.getFloatVelocity();
        } else if (msg.isController()) {
            ev.isController = true;
            ev.ccNumber = msg.getControllerNumber();
            ev.ccValue = msg.getControllerValue();
        }
        events.push_back(ev);
    }
    return events;
}

} // namespace

TEST_CASE("MidiPerformanceMapper disabled mode passes all MIDI unchanged", "[interaction][midi_mapper]") {
    HarmonyConfiguration config;
    DiatonicChordVoicer voicer;
    MidiPerformanceMapper mapper(config, voicer);

    mapper.setEnabled(false);
    REQUIRE_FALSE(mapper.getEnabled());

    juce::MidiBuffer input;
    input.addEvent(juce::MidiMessage::noteOn(1, 36, 0.7f), 10);
    input.addEvent(juce::MidiMessage::controllerEvent(1, 20, 100), 20);
    input.addEvent(juce::MidiMessage::noteOff(1, 36, 0.0f), 30);
    input.addEvent(juce::MidiMessage::noteOn(3, 60, 0.9f), 40);

    juce::MidiBuffer output;
    mapper.processBlock(input, output, 128);

    auto events = parseMidiBuffer(output);
    REQUIRE(events.size() == 4);

    REQUIRE(events[0].isNoteOn);
    REQUIRE(events[0].noteNumber == 36);
    REQUIRE(events[0].sampleOffset == 10);

    REQUIRE(events[1].isController);
    REQUIRE(events[1].ccNumber == 20);
    REQUIRE(events[1].ccValue == 100);
    REQUIRE(events[1].sampleOffset == 20);

    REQUIRE(events[2].isNoteOff);
    REQUIRE(events[2].noteNumber == 36);
    REQUIRE(events[2].sampleOffset == 30);

    REQUIRE(events[3].isNoteOn);
    REQUIRE(events[3].channel == 3);
    REQUIRE(events[3].noteNumber == 60);
    REQUIRE(events[3].sampleOffset == 40);
}

TEST_CASE("MidiPerformanceMapper enabled maps notes 36-42 to degrees 0-6 with bass on ch 2", "[interaction][midi_mapper]") {
    HarmonyConfiguration config;
    // Scene A: Diatónica (triads, compact, manual leading, no bass)
    // Degree 0 (I) in C major = C4, E4, G4 (MIDI 60, 64, 67)
    DiatonicChordVoicer voicer;
    MidiPerformanceMapper mapper(config, voicer);

    mapper.setEnabled(true);
    REQUIRE(mapper.getEnabled());

    MidiPerformanceMapper::Context ctx{
        .tonic = 0,
        .scale = Scale::major,
        .diatonicMode = true,
        .sceneIndex = 0, // Scene A
        .palette = TransformPalette::loFi
    };
    mapper.setContext(ctx);

    SECTION("Note 36 triggers Degree 0 chord and preserves sample offset & velocity") {
        juce::MidiBuffer input;
        input.addEvent(juce::MidiMessage::noteOn(1, 36, 0.85f), 16);

        juce::MidiBuffer output;
        mapper.processBlock(input, output, 64);

        REQUIRE(mapper.hasActiveChord());
        REQUIRE(mapper.getActiveDegree() == 0);

        auto events = parseMidiBuffer(output);
        REQUIRE(events.size() == 3); // C4, E4, G4
        for (const auto& ev : events) {
            REQUIRE(ev.channel == 1);
            REQUIRE(ev.isNoteOn);
            REQUIRE(ev.velocity == Catch::Approx(0.85f).margin(0.01f));
            REQUIRE(ev.sampleOffset == 16);
        }
        REQUIRE(events[0].noteNumber == 48);
        REQUIRE(events[1].noteNumber == 52);
        REQUIRE(events[2].noteNumber == 55);

        // Note-off releases active chord
        input.clear();
        input.addEvent(juce::MidiMessage::noteOff(1, 36, 0.0f), 32);
        output.clear();
        mapper.processBlock(input, output, 64);

        REQUIRE_FALSE(mapper.hasActiveChord());
        events = parseMidiBuffer(output);
        REQUIRE(events.size() == 3);
        for (const auto& ev : events) {
            REQUIRE(ev.channel == 1);
            REQUIRE(ev.isNoteOff);
            REQUIRE(ev.sampleOffset == 32);
        }
    }

    SECTION("Scene C Lo-Fi Warm triggers separate bass on channel 2") {
        ctx.sceneIndex = 2; // Scene C has Root bass
        mapper.setContext(ctx);

        juce::MidiBuffer input;
        input.addEvent(juce::MidiMessage::noteOn(1, 36, 0.75f), 0);

        juce::MidiBuffer output;
        mapper.processBlock(input, output, 64);

        auto events = parseMidiBuffer(output);
        // Harmonic notes on channel 1, Bass on channel 2
        bool foundCh1 = false;
        bool foundCh2 = false;
        for (const auto& ev : events) {
            if (ev.channel == 1) foundCh1 = true;
            if (ev.channel == 2) {
                foundCh2 = true;
                REQUIRE(ev.noteNumber >= 24);
                REQUIRE(ev.noteNumber <= 47);
            }
        }
        REQUIRE(foundCh1);
        REQUIRE(foundCh2);

        // Releasing note 36 releases both ch1 and ch2 notes
        input.clear();
        input.addEvent(juce::MidiMessage::noteOff(1, 36, 0.0f), 10);
        output.clear();
        mapper.processBlock(input, output, 64);

        events = parseMidiBuffer(output);
        bool offCh1 = false;
        bool offCh2 = false;
        for (const auto& ev : events) {
            REQUIRE(ev.isNoteOff);
            if (ev.channel == 1) offCh1 = true;
            if (ev.channel == 2) offCh2 = true;
        }
        REQUIRE(offCh1);
        REQUIRE(offCh2);
    }
}

TEST_CASE("MidiPerformanceMapper passes through unmapped notes and MIDI messages", "[interaction][midi_mapper]") {
    HarmonyConfiguration config;
    DiatonicChordVoicer voicer;
    MidiPerformanceMapper mapper(config, voicer);
    mapper.setEnabled(true);

    juce::MidiBuffer input;
    input.addEvent(juce::MidiMessage::noteOn(1, 60, 0.5f), 5); // Unmapped note 60
    input.addEvent(juce::MidiMessage::pitchWheel(1, 8192), 15);
    input.addEvent(juce::MidiMessage::controllerEvent(1, 1, 64), 25); // Modulation wheel

    juce::MidiBuffer output;
    mapper.processBlock(input, output, 64);

    auto events = parseMidiBuffer(output);
    REQUIRE(events.size() == 3);
    REQUIRE(events[0].isNoteOn);
    REQUIRE(events[0].noteNumber == 60);
    REQUIRE(events[0].sampleOffset == 5);
    REQUIRE(events[2].isController);
    REQUIRE(events[2].ccNumber == 1);
    REQUIRE(events[2].sampleOffset == 25);
}

TEST_CASE("MidiPerformanceMapper CC 20-27 applies temporary chord transforms with differential notes", "[interaction][midi_mapper]") {
    HarmonyConfiguration config;
    DiatonicChordVoicer voicer;
    MidiPerformanceMapper mapper(config, voicer);
    mapper.setEnabled(true);

    MidiPerformanceMapper::Context ctx{
        .tonic = 0,
        .scale = Scale::major,
        .diatonicMode = true,
        .sceneIndex = 0, // Scene A: Triads
        .palette = TransformPalette::basic
    };
    mapper.setContext(ctx);

    // 1. Play C Major triad (degree 0): 60, 64, 67
    juce::MidiBuffer input;
    input.addEvent(juce::MidiMessage::noteOn(1, 36, 0.8f), 0);
    juce::MidiBuffer output;
    mapper.processBlock(input, output, 64);

    REQUIRE(mapper.hasActiveChord());
    REQUIRE_FALSE(mapper.getActiveTransformSlot().has_value());

    // 2. Send CC 20 (Slot 1: Major/Minor flip in Basic palette -> C Minor triad: 60, 63, 67)
    input.clear();
    input.addEvent(juce::MidiMessage::controllerEvent(1, 20, 127), 10);
    output.clear();
    mapper.processBlock(input, output, 64);

    REQUIRE(mapper.getActiveTransformSlot() == TransformSlot::one);
    auto events = parseMidiBuffer(output);
    // Differential voicing: Note 52 (E3) is removed, Note 51 (D#3) is added. Note 48 and 55 remain sounding.
    REQUIRE(events.size() == 2);
    REQUIRE(events[0].isNoteOff);
    REQUIRE(events[0].noteNumber == 52);
    REQUIRE(events[0].sampleOffset == 10);
    REQUIRE(events[1].isNoteOn);
    REQUIRE(events[1].noteNumber == 51);
    REQUIRE(events[1].sampleOffset == 10);

    // 3. Release CC 20 (Value < 64) -> restores C Major triad (48, 52, 55)
    input.clear();
    input.addEvent(juce::MidiMessage::controllerEvent(1, 20, 0), 20);
    output.clear();
    mapper.processBlock(input, output, 64);

    REQUIRE_FALSE(mapper.getActiveTransformSlot().has_value());
    events = parseMidiBuffer(output);
    REQUIRE(events.size() == 2);
    REQUIRE(events[0].isNoteOff);
    REQUIRE(events[0].noteNumber == 51);
    REQUIRE(events[1].isNoteOn);
    REQUIRE(events[1].noteNumber == 52);

    // 4. CC when no chord is held is handled safely without notes emitted
    input.clear();
    input.addEvent(juce::MidiMessage::noteOff(1, 36, 0.0f), 0);
    output.clear();
    mapper.processBlock(input, output, 64);
    REQUIRE_FALSE(mapper.hasActiveChord());

    input.clear();
    input.addEvent(juce::MidiMessage::controllerEvent(1, 21, 100), 5);
    output.clear();
    mapper.processBlock(input, output, 64);
    events = parseMidiBuffer(output);
    REQUIRE(events.empty());
}

TEST_CASE("MidiPerformanceMapper keeps a held transform across degree presses and fallback", "[interaction][midi_mapper]") {
    HarmonyConfiguration config;
    DiatonicChordVoicer voicer;
    MidiPerformanceMapper mapper(config, voicer);
    mapper.setEnabled(true);
    mapper.setContext({
        .tonic = 0,
        .scale = Scale::major,
        .diatonicMode = true,
        .sceneIndex = 0,
        .palette = TransformPalette::basic
    });

    juce::MidiBuffer input;
    juce::MidiBuffer output;

    // A held CC transform must affect a chord played after the CC.
    input.addEvent(juce::MidiMessage::controllerEvent(1, 20, 127), 0);
    input.addEvent(juce::MidiMessage::noteOn(1, 36, 0.8f), 1);
    mapper.processBlock(input, output, 64);
    REQUIRE(mapper.getActiveTransformSlot() == TransformSlot::one);
    REQUIRE(mapper.getActiveHarmonicNotes() == NoteSet({48, 51, 55}, 3)); // C minor

    // Keeping the CC held applies the same transform to the next degree.
    input.clear();
    output.clear();
    input.addEvent(juce::MidiMessage::noteOn(1, 37, 0.8f), 2);
    mapper.processBlock(input, output, 64);
    REQUIRE(mapper.getActiveDegree() == 1);
    REQUIRE(mapper.getActiveTransformSlot() == TransformSlot::one);
    REQUIRE(mapper.getActiveHarmonicNotes() == NoteSet({50, 54, 57}, 3)); // D major

    // Releasing the newer degree returns to the still-held prior degree with its transform.
    input.clear();
    output.clear();
    input.addEvent(juce::MidiMessage::noteOff(1, 37), 3);
    mapper.processBlock(input, output, 64);
    REQUIRE(mapper.getActiveDegree() == 0);
    REQUIRE(mapper.getActiveTransformSlot() == TransformSlot::one);
    REQUIRE(mapper.getActiveHarmonicNotes() == NoteSet({48, 51, 55}, 3)); // C minor
}

TEST_CASE("MidiPerformanceMapper all-notes-off and reset clears active notes safely", "[interaction][midi_mapper]") {
    HarmonyConfiguration config;
    DiatonicChordVoicer voicer;
    MidiPerformanceMapper mapper(config, voicer);
    mapper.setEnabled(true);

    MidiPerformanceMapper::Context ctx{
        .tonic = 0,
        .scale = Scale::major,
        .diatonicMode = true,
        .sceneIndex = 2, // Scene C (Harmonic notes + bass)
        .palette = TransformPalette::loFi
    };
    mapper.setContext(ctx);

    juce::MidiBuffer input;
    input.addEvent(juce::MidiMessage::noteOn(1, 36, 0.8f), 0);
    juce::MidiBuffer output;
    mapper.processBlock(input, output, 64);
    REQUIRE(mapper.hasActiveChord());

    SECTION("CC 123 (All Notes Off) releases mapped notes") {
        input.clear();
        input.addEvent(juce::MidiMessage::allNotesOff(1), 10);
        output.clear();
        mapper.processBlock(input, output, 64);

        REQUIRE_FALSE(mapper.hasActiveChord());
        auto events = parseMidiBuffer(output);
        // Contains noteOffs for active notes plus the pass-through CC 123 event
        bool sawNoteOff = false;
        bool sawAllNotesOffMsg = false;
        for (const auto& ev : events) {
            if (ev.isNoteOff) sawNoteOff = true;
            if (ev.isController && ev.ccNumber == 123) sawAllNotesOffMsg = true;
        }
        REQUIRE(sawNoteOff);
        REQUIRE(sawAllNotesOffMsg);
    }

    SECTION("Explicit reset() resets internal state") {
        mapper.reset();
        REQUIRE_FALSE(mapper.hasActiveChord());
        REQUIRE_FALSE(mapper.getActiveTransformSlot().has_value());
    }
}

TEST_CASE("MidiPerformanceMapper repeated and overlapping degree notes retain trigger ownership", "[interaction][midi_mapper]") {
    HarmonyConfiguration config;
    DiatonicChordVoicer voicer;
    MidiPerformanceMapper mapper(config, voicer);
    mapper.setEnabled(true);

    juce::MidiBuffer input;
    juce::MidiBuffer output;

    input.addEvent(juce::MidiMessage::noteOn(1, 36, 0.8f), 0);
    input.addEvent(juce::MidiMessage::noteOn(1, 36, 0.8f), 1);
    mapper.processBlock(input, output, 64);
    REQUIRE(mapper.getActiveDegree() == 0);

    input.clear();
    output.clear();
    input.addEvent(juce::MidiMessage::noteOff(1, 36), 2);
    mapper.processBlock(input, output, 64);
    REQUIRE(mapper.getActiveDegree() == 0);
    REQUIRE(parseMidiBuffer(output).empty());

    input.clear();
    output.clear();
    input.addEvent(juce::MidiMessage::noteOn(1, 37, 0.9f), 3);
    mapper.processBlock(input, output, 64);
    REQUIRE(mapper.getActiveDegree() == 1);

    input.clear();
    output.clear();
    input.addEvent(juce::MidiMessage::noteOff(1, 37), 4);
    mapper.processBlock(input, output, 64);
    REQUIRE(mapper.getActiveDegree() == 0);
    REQUIRE_FALSE(parseMidiBuffer(output).empty());

    input.clear();
    output.clear();
    input.addEvent(juce::MidiMessage::noteOff(1, 36), 5);
    mapper.processBlock(input, output, 64);
    REQUIRE_FALSE(mapper.hasActiveChord());
}

TEST_CASE("MidiPerformanceMapper applies nearest voice leading from sounding notes", "[interaction][midi_mapper]") {
    HarmonyConfiguration config;
    DiatonicChordVoicer voicer;
    MidiPerformanceMapper mapper(config, voicer);
    mapper.setEnabled(true);
    mapper.setContext({
        .tonic = 0,
        .scale = Scale::major,
        .diatonicMode = true,
        .sceneIndex = 1,
        .palette = TransformPalette::loFi
    });

    juce::MidiBuffer input;
    juce::MidiBuffer output;
    input.addEvent(juce::MidiMessage::noteOn(1, 42, 0.8f), 0);
    mapper.processBlock(input, output, 64);
    const auto previous = mapper.getActiveHarmonicNotes();

    const auto spec = config.getSpec(1, 0);
    const auto defaultTarget = voicer.voiceChord(0, 0, spec, Scale::major).notes;
    const auto nearestTarget = VoiceLeadingResolver::resolveNearestVoiceLeading(previous, defaultTarget);
    REQUIRE(nearestTarget != defaultTarget);

    input.clear();
    output.clear();
    input.addEvent(juce::MidiMessage::noteOn(1, 36, 0.8f), 1);
    mapper.processBlock(input, output, 64);
    REQUIRE(mapper.getActiveHarmonicNotes() == nearestTarget);
}

TEST_CASE("MidiPerformanceMapper in-place processBlock overload works identically", "[interaction][midi_mapper]") {
    HarmonyConfiguration config;
    DiatonicChordVoicer voicer;
    MidiPerformanceMapper mapper(config, voicer);
    mapper.setEnabled(true);

    juce::MidiBuffer buffer;
    buffer.addEvent(juce::MidiMessage::noteOn(1, 36, 0.8f), 12);

    mapper.processBlock(buffer, 64);

    REQUIRE(mapper.hasActiveChord());
    auto events = parseMidiBuffer(buffer);
    REQUIRE_FALSE(events.empty());
    REQUIRE(events[0].sampleOffset == 12);
}
