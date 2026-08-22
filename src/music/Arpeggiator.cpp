#include "Arpeggiator.h"

namespace chordsynth::music {

void Arpeggiator::prepare(double sampleRate) noexcept
{
    clock.prepare(sampleRate);
    reset();
}

void Arpeggiator::reset() noexcept
{
    sampleCounter = 0;
    patternIndex = 0;
    currentPlayingNote = -1;
    noteOffSampleRemaining = -1;
    heldNotes.clear();
    patternNotes.clear();
}

void Arpeggiator::setEnabled(bool isEnabled) noexcept
{
    if (enabled != isEnabled) {
        enabled = isEnabled;
        if (!enabled) {
            reset();
        }
    }
}

void Arpeggiator::setMode(ArpMode mode) noexcept
{
    if (currentMode != mode) {
        currentMode = mode;
        updatePattern();
    }
}

void Arpeggiator::setRate(ArpRate rate) noexcept
{
    clock.setRate(rate);
}

void Arpeggiator::setGate(float gateRatio) noexcept
{
    if (std::isfinite(gateRatio))
        gate = std::clamp(gateRatio, 0.1f, 1.0f);
    else
        gate = 0.8f;
}

void Arpeggiator::setSeed(unsigned int seed) noexcept
{
    rng.seed(seed);
}

void Arpeggiator::noteOn(int midiNoteNumber, float velocity) noexcept
{
    auto it = std::find_if(heldNotes.begin(), heldNotes.end(),
                           [midiNoteNumber](const HeldNote& n) { return n.noteNumber == midiNoteNumber; });
    if (it == heldNotes.end()) {
        heldNotes.push_back({midiNoteNumber, velocity});
        updatePattern();
    }
}

void Arpeggiator::noteOff(int midiNoteNumber) noexcept
{
    auto it = std::find_if(heldNotes.begin(), heldNotes.end(),
                           [midiNoteNumber](const HeldNote& n) { return n.noteNumber == midiNoteNumber; });
    if (it != heldNotes.end()) {
        heldNotes.erase(it);
        updatePattern();
    }
}

void Arpeggiator::allNotesOff() noexcept
{
    heldNotes.clear();
    patternNotes.clear();
    patternIndex = 0;
}

void Arpeggiator::updatePattern() noexcept
{
    if (heldNotes.empty()) {
        patternNotes.clear();
        patternIndex = 0;
        return;
    }

    std::vector<int> sorted;
    sorted.reserve(heldNotes.size());
    for (const auto& hn : heldNotes) {
        sorted.push_back(hn.noteNumber);
    }
    std::sort(sorted.begin(), sorted.end());

    patternNotes.clear();
    switch (currentMode) {
        case ArpMode::up:
            patternNotes = sorted;
            break;
        case ArpMode::down:
            patternNotes = sorted;
            std::reverse(patternNotes.begin(), patternNotes.end());
            break;
        case ArpMode::upDown:
            if (sorted.size() <= 2) {
                patternNotes = sorted;
            } else {
                patternNotes = sorted;
                // Add descending inner notes without duplicating the extremes
                for (size_t i = sorted.size() - 2; i > 0; --i) {
                    patternNotes.push_back(sorted[i]);
                }
            }
            break;
        case ArpMode::random:
            patternNotes = sorted;
            break;
    }

    if (patternIndex >= patternNotes.size())
        patternIndex = 0;
}

void Arpeggiator::processBlock(juce::MidiBuffer& midiMessages, int numSamples, double bpm) noexcept
{
    if (!enabled)
        return;

    const int stepSamples = clock.getSamplesPerStep(bpm);
    const int gateSamples = std::max(1, static_cast<int>(std::round(stepSamples * gate)));

    for (int sample = 0; sample < numSamples; ++sample) {
        if (noteOffSampleRemaining > 0) {
            --noteOffSampleRemaining;
            if (noteOffSampleRemaining == 0 && currentPlayingNote >= 0) {
                midiMessages.addEvent(juce::MidiMessage::noteOff(1, currentPlayingNote), sample);
                currentPlayingNote = -1;
            }
        }

        if (sampleCounter <= 0) {
            sampleCounter = stepSamples;
            if (!patternNotes.empty()) {
                // If there's an active note, send note-off right before new note-on
                if (currentPlayingNote >= 0) {
                    midiMessages.addEvent(juce::MidiMessage::noteOff(1, currentPlayingNote), sample);
                    currentPlayingNote = -1;
                }

                int noteToPlay = 60;
                if (currentMode == ArpMode::random) {
                    std::uniform_int_distribution<size_t> dist(0, patternNotes.size() - 1);
                    noteToPlay = patternNotes[dist(rng)];
                } else {
                    noteToPlay = patternNotes[patternIndex];
                    patternIndex = (patternIndex + 1) % patternNotes.size();
                }

                currentPlayingNote = noteToPlay;
                midiMessages.addEvent(juce::MidiMessage::noteOn(1, noteToPlay, (juce::uint8)100), sample);
                noteOffSampleRemaining = gateSamples;
            }
        }
        --sampleCounter;
    }
}

} // namespace chordsynth::music
