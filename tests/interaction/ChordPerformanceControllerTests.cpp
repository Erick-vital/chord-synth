#include <catch2/catch_test_macros.hpp>
#include "interaction/ChordPerformanceController.h"
#include <vector>

using namespace chordsynth;

class MockMidiOutput : public interaction::MidiBatchOutput {
public:
    bool tryPushBatch(std::span<const juce::MidiMessage> messages) noexcept override {
        if (failPushes) {
            return false;
        }
        for (const auto& msg : messages) {
            pushedMessages.push_back(msg);
        }
        return true;
    }

    std::vector<juce::MidiMessage> pushedMessages;
    bool failPushes{false};
};

TEST_CASE("ChordPerformanceController press, release and lifecycle", "[interaction][controller]") {
    music::HarmonyConfiguration config;
    music::DiatonicChordVoicer voicer;
    MockMidiOutput output;

    interaction::ChordPerformanceController controller(config, voicer, output);

    // Initial state: C major (tonic = 0), Scene A (triads, root, close)
    controller.setTonic(0);
    controller.setScene(0);
    controller.setLiveRevoice(false);

    SECTION("pressDegree produces note-on events and updates active chord") {
        // Degree 0 (I in C major) -> C3(48), E3(52), G3(55)
        REQUIRE(controller.pressDegree(0, 0.8f));
        REQUIRE(controller.getActiveChord().has_value());
        REQUIRE(controller.getActiveChord()->degree == 0);
        REQUIRE(controller.getActiveChord()->notes.size() == 3);

        REQUIRE(output.pushedMessages.size() == 3);
        for (const auto& msg : output.pushedMessages) {
            REQUIRE(msg.isNoteOn());
        }
        REQUIRE(output.pushedMessages[0].getNoteNumber() == 48);
        REQUIRE(output.pushedMessages[1].getNoteNumber() == 52);
        REQUIRE(output.pushedMessages[2].getNoteNumber() == 55);
    }

    SECTION("Diatonic mode ignores a saved manual quality override") {
        music::VoicingSpec overriddenSpec;
        overriddenSpec.qualityRule = music::QualityRule::major;
        config.setSpec(0, 1, overriddenSpec);

        controller.setDiatonicMode(true);
        REQUIRE(controller.pressDegree(1, 0.8f));
        REQUIRE(controller.getActiveChord()->notes == music::NoteSet({50, 53, 57}, 3));

        controller.releaseActiveChord();
        output.pushedMessages.clear();
        controller.setDiatonicMode(false);
        REQUIRE(controller.pressDegree(1, 0.8f));
        REQUIRE(controller.getActiveChord()->notes == music::NoteSet({50, 54, 57}, 3));
    }

    SECTION("releaseActiveChord produces note-off events for active notes and clears state") {
        REQUIRE(controller.pressDegree(0, 0.8f));
        output.pushedMessages.clear();

        controller.releaseActiveChord();
        REQUIRE_FALSE(controller.getActiveChord().has_value());

        REQUIRE(output.pushedMessages.size() == 3);
        for (const auto& msg : output.pushedMessages) {
            REQUIRE(msg.isNoteOff());
        }
        REQUIRE(output.pushedMessages[0].getNoteNumber() == 48);
        REQUIRE(output.pushedMessages[1].getNoteNumber() == 52);
        REQUIRE(output.pushedMessages[2].getNoteNumber() == 55);

        // Calling release again is safe and produces no new events
        output.pushedMessages.clear();
        controller.releaseActiveChord();
        REQUIRE(output.pushedMessages.empty());
    }

    SECTION("Changing tonic or scene before release does not corrupt note-off notes") {
        REQUIRE(controller.pressDegree(0, 0.8f));
        output.pushedMessages.clear();

        // Change tonic to G major (7) and scene to B (seventh) while chord is held
        controller.setTonic(7);
        controller.setScene(1);

        // Release must still turn off the C major triad (48, 52, 55), not G major seventh
        controller.releaseActiveChord();
        REQUIRE_FALSE(controller.getActiveChord().has_value());

        REQUIRE(output.pushedMessages.size() == 3);
        REQUIRE(output.pushedMessages[0].getNoteNumber() == 48);
        REQUIRE(output.pushedMessages[1].getNoteNumber() == 52);
        REQUIRE(output.pushedMessages[2].getNoteNumber() == 55);
        for (const auto& msg : output.pushedMessages) {
            REQUIRE(msg.isNoteOff());
        }
    }

    SECTION("Pressing another degree releases the active chord first then activates the new one") {
        REQUIRE(controller.pressDegree(0, 0.8f)); // C triad: 48, 52, 55
        output.pushedMessages.clear();

        // Press degree 1 (ii in C major -> Dm triad: 50, 53, 57)
        REQUIRE(controller.pressDegree(1, 0.9f));
        REQUIRE(controller.getActiveChord().has_value());
        REQUIRE(controller.getActiveChord()->degree == 1);

        // Must have sent note-offs for 48, 52, 55, then note-ons for 50, 53, 57
        REQUIRE(output.pushedMessages.size() == 6);
        REQUIRE(output.pushedMessages[0].isNoteOff());
        REQUIRE(output.pushedMessages[0].getNoteNumber() == 48);
        REQUIRE(output.pushedMessages[1].isNoteOff());
        REQUIRE(output.pushedMessages[1].getNoteNumber() == 52);
        REQUIRE(output.pushedMessages[2].isNoteOff());
        REQUIRE(output.pushedMessages[2].getNoteNumber() == 55);

        REQUIRE(output.pushedMessages[3].isNoteOn());
        REQUIRE(output.pushedMessages[3].getNoteNumber() == 50);
        REQUIRE(output.pushedMessages[4].isNoteOn());
        REQUIRE(output.pushedMessages[4].getNoteNumber() == 53);
        REQUIRE(output.pushedMessages[5].isNoteOn());
        REQUIRE(output.pushedMessages[5].getNoteNumber() == 57);
    }

    SECTION("Repeated press of the same degree does not duplicate note-ons") {
        REQUIRE(controller.pressDegree(0, 0.8f));
        output.pushedMessages.clear();

        // Repeating press on 0 while already active
        REQUIRE(controller.pressDegree(0, 0.8f));
        REQUIRE(output.pushedMessages.empty());
    }

    SECTION("Scene change with liveRevoice=false leaves held chord intact") {
        REQUIRE(controller.pressDegree(0, 0.8f));
        output.pushedMessages.clear();

        controller.setLiveRevoice(false);
        controller.setScene(1); // Scene B = seventh chords

        // No MIDI should have been output
        REQUIRE(output.pushedMessages.empty());
        REQUIRE(controller.getActiveChord()->notes.size() == 3);
    }

    SECTION("Scene change with liveRevoice=true calculates differential note changes") {
        controller.setLiveRevoice(true);
        REQUIRE(controller.pressDegree(0, 0.8f)); // C triad: 48, 52, 55
        output.pushedMessages.clear();

        // 1. C triad -> Cmaj7 (Scene B: 48, 52, 55, 59)
        // Only 59 (B) is added, 48, 52, 55 are common
        controller.setScene(1);
        REQUIRE(controller.getActiveChord()->notes.size() == 4);
        REQUIRE(output.pushedMessages.size() == 1);
        REQUIRE(output.pushedMessages[0].isNoteOn());
        REQUIRE(output.pushedMessages[0].getNoteNumber() == 59);

        // 2. Cmaj7 -> C 1st inversion triad (Scene D: 52, 55, 60)
        // Common notes: 52, 55. Removed: 48, 59. Added: 60.
        // Offs should be enqueued before ons!
        output.pushedMessages.clear();
        controller.setScene(3); // Scene D
        REQUIRE(controller.getActiveChord()->notes.size() == 3);
        REQUIRE(output.pushedMessages.size() == 3);
        // Removed notes (note-offs)
        REQUIRE(output.pushedMessages[0].isNoteOff());
        REQUIRE(output.pushedMessages[0].getNoteNumber() == 48);
        REQUIRE(output.pushedMessages[1].isNoteOff());
        REQUIRE(output.pushedMessages[1].getNoteNumber() == 59);
        // Added notes (note-ons)
        REQUIRE(output.pushedMessages[2].isNoteOn());
        REQUIRE(output.pushedMessages[2].getNoteNumber() == 60);

        // 3. Release active chord after re-voicing turns off the updated active notes (52, 55, 60)
        output.pushedMessages.clear();
        controller.releaseActiveChord();
        REQUIRE(output.pushedMessages.size() == 3);
        REQUIRE(output.pushedMessages[0].isNoteOff());
        REQUIRE(output.pushedMessages[0].getNoteNumber() == 52);
        REQUIRE(output.pushedMessages[1].isNoteOff());
        REQUIRE(output.pushedMessages[1].getNoteNumber() == 55);
        REQUIRE(output.pushedMessages[2].isNoteOff());
        REQUIRE(output.pushedMessages[2].getNoteNumber() == 60);
    }

    SECTION("revoiceActiveChordIfHeld updates held chord if liveRevoice is enabled") {
        controller.setLiveRevoice(true);
        REQUIRE(controller.pressDegree(0, 0.8f)); // C triad: 48, 52, 55
        output.pushedMessages.clear();

        // Mutate spec for degree 0 in Scene A to seventh chord
        music::VoicingSpec seventhSpec;
        seventhSpec.extension = music::ChordExtension::seventh;
        config.setSpec(0, 0, seventhSpec);

        // Calling revoiceActiveChordIfHeld(0) while holding degree 0
        controller.revoiceActiveChordIfHeld(0);
        REQUIRE(controller.getActiveChord()->notes.size() == 4);
        REQUIRE(output.pushedMessages.size() == 1);
        REQUIRE(output.pushedMessages[0].isNoteOn());
        REQUIRE(output.pushedMessages[0].getNoteNumber() == 59);

        // Calling revoiceActiveChordIfHeld on another degree (e.g. 1) does nothing
        output.pushedMessages.clear();
        controller.revoiceActiveChordIfHeld(1);
        REQUIRE(output.pushedMessages.empty());

        // With liveRevoice disabled, revoiceActiveChordIfHeld does nothing
        controller.setLiveRevoice(false);
        output.pushedMessages.clear();
        controller.revoiceActiveChordIfHeld(0);
        REQUIRE(output.pushedMessages.empty());
    }

    SECTION("allNotesOff, destructor and enqueue failure maintain consistent state") {
        REQUIRE(controller.pressDegree(0, 0.8f));
        output.pushedMessages.clear();

        controller.allNotesOff();
        REQUIRE_FALSE(controller.getActiveChord().has_value());
        REQUIRE(output.pushedMessages.size() == 3);

        // Output failure simulation:
        output.failPushes = true;
        output.pushedMessages.clear();
        REQUIRE_FALSE(controller.pressDegree(0, 0.8f));
        // Active chord must NOT be set if push failed
        REQUIRE_FALSE(controller.getActiveChord().has_value());
    }

    SECTION("Destructor sends note-offs for currently active chord") {
        {
            interaction::ChordPerformanceController localController(config, voicer, output);
            localController.setTonic(0);
            localController.setScene(0);
            output.failPushes = false;
            REQUIRE(localController.pressDegree(0, 0.8f));
            output.pushedMessages.clear();
            // localController goes out of scope here
        }
        REQUIRE(output.pushedMessages.size() == 3);
        for (const auto& msg : output.pushedMessages) {
            REQUIRE(msg.isNoteOff());
        }
    }

    SECTION("QueueMidiBatchOutput adapter integrates cleanly with UiMidiQueue") {
        dsp::UiMidiQueue realQueue;
        interaction::QueueMidiBatchOutput realOutput(realQueue);
        interaction::ChordPerformanceController controllerWithRealQueue(config, voicer, realOutput);

        controllerWithRealQueue.setTonic(0);
        controllerWithRealQueue.setScene(0);

        REQUIRE(controllerWithRealQueue.pressDegree(0, 0.8f));
        REQUIRE(controllerWithRealQueue.getActiveChord().has_value());

        juce::MidiBuffer dest;
        realQueue.drainTo(dest, 0);
        REQUIRE(dest.getNumEvents() == 3);
        for (const auto meta : dest) {
            REQUIRE(meta.getMessage().isNoteOn());
        }
    }
}
