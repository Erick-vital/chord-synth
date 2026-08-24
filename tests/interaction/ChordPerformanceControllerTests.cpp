#include <catch2/catch_test_macros.hpp>
#include "interaction/ChordPerformanceController.h"
#include "music/VoiceLeadingResolver.h"
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

    SECTION("Nearest voice leading resolves the next degree from the sounding notes") {
        controller.setScene(1); // nearest by factory contract
        REQUIRE(controller.pressDegree(6, 0.8f));
        const auto previousNotes = controller.getActiveChord()->notes;
        output.pushedMessages.clear();

        const auto targetSpec = config.getSpec(1, 0);
        const auto defaultTarget = voicer.voiceChord(0, 0, targetSpec, music::Scale::major).notes;
        const auto nearestTarget = music::VoiceLeadingResolver::resolveNearestVoiceLeading(previousNotes, defaultTarget);
        REQUIRE(nearestTarget != defaultTarget);

        REQUIRE(controller.pressDegree(0, 0.8f));
        REQUIRE(controller.getActiveChord()->notes == nearestTarget);
    }

    SECTION("Failed release retains ownership so note-offs can be retried") {
        REQUIRE(controller.pressDegree(0, 0.8f));
        output.pushedMessages.clear();
        output.failPushes = true;

        controller.releaseActiveChord();
        REQUIRE(controller.getActiveChord().has_value());

        output.failPushes = false;
        controller.releaseActiveChord();
        REQUIRE_FALSE(controller.getActiveChord().has_value());
        REQUIRE(output.pushedMessages.size() == 3);
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
        // Explicitly set Scene A degree 0 to triad and Scene D degree 0 to 1st inversion triad for test predictability
        music::VoicingSpec specA{};
        specA.shape = music::ChordShape::triad;
        specA.style = music::VoicingStyle::compact;
        specA.inversion = 0;
        config.setSpec(0, 0, specA);

        music::VoicingSpec specB{};
        specB.shape = music::ChordShape::seventh;
        specB.style = music::VoicingStyle::compact;
        specB.inversion = 0;
        config.setSpec(1, 0, specB);

        music::VoicingSpec specD{};
        specD.shape = music::ChordShape::triad;
        specD.style = music::VoicingStyle::compact;
        specD.inversion = 1;
        config.setSpec(3, 0, specD);

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

    SECTION("Root and slash bass note handling in controller") {
        // Configure degree 0 with BassMode::root
        music::VoicingSpec rootBassSpec;
        rootBassSpec.bassMode = music::BassMode::root;
        config.setSpec(0, 0, rootBassSpec);

        // Configure degree 1 with BassMode::slashDegree (degree 0 = C)
        music::VoicingSpec slashBassSpec;
        slashBassSpec.bassMode = music::BassMode::slashDegree;
        slashBassSpec.slashDegree = 0;
        config.setSpec(0, 1, slashBassSpec);

        // 1. Press degree 0: should send note-ons for chord notes on channel 1 (or configured midiChannel) AND bass note on channel 2
        REQUIRE(controller.pressDegree(0, 0.8f));
        REQUIRE(controller.getActiveChord().has_value());
        REQUIRE(controller.getActiveChord()->bassMidi.has_value());
        REQUIRE(*controller.getActiveChord()->bassMidi == 36); // C2

        // Check output messages: 3 notes on ch 1 + 1 note on ch 2 = 4 messages
        REQUIRE(output.pushedMessages.size() == 4);
        int ch1Count = 0;
        int ch2Count = 0;
        for (const auto& msg : output.pushedMessages) {
            REQUIRE(msg.isNoteOn());
            if (msg.getChannel() == 1) {
                ch1Count++;
            } else if (msg.getChannel() == 2) {
                ch2Count++;
                REQUIRE(msg.getNoteNumber() == 36);
            }
        }
        REQUIRE(ch1Count == 3);
        REQUIRE(ch2Count == 1);

        // 2. Press degree 1 while degree 0 is active:
        // Should emit note-offs for old chord (ch1) AND old bass (ch2), then note-ons for new chord (ch1) AND new bass (ch2)
        output.pushedMessages.clear();
        REQUIRE(controller.pressDegree(1, 0.9f));
        REQUIRE(controller.getActiveChord().has_value());
        REQUIRE(controller.getActiveChord()->degree == 1);
        REQUIRE(controller.getActiveChord()->bassMidi.has_value());
        REQUIRE(*controller.getActiveChord()->bassMidi == 36); // Slash degree 0 in C major is C2 (36)

        // Old off: 3 (ch1) + 1 (ch2) = 4 offs
        // New on: 3 (ch1) + 1 (ch2) = 4 ons
        // Total = 8 messages
        REQUIRE(output.pushedMessages.size() == 8);
        int offCh1 = 0, offCh2 = 0, onCh1 = 0, onCh2 = 0;
        for (size_t i = 0; i < 4; ++i) {
            REQUIRE(output.pushedMessages[i].isNoteOff());
            if (output.pushedMessages[i].getChannel() == 1) offCh1++;
            if (output.pushedMessages[i].getChannel() == 2) offCh2++;
        }
        for (size_t i = 4; i < 8; ++i) {
            REQUIRE(output.pushedMessages[i].isNoteOn());
            if (output.pushedMessages[i].getChannel() == 1) onCh1++;
            if (output.pushedMessages[i].getChannel() == 2) onCh2++;
        }
        REQUIRE(offCh1 == 3);
        REQUIRE(offCh2 == 1);
        REQUIRE(onCh1 == 3);
        REQUIRE(onCh2 == 1);

        // 3. Release active chord: turns off harmonic notes on ch1 and bass on ch2
        output.pushedMessages.clear();
        controller.releaseActiveChord();
        REQUIRE_FALSE(controller.getActiveChord().has_value());
        REQUIRE(output.pushedMessages.size() == 4);
        offCh1 = 0; offCh2 = 0;
        for (const auto& msg : output.pushedMessages) {
            REQUIRE(msg.isNoteOff());
            if (msg.getChannel() == 1) offCh1++;
            if (msg.getChannel() == 2) offCh2++;
        }
        REQUIRE(offCh1 == 3);
        REQUIRE(offCh2 == 1);
    }

    SECTION("Live revoicing updates bass note independently on channel 2") {
        controller.setLiveRevoice(true);
        // Degree 0 starts with no bass
        music::VoicingSpec specNoBass;
        specNoBass.bassMode = music::BassMode::none;
        config.setSpec(0, 0, specNoBass);

        REQUIRE(controller.pressDegree(0, 0.8f));
        REQUIRE_FALSE(controller.getActiveChord()->bassMidi.has_value());
        output.pushedMessages.clear();

        // Mutate spec to BassMode::root
        music::VoicingSpec specWithBass;
        specWithBass.bassMode = music::BassMode::root;
        config.setSpec(0, 0, specWithBass);

        controller.revoiceActiveChordIfHeld(0);
        REQUIRE(controller.getActiveChord()->bassMidi.has_value());
        REQUIRE(*controller.getActiveChord()->bassMidi == 36);

        // Harmonic notes did not change, so only 1 noteOn on channel 2 for bass should be emitted
        REQUIRE(output.pushedMessages.size() == 1);
        REQUIRE(output.pushedMessages[0].isNoteOn());
        REQUIRE(output.pushedMessages[0].getChannel() == 2);
        REQUIRE(output.pushedMessages[0].getNoteNumber() == 36);

        // Change slashDegree from none to slashDegree = 2 (E2 -> 40)
        output.pushedMessages.clear();
        music::VoicingSpec specSlash;
        specSlash.bassMode = music::BassMode::slashDegree;
        specSlash.slashDegree = 2;
        config.setSpec(0, 0, specSlash);

        controller.revoiceActiveChordIfHeld(0);
        REQUIRE(controller.getActiveChord()->bassMidi.has_value());
        REQUIRE(*controller.getActiveChord()->bassMidi == 40);

        // Should emit note-off for old bass (36) on ch 2 and note-on for new bass (40) on ch 2
        REQUIRE(output.pushedMessages.size() == 2);
        REQUIRE(output.pushedMessages[0].isNoteOff());
        REQUIRE(output.pushedMessages[0].getChannel() == 2);
        REQUIRE(output.pushedMessages[0].getNoteNumber() == 36);
        REQUIRE(output.pushedMessages[1].isNoteOn());
        REQUIRE(output.pushedMessages[1].getChannel() == 2);
        REQUIRE(output.pushedMessages[1].getNoteNumber() == 40);
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

TEST_CASE("ChordPerformanceController temporary transform lifecycle", "[interaction][controller][transform]") {
    music::HarmonyConfiguration config;
    music::DiatonicChordVoicer voicer;
    MockMidiOutput output;

    interaction::ChordPerformanceController controller(config, voicer, output);
    controller.setTonic(0); // C Major
    controller.setScene(0); // Scene A (triads)
    controller.setLiveRevoice(false);

    SECTION("beginTransform without active chord updates transform state but emits no MIDI and returns true/false according to spec") {
        // Without active degree: transformedSpecForActiveDegree returns nullopt
        REQUIRE_FALSE(controller.getActiveChord().has_value());
        REQUIRE_FALSE(controller.transformedSpecForActiveDegree().has_value());
        REQUIRE_FALSE(controller.hasActiveTransform());

        // beginTransform stores visual selection without emitting notes
        bool beginResult = controller.beginTransform(interaction::TransformPalette::basic, interaction::TransformSlot::one);
        REQUIRE(controller.hasActiveTransform());
        auto activeTrans = controller.getActiveTransform();
        REQUIRE(activeTrans.has_value());
        REQUIRE(activeTrans->first == interaction::TransformPalette::basic);
        REQUIRE(activeTrans->second == interaction::TransformSlot::one);
        REQUIRE(output.pushedMessages.empty());
        REQUIRE_FALSE(controller.transformedSpecForActiveDegree().has_value());

        // ending transform clears selection
        controller.endTransform();
        REQUIRE_FALSE(controller.hasActiveTransform());
        REQUIRE(output.pushedMessages.empty());
    }

    SECTION("beginTransform on held chord computes transformed spec and sends differential MIDI") {
        // Press C major triad (deg 0: C3=48, E3=52, G3=55)
        REQUIRE(controller.pressDegree(0, 0.8f));
        output.pushedMessages.clear();

        // Palette basic, Slot one: Major/minor flip -> C minor triad (C3=48, Eb3=51, G3=55)
        REQUIRE(controller.beginTransform(interaction::TransformPalette::basic, interaction::TransformSlot::one));
        REQUIRE(controller.hasActiveTransform());
        auto transSpecOpt = controller.transformedSpecForActiveDegree();
        REQUIRE(transSpecOpt.has_value());
        REQUIRE(transSpecOpt->qualityRule == music::QualityRule::minor);

        // Held notes should now be 48, 51, 55
        REQUIRE(controller.getActiveChord()->notes == music::NoteSet({48, 51, 55}, 3));

        // Diff should be noteOff 52 (E), noteOn 51 (Eb)
        REQUIRE(output.pushedMessages.size() == 2);
        REQUIRE(output.pushedMessages[0].isNoteOff());
        REQUIRE(output.pushedMessages[0].getNoteNumber() == 52);
        REQUIRE(output.pushedMessages[1].isNoteOn());
        REQUIRE(output.pushedMessages[1].getNoteNumber() == 51);

        // Saved config in HarmonyConfiguration must NOT have mutated!
        REQUIRE(config.getSpec(0, 0).qualityRule == music::QualityRule::diatonic);
    }

    SECTION("Switching transform slots calculates from the saved base spec, not the previous transform") {
        // Press degree 0 (C major triad: 48, 52, 55)
        REQUIRE(controller.pressDegree(0, 0.8f));
        output.pushedMessages.clear();

        // 1. Transform to Minor (basic slot 1: 48, 51, 55)
        REQUIRE(controller.beginTransform(interaction::TransformPalette::basic, interaction::TransformSlot::one));
        output.pushedMessages.clear();

        // 2. Switch to Dominant 7 (basic slot 2: C7 -> C3=48, E3=52, G3=55, Bb3=58)
        REQUIRE(controller.beginTransform(interaction::TransformPalette::basic, interaction::TransformSlot::two));
        REQUIRE(controller.getActiveChord()->notes == music::NoteSet({48, 52, 55, 58}, 4));

        // Previous sounding notes were {48, 51, 55}. New notes are {48, 52, 55, 58}.
        // Diff: NoteOff 51 (Eb), NoteOn 52 (E), NoteOn 58 (Bb)
        REQUIRE(output.pushedMessages.size() == 3);
        REQUIRE(output.pushedMessages[0].isNoteOff());
        REQUIRE(output.pushedMessages[0].getNoteNumber() == 51);
        REQUIRE(output.pushedMessages[1].isNoteOn());
        REQUIRE(output.pushedMessages[1].getNoteNumber() == 52);
        REQUIRE(output.pushedMessages[2].isNoteOn());
        REQUIRE(output.pushedMessages[2].getNoteNumber() == 58);
    }

    SECTION("endTransform restores the exact saved base spec with differential events") {
        REQUIRE(controller.pressDegree(0, 0.8f)); // 48, 52, 55
        REQUIRE(controller.beginTransform(interaction::TransformPalette::basic, interaction::TransformSlot::one)); // 48, 51, 55
        output.pushedMessages.clear();

        controller.endTransform();
        REQUIRE_FALSE(controller.hasActiveTransform());
        REQUIRE(controller.getActiveChord()->notes == music::NoteSet({48, 52, 55}, 3));

        // NoteOff 51 (Eb), NoteOn 52 (E)
        REQUIRE(output.pushedMessages.size() == 2);
        REQUIRE(output.pushedMessages[0].isNoteOff());
        REQUIRE(output.pushedMessages[0].getNoteNumber() == 51);
        REQUIRE(output.pushedMessages[1].isNoteOn());
        REQUIRE(output.pushedMessages[1].getNoteNumber() == 52);
    }

    SECTION("commitActiveTransform writes transformed spec to targetConfig and clears temporary state without reverting audio") {
        REQUIRE(controller.pressDegree(0, 0.8f)); // 48, 52, 55
        REQUIRE(controller.beginTransform(interaction::TransformPalette::basic, interaction::TransformSlot::one)); // 48, 51, 55
        output.pushedMessages.clear();

        // Target config to commit to
        music::HarmonyConfiguration targetConfig = config;
        REQUIRE(controller.commitActiveTransform(targetConfig));

        // Transform is now cleared
        REQUIRE_FALSE(controller.hasActiveTransform());
        // Audio is unchanged (no new MIDI messages pushed during commit)
        REQUIRE(output.pushedMessages.empty());
        REQUIRE(controller.getActiveChord()->notes == music::NoteSet({48, 51, 55}, 3));

        // targetConfig has been updated with the minor qualityRule
        auto savedSpec = targetConfig.getSpec(0, 0);
        REQUIRE(savedSpec.qualityRule == music::QualityRule::minor);

        // Calling commitActiveTransform when no transform is active returns false
        REQUIRE_FALSE(controller.commitActiveTransform(targetConfig));
    }

    SECTION("commitActiveTransform returns false when no chord is active") {
        music::HarmonyConfiguration targetConfig;
        controller.beginTransform(interaction::TransformPalette::basic, interaction::TransformSlot::one);
        REQUIRE_FALSE(controller.commitActiveTransform(targetConfig));
    }

    SECTION("releaseActiveChord clears transform and turns off sounding transformed notes") {
        REQUIRE(controller.pressDegree(0, 0.8f));
        REQUIRE(controller.beginTransform(interaction::TransformPalette::basic, interaction::TransformSlot::one)); // 48, 51, 55
        output.pushedMessages.clear();

        controller.releaseActiveChord();
        REQUIRE_FALSE(controller.getActiveChord().has_value());
        REQUIRE_FALSE(controller.hasActiveTransform());

        // Note-offs sent for currently sounding 48, 51, 55
        REQUIRE(output.pushedMessages.size() == 3);
        REQUIRE(output.pushedMessages[0].isNoteOff());
        REQUIRE(output.pushedMessages[0].getNoteNumber() == 48);
        REQUIRE(output.pushedMessages[1].isNoteOff());
        REQUIRE(output.pushedMessages[1].getNoteNumber() == 51);
        REQUIRE(output.pushedMessages[2].isNoteOff());
        REQUIRE(output.pushedMessages[2].getNoteNumber() == 55);
    }

    SECTION("allNotesOff clears transform state and releases notes") {
        REQUIRE(controller.pressDegree(0, 0.8f));
        REQUIRE(controller.beginTransform(interaction::TransformPalette::basic, interaction::TransformSlot::one));
        REQUIRE(controller.hasActiveTransform());

        controller.allNotesOff();
        REQUIRE_FALSE(controller.hasActiveTransform());
        REQUIRE_FALSE(controller.getActiveChord().has_value());
    }

    SECTION("A held transform applies when its chord is pressed afterwards and survives a degree change") {
        // A performer may hold a color key before pressing any chord key.
        REQUIRE(controller.beginTransform(interaction::TransformPalette::basic, interaction::TransformSlot::one));
        REQUIRE(controller.hasActiveTransform());
        REQUIRE_FALSE(controller.getActiveChord().has_value());

        REQUIRE(controller.pressDegree(0, 0.8f));
        REQUIRE(controller.getActiveChord()->notes == music::NoteSet({48, 51, 55}, 3)); // C minor
        REQUIRE(controller.hasActiveTransform());

        output.pushedMessages.clear();
        // Keeping the color key held must apply it to the next degree too.
        REQUIRE(controller.pressDegree(1, 0.8f));
        REQUIRE(controller.getActiveChord()->degree == 1);
        REQUIRE(controller.getActiveChord()->notes == music::NoteSet({50, 54, 57}, 3)); // D major after Flip (M/m)
        REQUIRE(controller.hasActiveTransform());
        REQUIRE(output.pushedMessages.size() == 6);
    }

    SECTION("Scene, tonic, or scale change ends active transform safely") {
        REQUIRE(controller.pressDegree(0, 0.8f));
        REQUIRE(controller.beginTransform(interaction::TransformPalette::basic, interaction::TransformSlot::one));
        REQUIRE(controller.hasActiveTransform());

        // Tonic change
        controller.setTonic(2);
        REQUIRE_FALSE(controller.hasActiveTransform());

        // Begin transform again
        REQUIRE(controller.beginTransform(interaction::TransformPalette::basic, interaction::TransformSlot::one));
        REQUIRE(controller.hasActiveTransform());

        // Scale change
        controller.setScale(music::Scale::naturalMinor);
        REQUIRE_FALSE(controller.hasActiveTransform());

        // Begin transform again
        REQUIRE(controller.beginTransform(interaction::TransformPalette::basic, interaction::TransformSlot::one));
        REQUIRE(controller.hasActiveTransform());

        // Scene change
        controller.setScene(1);
        REQUIRE_FALSE(controller.hasActiveTransform());
    }

    SECTION("Queue failure on beginTransform preserves previously sounding state") {
        REQUIRE(controller.pressDegree(0, 0.8f)); // 48, 52, 55
        output.pushedMessages.clear();

        output.failPushes = true;
        REQUIRE_FALSE(controller.beginTransform(interaction::TransformPalette::basic, interaction::TransformSlot::one));

        // State should still be the base chord notes and no active transform
        REQUIRE(controller.getActiveChord()->notes == music::NoteSet({48, 52, 55}, 3));
        REQUIRE_FALSE(controller.hasActiveTransform());
    }
}
