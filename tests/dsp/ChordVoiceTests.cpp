#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "dsp/ChordVoice.h"
#include "dsp/ChordSound.h"
#include <cmath>

using namespace chordsynth::dsp;

TEST_CASE("ChordVoice renders polyphonic note events with ADSR anti-click envelope", "[dsp][voice]") {
    juce::Synthesiser synth;
    synth.addSound(new ChordSound());
    synth.addVoice(new ChordVoice());

    SECTION("Note-on produces non-zero energy in the rendered buffer") {
        synth.setCurrentPlaybackSampleRate(44100.0);
        synth.noteOn(1, 60, 0.8f);

        juce::AudioBuffer<float> buffer(2, 256);
        buffer.clear();
        juce::MidiBuffer midiMessages;

        synth.renderNextBlock(buffer, midiMessages, 0, 256);

        float magnitude = buffer.getMagnitude(0, 0, 256);
        REQUIRE(magnitude > 0.01f);
        REQUIRE(synth.getNumVoices() > 0);
        REQUIRE(synth.getVoice(0)->isVoiceActive());
    }

    SECTION("Note-off decays cleanly to silence and frees the voice") {
        synth.setCurrentPlaybackSampleRate(44100.0);
        synth.noteOn(1, 69, 1.0f);

        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        juce::MidiBuffer midiMessages;
        synth.renderNextBlock(buffer, midiMessages, 0, 512);

        synth.noteOff(1, 69, 0.0f, true);

        // Render enough samples for release to complete (release is ~120ms = ~5300 samples at 44.1kHz)
        juce::AudioBuffer<float> releaseBuffer(2, 1024);
        for (int i = 0; i < 10; ++i) {
            releaseBuffer.clear();
            synth.renderNextBlock(releaseBuffer, midiMessages, 0, 1024);
        }

        // Voice should no longer be active and subsequent buffer should be silent
        REQUIRE_FALSE(synth.getVoice(0)->isVoiceActive());
        releaseBuffer.clear();
        synth.renderNextBlock(releaseBuffer, midiMessages, 0, 1024);
        REQUIRE(releaseBuffer.getMagnitude(0, 0, 1024) == 0.0f);
    }

    SECTION("Velocity scaling affects output amplitude proportionally") {
        juce::Synthesiser synthQuiet;
        synthQuiet.addSound(new ChordSound());
        synthQuiet.addVoice(new ChordVoice());
        synthQuiet.setCurrentPlaybackSampleRate(44100.0);
        synthQuiet.noteOn(1, 60, 0.2f);

        juce::Synthesiser synthLoud;
        synthLoud.addSound(new ChordSound());
        synthLoud.addVoice(new ChordVoice());
        synthLoud.setCurrentPlaybackSampleRate(44100.0);
        synthLoud.noteOn(1, 60, 1.0f);

        juce::AudioBuffer<float> bufferQuiet(2, 512);
        juce::AudioBuffer<float> bufferLoud(2, 512);
        bufferQuiet.clear();
        bufferLoud.clear();
        juce::MidiBuffer midiMessages;

        synthQuiet.renderNextBlock(bufferQuiet, midiMessages, 0, 512);
        synthLoud.renderNextBlock(bufferLoud, midiMessages, 0, 512);

        float magQuiet = bufferQuiet.getMagnitude(0, 0, 512);
        float magLoud = bufferLoud.getMagnitude(0, 0, 512);

        REQUIRE(magLoud > magQuiet * 2.0f);
    }

    SECTION("Output buffer remains strictly finite across varied sample rates and block sizes") {
        const std::array<double, 3> sampleRates{44100.0, 48000.0, 96000.0};
        const std::array<int, 6> blockSizes{1, 16, 64, 256, 512, 1024};

        for (double sr : sampleRates) {
            for (int bs : blockSizes) {
                juce::Synthesiser s;
                s.addSound(new ChordSound());
                s.addVoice(new ChordVoice());
                s.setCurrentPlaybackSampleRate(sr);
                s.noteOn(1, 64, 0.7f);

                juce::AudioBuffer<float> buf(2, bs);
                buf.clear();
                juce::MidiBuffer midi;
                s.renderNextBlock(buf, midi, 0, bs);

                for (int ch = 0; ch < 2; ++ch) {
                    const float* readPtr = buf.getReadPointer(ch);
                    for (int smp = 0; smp < bs; ++smp) {
                        REQUIRE_FALSE(std::isnan(readPtr[smp]));
                        REQUIRE_FALSE(std::isinf(readPtr[smp]));
                        REQUIRE(readPtr[smp] >= -1.0f);
                        REQUIRE(readPtr[smp] <= 1.0f);
                    }
                }
            }
        }
    }
}
