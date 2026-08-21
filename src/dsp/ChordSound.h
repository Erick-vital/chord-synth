#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace chordsynth::dsp {

class ChordSound : public juce::SynthesiserSound {
public:
    ChordSound() = default;

    bool appliesToNote(int /*midiNoteNumber*/) override { return true; }
    bool appliesToChannel(int /*midiChannel*/) override { return true; }
};

} // namespace chordsynth::dsp
