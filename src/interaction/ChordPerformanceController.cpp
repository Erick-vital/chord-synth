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
    auto spec = config.getSpec(targetScene, degree);
    if (diatonicMode) {
        spec.qualityRule = music::QualityRule::diatonic;
    }
    const auto voiced = voicer.voiceChord(tonic, degree, spec, scale);
    const auto& newNoteSet = voiced.notes;
    const auto& oldNoteSet = activeChord->notes;
    const auto newBassMidi = voiced.bassMidi;
    const auto oldBassMidi = activeChord->bassMidi;

    if (newNoteSet == oldNoteSet && newBassMidi == oldBassMidi) {
        return;
    }

    // Determine removed notes and added notes
    // Capacity for notes is at most 6 harmonic + 1 bass = 7, so max 7 off + 7 on = 14 messages
    constexpr std::size_t maxSoundingNotes = music::maxChordTones + 1; // optional bass
    constexpr std::size_t maxReplacementEvents = maxSoundingNotes * 2;
    std::array<juce::MidiMessage, maxReplacementEvents> batchMessages{};
    size_t batchCount = 0;

    // 1. Offs for removed harmonic notes
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
            batchMessages[batchCount++] = juce::MidiMessage::noteOff(activeChord->midiChannel, oldNote, 0.0f);
        }
    }

    // 2. Off for old bass note if changed or removed (channel 2)
    if (oldBassMidi.has_value() && (!newBassMidi.has_value() || *oldBassMidi != *newBassMidi)) {
        batchMessages[batchCount++] = juce::MidiMessage::noteOff(2, *oldBassMidi, 0.0f);
    }

    // 3. Ons for added harmonic notes
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
            batchMessages[batchCount++] = juce::MidiMessage::noteOn(activeChord->midiChannel, newNote, activeChord->velocity);
        }
    }

    // 4. On for new bass note if added or changed (channel 2)
    if (newBassMidi.has_value() && (!oldBassMidi.has_value() || *oldBassMidi != *newBassMidi)) {
        batchMessages[batchCount++] = juce::MidiMessage::noteOn(2, *newBassMidi, activeChord->velocity);
    }

    if (batchCount > 0) {
        std::span<const juce::MidiMessage> batch(batchMessages.data(), batchCount);
        if (output.tryPushBatch(batch)) {
            activeChord->notes = newNoteSet;
            activeChord->bassMidi = newBassMidi;
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

    auto spec = config.getSpec(currentScene, degree);
    if (diatonicMode) {
        spec.qualityRule = music::QualityRule::diatonic;
    }
    const auto voiced = voicer.voiceChord(tonic, degree, spec, scale);
    const auto& newNotes = voiced.notes;

    if (newNotes.empty()) {
        return false;
    }

    // Prepare batch: if a chord was active, release it first, then start new notes
    // Max 7 note-offs + max 7 note-ons = 14 messages
    constexpr std::size_t maxSoundingNotes = music::maxChordTones + 1; // optional bass
    constexpr std::size_t maxReplacementEvents = maxSoundingNotes * 2;
    std::array<juce::MidiMessage, maxReplacementEvents> batchMessages{};
    size_t batchCount = 0;

    if (activeChord.has_value()) {
        for (int i = 0; i < activeChord->notes.size(); ++i) {
            batchMessages[batchCount++] = juce::MidiMessage::noteOff(
                activeChord->midiChannel,
                activeChord->notes[static_cast<size_t>(i)],
                0.0f);
        }
        if (activeChord->bassMidi.has_value()) {
            batchMessages[batchCount++] = juce::MidiMessage::noteOff(
                2,
                *activeChord->bassMidi,
                0.0f);
        }
    }

    for (int i = 0; i < newNotes.size(); ++i) {
        batchMessages[batchCount++] = juce::MidiMessage::noteOn(
            midiChannel,
            newNotes[static_cast<size_t>(i)],
            velocity);
    }
    if (voiced.bassMidi.has_value()) {
        batchMessages[batchCount++] = juce::MidiMessage::noteOn(
            2,
            *voiced.bassMidi,
            velocity);
    }

    std::span<const juce::MidiMessage> batch(batchMessages.data(), batchCount);
    if (!output.tryPushBatch(batch)) {
        return false;
    }

    activeChord = ActiveChord{
        .degree = degree,
        .notes = newNotes,
        .bassMidi = voiced.bassMidi,
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
    const auto bassMidi = activeChord->bassMidi;

    constexpr std::size_t maxSoundingNotes = music::maxChordTones + 1;
    std::array<juce::MidiMessage, maxSoundingNotes> batchMessages{};
    size_t batchCount = 0;

    for (int i = 0; i < notes.size(); ++i) {
        batchMessages[batchCount++] = juce::MidiMessage::noteOff(
            activeChord->midiChannel,
            notes[static_cast<size_t>(i)],
            0.0f);
    }

    if (bassMidi.has_value()) {
        batchMessages[batchCount++] = juce::MidiMessage::noteOff(
            2,
            *bassMidi,
            0.0f);
    }

    if (batchCount > 0) {
        std::span<const juce::MidiMessage> batch(batchMessages.data(), batchCount);
        output.tryPushBatch(batch);
    }

    activeChord.reset();
}

void ChordPerformanceController::allNotesOff() noexcept {
    releaseActiveChord();
}

} // namespace chordsynth::interaction
