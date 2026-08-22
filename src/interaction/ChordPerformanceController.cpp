#include "interaction/ChordPerformanceController.h"
#include <algorithm>
#include <array>

namespace chordsynth::interaction {

ChordPerformanceController::ChordPerformanceController(
    const music::HarmonyConfiguration& harmonyConfig,
    const music::DiatonicChordVoicer& chordVoicer,
    MidiBatchOutput& midiOutput) noexcept
    : config(harmonyConfig), voicer(chordVoicer), output(midiOutput) {}

ChordPerformanceController::~ChordPerformanceController() {
    allNotesOff();
}

void ChordPerformanceController::setTonic(int newTonic) noexcept {
    tonic = ((newTonic % 12) + 12) % 12;
}

void ChordPerformanceController::setScene(int newSceneIndex) noexcept {
    if (!music::HarmonyConfiguration::isValidScene(newSceneIndex)) {
        return;
    }

    if (currentScene == newSceneIndex) {
        return;
    }

    if (liveRevoice && activeChord.has_value()) {
        applyLiveRevoicing(newSceneIndex);
    }

    currentScene = newSceneIndex;
}

void ChordPerformanceController::applyLiveRevoicing(int targetScene) noexcept {
    if (!activeChord.has_value()) {
        return;
    }

    const int degree = activeChord->degree;
    const auto spec = config.getSpec(targetScene, degree);
    const auto voiced = voicer.voiceChord(tonic, degree, spec);
    const auto& newNoteSet = voiced.notes;
    const auto& oldNoteSet = activeChord->notes;

    if (newNoteSet == oldNoteSet) {
        return;
    }

    // Determine removed notes and added notes
    // Capacity for notes is at most 4, so max 4 off + 4 on = 8 messages
    std::array<juce::MidiMessage, 8> batchMessages{};
    size_t batchCount = 0;

    // 1. Offs for removed notes
    for (int i = 0; i < oldNoteSet.size(); ++i) {
        const int oldNote = oldNoteSet[static_cast<size_t>(i)];
        bool stillPresent = false;
        for (int j = 0; j < newNoteSet.size(); ++j) {
            if (newNoteSet[static_cast<size_t>(j)] == oldNote) {
                stillPresent = true;
                break;
            }
        }
        if (!stillPresent) {
            batchMessages[batchCount++] = juce::MidiMessage::noteOff(midiChannel, oldNote, 0.0f);
        }
    }

    // 2. Ons for added notes
    for (int j = 0; j < newNoteSet.size(); ++j) {
        const int newNote = newNoteSet[static_cast<size_t>(j)];
        bool wasPresent = false;
        for (int i = 0; i < oldNoteSet.size(); ++i) {
            if (oldNoteSet[static_cast<size_t>(i)] == newNote) {
                wasPresent = true;
                break;
            }
        }
        if (!wasPresent) {
            batchMessages[batchCount++] = juce::MidiMessage::noteOn(midiChannel, newNote, activeChord->velocity);
        }
    }

    if (batchCount > 0) {
        std::span<const juce::MidiMessage> batch(batchMessages.data(), batchCount);
        if (output.tryPushBatch(batch)) {
            activeChord->notes = newNoteSet;
        }
    }
}

void ChordPerformanceController::revoiceActiveChordIfHeld(int degree) noexcept {
    if (!liveRevoice || !activeChord.has_value() || activeChord->degree != degree) {
        return;
    }

    applyLiveRevoicing(currentScene);
}

bool ChordPerformanceController::pressDegree(int degree, float velocity) noexcept {
    if (!music::HarmonyConfiguration::isValidDegree(degree)) {
        return false;
    }

    // If the same degree is already active, ignore repeated press
    if (activeChord.has_value() && activeChord->degree == degree) {
        return true;
    }

    const auto spec = config.getSpec(currentScene, degree);
    const auto voiced = voicer.voiceChord(tonic, degree, spec);
    const auto& newNotes = voiced.notes;

    if (newNotes.empty()) {
        return false;
    }

    // Prepare batch: if a chord was active, release it first, then start new notes
    // Max 4 note-offs + max 4 note-ons = 8 messages
    std::array<juce::MidiMessage, 8> batchMessages{};
    size_t batchCount = 0;

    if (activeChord.has_value()) {
        for (int i = 0; i < activeChord->notes.size(); ++i) {
            batchMessages[batchCount++] = juce::MidiMessage::noteOff(
                activeChord->midiChannel,
                activeChord->notes[static_cast<size_t>(i)],
                0.0f);
        }
    }

    for (int i = 0; i < newNotes.size(); ++i) {
        batchMessages[batchCount++] = juce::MidiMessage::noteOn(
            midiChannel,
            newNotes[static_cast<size_t>(i)],
            velocity);
    }

    std::span<const juce::MidiMessage> batch(batchMessages.data(), batchCount);
    if (!output.tryPushBatch(batch)) {
        return false;
    }

    activeChord = ActiveChord{
        .degree = degree,
        .notes = newNotes,
        .velocity = velocity,
        .midiChannel = midiChannel
    };

    return true;
}

void ChordPerformanceController::releaseActiveChord() noexcept {
    if (!activeChord.has_value()) {
        return;
    }

    const auto& notes = activeChord->notes;
    if (!notes.empty()) {
        std::array<juce::MidiMessage, 4> batchMessages{};
        for (int i = 0; i < notes.size(); ++i) {
            batchMessages[static_cast<size_t>(i)] = juce::MidiMessage::noteOff(
                activeChord->midiChannel,
                notes[static_cast<size_t>(i)],
                0.0f);
        }
        std::span<const juce::MidiMessage> batch(batchMessages.data(), static_cast<size_t>(notes.size()));
        output.tryPushBatch(batch);
    }

    activeChord.reset();
}

void ChordPerformanceController::allNotesOff() noexcept {
    releaseActiveChord();
}

} // namespace chordsynth::interaction
