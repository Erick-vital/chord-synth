#include <catch2/catch_test_macros.hpp>
#include "plugin/PluginProcessor.h"
#include "parameters/ParameterIds.h"
#include <cmath>
#include <limits>
#include <vector>

using namespace chordsynth;

namespace {

std::vector<float> renderWaveformRaw(float rawWaveform) {
    ChordSynthAudioProcessor processor;
    processor.prepareToPlay(48000.0, 256);
    auto* rawParameter = processor.getAPVTS().getRawParameterValue(parameters::ids::waveform);
    REQUIRE(rawParameter != nullptr);
    rawParameter->store(rawWaveform, std::memory_order_relaxed);

    juce::AudioBuffer<float> buffer(2, 256);
    buffer.clear();
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.0f), 0);
    processor.processBlock(buffer, midi);
    return {buffer.getReadPointer(0), buffer.getReadPointer(0) + buffer.getNumSamples()};
}

void requireFinite(const std::vector<float>& samples) {
    for (const auto sample : samples)
        REQUIRE(std::isfinite(sample));
}

} // namespace

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

TEST_CASE("APVTS waveform automation reaches active voices and oscillator rendering", "[plugin][processor][waveform]") {
    ChordSynthAudioProcessor testProcessor;
    ChordSynthAudioProcessor sineControlProcessor;
    testProcessor.prepareToPlay(48000.0, 256);
    sineControlProcessor.prepareToPlay(48000.0, 256);
    auto* testParameter = dynamic_cast<juce::AudioParameterChoice*>(
        testProcessor.getAPVTS().getParameter(parameters::ids::waveform));
    auto* controlParameter = dynamic_cast<juce::AudioParameterChoice*>(
        sineControlProcessor.getAPVTS().getParameter(parameters::ids::waveform));
    REQUIRE(testParameter != nullptr);
    REQUIRE(controlParameter != nullptr);

    juce::AudioBuffer<float> testBuffer(2, 256);
    juce::AudioBuffer<float> controlBuffer(2, 256);
    juce::MidiBuffer testNoteOn;
    juce::MidiBuffer controlNoteOn;
    testNoteOn.addEvent(juce::MidiMessage::noteOn(1, 64, 0.8f), 0);
    controlNoteOn.addEvent(juce::MidiMessage::noteOn(1, 64, 0.8f), 0);
    testProcessor.processBlock(testBuffer, testNoteOn);
    sineControlProcessor.processBlock(controlBuffer, controlNoteOn);

    for (int selection = 0; selection < 4; ++selection) {
        *testParameter = selection;
        *controlParameter = 0;
        testBuffer.clear();
        controlBuffer.clear();
        juce::MidiBuffer testNoMidi;
        juce::MidiBuffer controlNoMidi;
        testProcessor.processBlock(testBuffer, testNoMidi);
        sineControlProcessor.processBlock(controlBuffer, controlNoMidi);

        const std::vector<float> testSamples{
            testBuffer.getReadPointer(0), testBuffer.getReadPointer(0) + testBuffer.getNumSamples()};
        const std::vector<float> controlSamples{
            controlBuffer.getReadPointer(0), controlBuffer.getReadPointer(0) + controlBuffer.getNumSamples()};
        requireFinite(testSamples);
        requireFinite(controlSamples);

        if (selection == 0)
            REQUIRE(testSamples == controlSamples);
        else
            REQUIRE(testSamples != controlSamples);
    }
}

TEST_CASE("Raw waveform automation is clamped and rounded to deterministic oscillator choices",
          "[plugin][processor][waveform][regression]") {
    const auto sine = renderWaveformRaw(0.0f);
    const auto saw = renderWaveformRaw(1.0f);
    const auto square = renderWaveformRaw(2.0f);
    const auto triangle = renderWaveformRaw(3.0f);

    requireFinite(sine);
    requireFinite(saw);
    requireFinite(square);
    requireFinite(triangle);
    REQUIRE(renderWaveformRaw(-100.0f) == sine);
    REQUIRE(renderWaveformRaw(100.0f) == triangle);
    REQUIRE(renderWaveformRaw(std::numeric_limits<float>::quiet_NaN()) == sine);
    REQUIRE(renderWaveformRaw(std::numeric_limits<float>::infinity()) == sine);

    // Raw host values use round-half-up after clamping to the four choices.
    REQUIRE(renderWaveformRaw(0.49f) == sine);
    REQUIRE(renderWaveformRaw(0.5f) == saw);
    REQUIRE(renderWaveformRaw(2.49f) == square);
    REQUIRE(renderWaveformRaw(2.5f) == triangle);
}
