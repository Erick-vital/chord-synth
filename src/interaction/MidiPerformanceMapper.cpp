#include "interaction/MidiPerformanceMapper.h"
#include "interaction/PerformanceVoicing.h"
#include <algorithm>
#include <limits>

namespace chordsynth::interaction {

MidiPerformanceMapper::MidiPerformanceMapper(
    const music::HarmonyConfiguration& harmonyConfig,
    const music::DiatonicChordVoicer& chordVoicer)
    : config(harmonyConfig), voicer(chordVoicer) {
    scratchBuffer.ensureSize(realtimeMidiBufferBytes);
}

void MidiPerformanceMapper::setContext(const Context& newContext) noexcept {
    const int normalizedTonic = ((newContext.tonic % 12) + 12) % 12;
    const bool tonicChanged = (context.tonic != normalizedTonic);
    const bool scaleChanged = (context.scale != newContext.scale);
    const bool diatonicChanged = (context.diatonicMode != newContext.diatonicMode);
    const bool paletteChanged = (context.palette != newContext.palette);

    context = newContext;
    context.tonic = normalizedTonic;

    if (tonicChanged || scaleChanged || diatonicChanged || paletteChanged) {
        // If context changes out-of-band while playing, we reset transform state
        activeTransformSlot.reset();
    }
}

void MidiPerformanceMapper::reset() noexcept {
    activeDegree.reset();
    activeTransformSlot.reset();
    activeHarmonicNotes = music::NoteSet{};
    activeBassMidi.reset();
    activeVelocity = 0.8f;
    heldDegreeCounts.fill(0);
    degreePressOrder.fill(0);
    degreeVelocities.fill(0.8f);
    nextPressOrder = 1;
    scratchBuffer.clear();
}

music::RealtimeVoicedChord MidiPerformanceMapper::computeCurrentVoicing(int degree) const noexcept {
    const auto transform = activeTransformSlot.has_value()
        ? std::optional<TransformSelection>{TransformSelection{.palette = context.palette, .slot = *activeTransformSlot}}
        : std::nullopt;
    const auto spec = resolvePerformanceVoicingSpec(
        config,
        {.scale = context.scale, .diatonicMode = context.diatonicMode},
        degree,
        transform);
    auto voiced = voicer.voiceChordRealtime(context.tonic, degree, spec, context.scale);
    if (spec.voiceLeading == music::VoiceLeadingMode::nearest && !activeHarmonicNotes.empty()) {
        voiced.notes = music::VoiceLeadingResolver::resolveNearestVoiceLeading(
            activeHarmonicNotes,
            voiced.notes);
    }
    return voiced;
}

void MidiPerformanceMapper::sendDifferentialVoicing(
    const music::RealtimeVoicedChord& newVoiced,
    float velocity,
    int sampleOffset,
    juce::MidiBuffer& outputMidi) noexcept {

    const auto& newNoteSet = newVoiced.notes;
    const auto& oldNoteSet = activeHarmonicNotes;
    const auto newBassMidi = newVoiced.bassMidi;
    const auto oldBassMidi = activeBassMidi;

    if (newNoteSet == oldNoteSet && newBassMidi == oldBassMidi) {
        return;
    }

    // 1. Offs for removed harmonic notes (channel 1)
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
            outputMidi.addEvent(juce::MidiMessage::noteOff(1, oldNote, 0.0f), sampleOffset);
        }
    }

    // 2. Off for old bass note if changed or removed (channel 2)
    if (oldBassMidi.has_value() && (!newBassMidi.has_value() || *oldBassMidi != *newBassMidi)) {
        outputMidi.addEvent(juce::MidiMessage::noteOff(2, *oldBassMidi, 0.0f), sampleOffset);
    }

    // 3. Ons for added harmonic notes (channel 1)
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
            outputMidi.addEvent(juce::MidiMessage::noteOn(1, newNote, velocity), sampleOffset);
        }
    }

    // 4. On for new bass note if added or changed (channel 2)
    if (newBassMidi.has_value() && (!oldBassMidi.has_value() || *oldBassMidi != *newBassMidi)) {
        outputMidi.addEvent(juce::MidiMessage::noteOn(2, *newBassMidi, velocity), sampleOffset);
    }

    activeHarmonicNotes = newNoteSet;
    activeBassMidi = newBassMidi;
}

void MidiPerformanceMapper::emitAllMappedNotesOff(int sampleOffset, juce::MidiBuffer& outputMidi) noexcept {
    for (int i = 0; i < activeHarmonicNotes.size(); ++i) {
        outputMidi.addEvent(juce::MidiMessage::noteOff(1, activeHarmonicNotes[static_cast<size_t>(i)], 0.0f), sampleOffset);
    }
    if (activeBassMidi.has_value()) {
        outputMidi.addEvent(juce::MidiMessage::noteOff(2, *activeBassMidi, 0.0f), sampleOffset);
    }

    activeHarmonicNotes = music::NoteSet{};
    activeBassMidi.reset();
    activeDegree.reset();
    activeTransformSlot.reset();
    heldDegreeCounts.fill(0);
    degreePressOrder.fill(0);
}

void MidiPerformanceMapper::handleDegreeNoteOn(
    int degree,
    float velocity,
    int sampleOffset,
    juce::MidiBuffer& outputMidi) noexcept {

    if (!music::HarmonyConfiguration::isValidDegree(degree)) {
        return;
    }

    const auto index = static_cast<std::size_t>(degree);
    auto& heldCount = heldDegreeCounts[index];
    if (heldCount < std::numeric_limits<std::uint8_t>::max()) {
        ++heldCount;
    }
    degreeVelocities[index] = velocity;
    if (heldCount == 1) {
        degreePressOrder[index] = nextPressOrder++;
    }
    if (activeDegree.has_value() && *activeDegree == degree) {
        return;
    }

    activeDegree = degree;
    activeVelocity = velocity;

    const auto voiced = computeCurrentVoicing(degree);
    sendDifferentialVoicing(voiced, velocity, sampleOffset, outputMidi);
}

void MidiPerformanceMapper::handleDegreeNoteOff(
    int degree,
    int sampleOffset,
    juce::MidiBuffer& outputMidi) noexcept {

    if (!music::HarmonyConfiguration::isValidDegree(degree)) {
        return;
    }

    auto& heldCount = heldDegreeCounts[static_cast<std::size_t>(degree)];
    if (heldCount == 0) {
        return;
    }
    --heldCount;
    if (heldCount > 0 || !activeDegree.has_value() || *activeDegree != degree) {
        return;
    }

    int fallbackDegree = -1;
    std::uint64_t newestOrder = 0;
    for (int candidate = 0; candidate < 7; ++candidate) {
        const auto index = static_cast<std::size_t>(candidate);
        if (heldDegreeCounts[index] > 0 && degreePressOrder[index] > newestOrder) {
            newestOrder = degreePressOrder[index];
            fallbackDegree = candidate;
        }
    }

    if (fallbackDegree >= 0) {
        activeDegree = fallbackDegree;
        activeVelocity = degreeVelocities[static_cast<std::size_t>(fallbackDegree)];
        const auto voiced = computeCurrentVoicing(fallbackDegree);
        sendDifferentialVoicing(voiced, activeVelocity, sampleOffset, outputMidi);
        return;
    }

    emitAllMappedNotesOff(sampleOffset, outputMidi);
}

void MidiPerformanceMapper::handleTransformCC(
    int ccNumber,
    int ccValue,
    int sampleOffset,
    juce::MidiBuffer& outputMidi) noexcept {

    const int slotIndex = ccNumber - minTransformCC;
    if (slotIndex < 0 || slotIndex > 7) {
        return;
    }

    const auto slot = static_cast<TransformSlot>(slotIndex);

    if (ccValue >= 64) {
        // Press / hold slot
        activeTransformSlot = slot;
        if (activeDegree.has_value()) {
            const auto voiced = computeCurrentVoicing(*activeDegree);
            sendDifferentialVoicing(voiced, activeVelocity, sampleOffset, outputMidi);
        }
    } else {
        // Release slot: only release if this is the active slot
        if (activeTransformSlot.has_value() && *activeTransformSlot == slot) {
            activeTransformSlot.reset();
            if (activeDegree.has_value()) {
                const auto voiced = computeCurrentVoicing(*activeDegree);
                sendDifferentialVoicing(voiced, activeVelocity, sampleOffset, outputMidi);
            }
        }
    }
}

void MidiPerformanceMapper::processBlock(
    const juce::MidiBuffer& inputMidi,
    juce::MidiBuffer& outputMidi,
    int /*numSamples*/) noexcept {

    outputMidi.clear();

    if (!isEnabled) {
        outputMidi.addEvents(inputMidi, 0, -1, 0);
        return;
    }

    for (const auto metadata : inputMidi) {
        const auto message = metadata.getMessage();
        const int sampleOffset = metadata.samplePosition;

        if (message.isAllNotesOff() || message.isAllSoundOff() ||
            (message.isController() && (message.getControllerNumber() == 120 || message.getControllerNumber() == 123))) {
            emitAllMappedNotesOff(sampleOffset, outputMidi);
            outputMidi.addEvent(message, sampleOffset);
            continue;
        }

        if (message.isNoteOn()) {
            const int noteNumber = message.getNoteNumber();
            if (noteNumber >= minDegreeNote && noteNumber <= maxDegreeNote) {
                const int degree = noteNumber - minDegreeNote;
                handleDegreeNoteOn(degree, message.getFloatVelocity(), sampleOffset, outputMidi);
                continue;
            }
        } else if (message.isNoteOff()) {
            const int noteNumber = message.getNoteNumber();
            if (noteNumber >= minDegreeNote && noteNumber <= maxDegreeNote) {
                const int degree = noteNumber - minDegreeNote;
                handleDegreeNoteOff(degree, sampleOffset, outputMidi);
                continue;
            }
        } else if (message.isController()) {
            const int ccNumber = message.getControllerNumber();
            if (ccNumber >= minTransformCC && ccNumber <= maxTransformCC) {
                handleTransformCC(ccNumber, message.getControllerValue(), sampleOffset, outputMidi);
                continue;
            }
        }

        // Pass-through any unhandled or non-mapped MIDI events unchanged
        outputMidi.addEvent(message, sampleOffset);
    }
}

void MidiPerformanceMapper::processBlock(juce::MidiBuffer& midiBuffer, int numSamples) noexcept {
    if (!isEnabled) {
        return;
    }

    scratchBuffer.clear();
    processBlock(midiBuffer, scratchBuffer, numSamples);
    midiBuffer.swapWith(scratchBuffer);
}

} // namespace chordsynth::interaction
