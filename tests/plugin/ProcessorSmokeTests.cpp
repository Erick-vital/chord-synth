#include <catch2/catch_test_macros.hpp>
#include "plugin/PluginProcessor.h"
#include <cmath>

using namespace chordsynth;

TEST_CASE("ChordSynthAudioProcessor renders 16-voice polyphonic audio from MIDI events", "[plugin][processor]") {
    ChordSynthAudioProcessor processor;

    REQUIRE(processor.acceptsMidi());
    REQUIRE_FALSE(processor.producesMidi());

    SECTION("A MIDI note-on event produces non-zero audio output") {
        processor.prepareToPlay(48000.0, 512);

        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();

        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);

        processor.processBlock(buffer, midi);

        REQUIRE(buffer.getMagnitude(0, 0, 512) > 0.01f);
        REQUIRE(buffer.getMagnitude(1, 0, 512) > 0.01f);
    }

    SECTION("A MIDI triad produces polyphonic audio without invalid or NaN samples") {
        processor.prepareToPlay(44100.0, 256);

        juce::AudioBuffer<float> buffer(2, 256);
        buffer.clear();

        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.7f), 0); // C4
        midi.addEvent(juce::MidiMessage::noteOn(1, 64, 0.7f), 0); // E4
        midi.addEvent(juce::MidiMessage::noteOn(1, 67, 0.7f), 0); // G4

        processor.processBlock(buffer, midi);

        for (int ch = 0; ch < 2; ++ch) {
            const float* channelData = buffer.getReadPointer(ch);
            for (int i = 0; i < 256; ++i) {
                REQUIRE_FALSE(std::isnan(channelData[i]));
                REQUIRE_FALSE(std::isinf(channelData[i]));
            }
        }
        REQUIRE(buffer.getMagnitude(0, 0, 256) > 0.02f);
    }

    SECTION("Note-off causes audio to decay back to silence") {
        processor.prepareToPlay(44100.0, 512);

        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        juce::MidiBuffer midiOn;
        midiOn.addEvent(juce::MidiMessage::noteOn(1, 60, 1.0f), 0);
        processor.processBlock(buffer, midiOn);

        // Send note off
        juce::MidiBuffer midiOff;
        midiOff.addEvent(juce::MidiMessage::noteOff(1, 60, 0.0f), 0);
        processor.processBlock(buffer, midiOff);

        // Process release blocks (~120ms release is ~5300 samples)
        juce::MidiBuffer emptyMidi;
        for (int i = 0; i < 15; ++i) {
            buffer.clear();
            processor.processBlock(buffer, emptyMidi);
        }

        buffer.clear();
        processor.processBlock(buffer, emptyMidi);
        REQUIRE(buffer.getMagnitude(0, 0, 512) == 0.0f);
    }

    SECTION("Sixteen simultaneous notes play without crashing or invalidating output") {
        processor.prepareToPlay(48000.0, 256);

        juce::AudioBuffer<float> buffer(2, 256);
        buffer.clear();

        juce::MidiBuffer midi;
        for (int note = 48; note < 48 + 16; ++note) {
            midi.addEvent(juce::MidiMessage::noteOn(1, note, 0.5f), 0);
        }

        REQUIRE_NOTHROW(processor.processBlock(buffer, midi));
        REQUIRE(buffer.getMagnitude(0, 0, 256) > 0.05f);
    }

    SECTION("Seventeen simultaneous notes execute deterministic voice stealing without crash or NaNs") {
        processor.prepareToPlay(48000.0, 256);

        juce::AudioBuffer<float> buffer(2, 256);
        buffer.clear();

        juce::MidiBuffer midi;
        for (int note = 40; note < 40 + 17; ++note) {
            midi.addEvent(juce::MidiMessage::noteOn(1, note, 0.5f), 0);
        }

        REQUIRE_NOTHROW(processor.processBlock(buffer, midi));
        REQUIRE(buffer.getMagnitude(0, 0, 256) > 0.05f);

        for (int ch = 0; ch < 2; ++ch) {
            const float* channelData = buffer.getReadPointer(ch);
            for (int i = 0; i < 256; ++i) {
                REQUIRE_FALSE(std::isnan(channelData[i]));
                REQUIRE_FALSE(std::isinf(channelData[i]));
            }
        }
    }
}
