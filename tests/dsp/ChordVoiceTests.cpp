#include <catch2/catch_approx.hpp>
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

    SECTION("Detune parameter creates stereo width with energy in both channels") {
        juce::Synthesiser sZero;
        sZero.addSound(new ChordSound());
        sZero.addVoice(new ChordVoice());
        sZero.setCurrentPlaybackSampleRate(44100.0);
        auto* vZero = dynamic_cast<ChordVoice*>(sZero.getVoice(0));
        REQUIRE(vZero != nullptr);
        vZero->setDetuneCents(0.0f);
        sZero.noteOn(1, 60, 0.8f);

        juce::AudioBuffer<float> bufZero(2, 512);
        bufZero.clear();
        juce::MidiBuffer midi;
        sZero.renderNextBlock(bufZero, midi, 0, 512);

        // At 0 cents detune, left and right channels should be identical (centered)
        for (int i = 0; i < 512; ++i) {
            REQUIRE(bufZero.getSample(0, i) == Catch::Approx(bufZero.getSample(1, i)).margin(1e-5f));
        }

        juce::Synthesiser sDetuned;
        sDetuned.addSound(new ChordSound());
        sDetuned.addVoice(new ChordVoice());
        sDetuned.setCurrentPlaybackSampleRate(44100.0);
        auto* vDetuned = dynamic_cast<ChordVoice*>(sDetuned.getVoice(0));
        REQUIRE(vDetuned != nullptr);
        vDetuned->setDetuneCents(10.0f);
        sDetuned.noteOn(1, 60, 0.8f);

        juce::AudioBuffer<float> bufDetuned(2, 512);
        bufDetuned.clear();
        sDetuned.renderNextBlock(bufDetuned, midi, 0, 512);

        float leftEnergy = bufDetuned.getMagnitude(0, 0, 512);
        float rightEnergy = bufDetuned.getMagnitude(1, 0, 512);
        REQUIRE(leftEnergy > 0.01f);
        REQUIRE(rightEnergy > 0.01f);

        // With detune > 0, left and right channels should differ over time
        bool differenceDetected = false;
        for (int i = 50; i < 512; ++i) {
            if (std::abs(bufDetuned.getSample(0, i) - bufDetuned.getSample(1, i)) > 1e-3f) {
                differenceDetected = true;
                break;
            }
        }
        REQUIRE(differenceDetected);

        // Mono sum (L + R) should not cancel out (mono compatibility)
        juce::AudioBuffer<float> monoSum(1, 512);
        monoSum.clear();
        monoSum.addFrom(0, 0, bufDetuned, 0, 0, 512);
        monoSum.addFrom(0, 0, bufDetuned, 1, 0, 512);
        REQUIRE(monoSum.getMagnitude(0, 0, 512) > 0.01f);
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
