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

TEST_CASE("Detune automation on ChordSynthAudioProcessor reaches stereo voice render", "[plugin][detune]") {
    ChordSynthAudioProcessor processor;
    auto* detuneParam = dynamic_cast<juce::AudioParameterFloat*>(
        processor.getAPVTS().getParameter(parameters::ids::detune));
    REQUIRE(detuneParam != nullptr);

    *detuneParam = 12.0f;
    processor.prepareToPlay(48000.0, 256);

    juce::AudioBuffer<float> buffer(2, 256);
    buffer.clear();
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.9f), 0);
    processor.processBlock(buffer, midi);

    REQUIRE(buffer.getMagnitude(0, 0, 256) > 0.01f);
    REQUIRE(buffer.getMagnitude(1, 0, 256) > 0.01f);

    bool leftRightDiffer = false;
    for (int i = 50; i < 256; ++i) {
        if (std::abs(buffer.getSample(0, i) - buffer.getSample(1, i)) > 1e-4f) {
            leftRightDiffer = true;
            break;
        }
    }
    REQUIRE(leftRightDiffer);
}

void requireFinite(const std::vector<float>& samples) {
    for (const auto sample : samples)
        REQUIRE(std::isfinite(sample));
}

float renderSawEnergyWithCutoff(float cutoffHz) {
    ChordSynthAudioProcessor processor;
    auto* waveform = dynamic_cast<juce::AudioParameterChoice*>(
        processor.getAPVTS().getParameter(parameters::ids::waveform));
    auto* cutoff = dynamic_cast<juce::AudioParameterFloat*>(
        processor.getAPVTS().getParameter(parameters::ids::cutoff));
    auto* resonance = dynamic_cast<juce::AudioParameterFloat*>(
        processor.getAPVTS().getParameter(parameters::ids::resonance));
    REQUIRE(waveform != nullptr);
    REQUIRE(cutoff != nullptr);
    REQUIRE(resonance != nullptr);
    *waveform = 1;
    *cutoff = cutoffHz;
    *resonance = 0.2f;
    processor.prepareToPlay(48000.0, 256);

    juce::AudioBuffer<float> buffer(2, 256);
    double sumSquares = 0.0;
    int sampleCount = 0;
    for (int block = 0; block < 36; ++block) {
        buffer.clear();
        juce::MidiBuffer midi;
        if (block == 0)
            midi.addEvent(juce::MidiMessage::noteOn(1, 84, 0.8f), 0);
        processor.processBlock(buffer, midi);
        if (block >= 12) {
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
                const auto value = buffer.getSample(0, sample);
                REQUIRE(std::isfinite(value));
                sumSquares += static_cast<double>(value) * value;
                ++sampleCount;
            }
        }
    }
    return static_cast<float>(std::sqrt(sumSquares / sampleCount));
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
        // The IIR filter has a vanishing tail after the voice reaches exact silence.
        REQUIRE(buffer.getMagnitude(0, 0, 512) < 1.0e-6f);
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

TEST_CASE("Processor applies the low-pass globally after deterministic synth rendering",
          "[plugin][processor][filter]") {
    const auto lowCutoffEnergy = renderSawEnergyWithCutoff(300.0f);
    const auto highCutoffEnergy = renderSawEnergyWithCutoff(18000.0f);
    REQUIRE(lowCutoffEnergy > 0.0f);
    REQUIRE(highCutoffEnergy > lowCutoffEnergy * 3.0f);
}

TEST_CASE("Rapid filter parameter automation keeps active synth output finite",
          "[plugin][processor][filter][automation]") {
    ChordSynthAudioProcessor processor;
    processor.prepareToPlay(48000.0, 64);
    auto* cutoff = processor.getAPVTS().getRawParameterValue(parameters::ids::cutoff);
    auto* resonance = processor.getAPVTS().getRawParameterValue(parameters::ids::resonance);
    REQUIRE(cutoff != nullptr);
    REQUIRE(resonance != nullptr);
    juce::AudioBuffer<float> buffer(2, 64);
    for (int block = 0; block < 100; ++block) {
        cutoff->store(block % 3 == 0 ? 20.0f : (block % 3 == 1 ? 20000.0f
                                                                  : std::numeric_limits<float>::infinity()),
                      std::memory_order_relaxed);
        resonance->store(block % 2 == 0 ? -1000.0f : 1000.0f,
                         std::memory_order_relaxed);
        buffer.clear();
        juce::MidiBuffer midi;
        if (block == 0)
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);
        processor.processBlock(buffer, midi);
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                REQUIRE(std::isfinite(buffer.getSample(channel, sample)));
    }
}

TEST_CASE("APVTS chorus parameter automation reaches output rendering", "[plugin][processor][chorus]") {
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256;
    constexpr int totalBlocks = 20;

    ChordSynthAudioProcessor procBypass;
    procBypass.prepareToPlay(sampleRate, blockSize);

    ChordSynthAudioProcessor procActive;
    procActive.prepareToPlay(sampleRate, blockSize);
    auto* chorusMix = dynamic_cast<juce::AudioParameterFloat*>(
        procActive.getAPVTS().getParameter(parameters::ids::chorusMix));
    REQUIRE(chorusMix != nullptr);
    *chorusMix = 0.5f;

    // Send identical note on both processors
    auto noteOn = juce::MidiMessage::noteOn(1, 60, (juce::uint8)100);
    procBypass.getUiMidiQueue().push(noteOn);
    procActive.getUiMidiQueue().push(noteOn);

    juce::AudioBuffer<float> bufBypass(2, blockSize);
    juce::AudioBuffer<float> bufActive(2, blockSize);
    juce::MidiBuffer midiBypass;
    juce::MidiBuffer midiActive;

    double diffSum = 0.0;
    for (int block = 0; block < totalBlocks; ++block) {
        bufBypass.clear();
        bufActive.clear();
        midiBypass.clear();
        midiActive.clear();

        procBypass.processBlock(bufBypass, midiBypass);
        procActive.processBlock(bufActive, midiActive);

        if (block >= 5) {
            for (int i = 0; i < blockSize; ++i) {
                float d0 = bufBypass.getSample(0, i) - bufActive.getSample(0, i);
                float d1 = bufBypass.getSample(1, i) - bufActive.getSample(1, i);
                diffSum += std::abs(d0) + std::abs(d1);
            }
        }
    }

    REQUIRE(diffSum > 0.1);
}

TEST_CASE("APVTS delay parameter automation reaches output rendering", "[plugin][processor][delay]") {
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256;
    constexpr int totalBlocks = 30;

    ChordSynthAudioProcessor procBypass;
    procBypass.prepareToPlay(sampleRate, blockSize);

    ChordSynthAudioProcessor procActive;
    procActive.prepareToPlay(sampleRate, blockSize);
    auto* delayMix = dynamic_cast<juce::AudioParameterFloat*>(
        procActive.getAPVTS().getParameter(parameters::ids::delayMix));
    auto* delayFeedback = dynamic_cast<juce::AudioParameterFloat*>(
        procActive.getAPVTS().getParameter(parameters::ids::delayFeedback));
    auto* delayTimeMs = dynamic_cast<juce::AudioParameterFloat*>(
        procActive.getAPVTS().getParameter(parameters::ids::delayTimeMs));
    auto* delaySync = dynamic_cast<juce::AudioParameterBool*>(
        procActive.getAPVTS().getParameter(parameters::ids::delaySync));
    REQUIRE(delayMix != nullptr);
    REQUIRE(delayFeedback != nullptr);
    REQUIRE(delayTimeMs != nullptr);
    REQUIRE(delaySync != nullptr);

    *delayMix = 0.5f;
    *delayFeedback = 0.4f;
    *delaySync = false;
    *delayTimeMs = 50.0f; // ~2400 samples delay (~9.3 blocks)

    // Send identical short note on both processors then release
    auto noteOn = juce::MidiMessage::noteOn(1, 60, (juce::uint8)100);
    procBypass.getUiMidiQueue().push(noteOn);
    procActive.getUiMidiQueue().push(noteOn);

    juce::AudioBuffer<float> bufBypass(2, blockSize);
    juce::AudioBuffer<float> bufActive(2, blockSize);
    juce::MidiBuffer midiBypass;
    juce::MidiBuffer midiActive;

    double diffSum = 0.0;
    for (int block = 0; block < totalBlocks; ++block) {
        bufBypass.clear();
        bufActive.clear();
        midiBypass.clear();
        midiActive.clear();

        if (block == 2) {
            auto noteOff = juce::MidiMessage::noteOff(1, 60);
            procBypass.getUiMidiQueue().push(noteOff);
            procActive.getUiMidiQueue().push(noteOff);
        }

        procBypass.processBlock(bufBypass, midiBypass);
        procActive.processBlock(bufActive, midiActive);

        if (block >= 10) {
            for (int i = 0; i < blockSize; ++i) {
                float d0 = bufBypass.getSample(0, i) - bufActive.getSample(0, i);
                float d1 = bufBypass.getSample(1, i) - bufActive.getSample(1, i);
                diffSum += std::abs(d0) + std::abs(d1);
            }
        }
    }

    REQUIRE(diffSum > 0.05);
}

TEST_CASE("APVTS reverb parameter automation reaches output rendering", "[plugin][processor][reverb]") {
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256;
    constexpr int totalBlocks = 30;

    ChordSynthAudioProcessor procBypass;
    procBypass.prepareToPlay(sampleRate, blockSize);

    ChordSynthAudioProcessor procActive;
    procActive.prepareToPlay(sampleRate, blockSize);
    auto* reverbMix = dynamic_cast<juce::AudioParameterFloat*>(
        procActive.getAPVTS().getParameter(parameters::ids::reverbMix));
    auto* reverbRoomSize = dynamic_cast<juce::AudioParameterFloat*>(
        procActive.getAPVTS().getParameter(parameters::ids::reverbRoomSize));
    REQUIRE(reverbMix != nullptr);
    REQUIRE(reverbRoomSize != nullptr);

    *reverbMix = 0.6f;
    *reverbRoomSize = 0.8f;

    // Send identical short note on both processors then release
    auto noteOn = juce::MidiMessage::noteOn(1, 60, (juce::uint8)100);
    procBypass.getUiMidiQueue().push(noteOn);
    procActive.getUiMidiQueue().push(noteOn);

    juce::AudioBuffer<float> bufBypass(2, blockSize);
    juce::AudioBuffer<float> bufActive(2, blockSize);
    juce::MidiBuffer midiBypass;
    juce::MidiBuffer midiActive;

    double diffSum = 0.0;
    for (int block = 0; block < totalBlocks; ++block) {
        bufBypass.clear();
        bufActive.clear();
        midiBypass.clear();
        midiActive.clear();

        if (block == 2) {
            auto noteOff = juce::MidiMessage::noteOff(1, 60);
            procBypass.getUiMidiQueue().push(noteOff);
            procActive.getUiMidiQueue().push(noteOff);
        }

        procBypass.processBlock(bufBypass, midiBypass);
        procActive.processBlock(bufActive, midiActive);

        if (block >= 10) {
            for (int i = 0; i < blockSize; ++i) {
                float d0 = bufBypass.getSample(0, i) - bufActive.getSample(0, i);
                float d1 = bufBypass.getSample(1, i) - bufActive.getSample(1, i);
                diffSum += std::abs(d0) + std::abs(d1);
            }
        }
    }

    REQUIRE(diffSum > 0.05);
}

TEST_CASE("APVTS arpeggiator parameter automation reaches output rendering", "[plugin][processor][arp]") {
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256;
    constexpr int totalBlocks = 30;

    ChordSynthAudioProcessor procDirect;
    procDirect.prepareToPlay(sampleRate, blockSize);

    ChordSynthAudioProcessor procArp;
    procArp.prepareToPlay(sampleRate, blockSize);
    auto* arpEnabled = dynamic_cast<juce::AudioParameterBool*>(
        procArp.getAPVTS().getParameter(parameters::ids::arpEnabled));
    auto* arpMode = dynamic_cast<juce::AudioParameterChoice*>(
        procArp.getAPVTS().getParameter(parameters::ids::arpMode));
    auto* arpRate = dynamic_cast<juce::AudioParameterChoice*>(
        procArp.getAPVTS().getParameter(parameters::ids::arpRate));
    REQUIRE(arpEnabled != nullptr);
    REQUIRE(arpMode != nullptr);
    REQUIRE(arpRate != nullptr);

    *arpEnabled = true;
    *arpMode = 0; // Up
    *arpRate = 2; // 1/16

    // Send a 3-note chord to both
    procDirect.getUiMidiQueue().push(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100));
    procDirect.getUiMidiQueue().push(juce::MidiMessage::noteOn(1, 64, (juce::uint8)100));
    procDirect.getUiMidiQueue().push(juce::MidiMessage::noteOn(1, 67, (juce::uint8)100));

    procArp.getUiMidiQueue().push(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100));
    procArp.getUiMidiQueue().push(juce::MidiMessage::noteOn(1, 64, (juce::uint8)100));
    procArp.getUiMidiQueue().push(juce::MidiMessage::noteOn(1, 67, (juce::uint8)100));

    juce::AudioBuffer<float> bufDirect(2, blockSize);
    juce::AudioBuffer<float> bufArp(2, blockSize);
    juce::MidiBuffer midiDirect;
    juce::MidiBuffer midiArp;

    double diffSum = 0.0;
    for (int block = 0; block < totalBlocks; ++block) {
        bufDirect.clear();
        bufArp.clear();
        midiDirect.clear();
        midiArp.clear();

        procDirect.processBlock(bufDirect, midiDirect);
        procArp.processBlock(bufArp, midiArp);

        for (int i = 0; i < blockSize; ++i) {
            float d0 = bufDirect.getSample(0, i) - bufArp.getSample(0, i);
            float d1 = bufDirect.getSample(1, i) - bufArp.getSample(1, i);
            diffSum += std::abs(d0) + std::abs(d1);
        }
    }

    REQUIRE(diffSum > 0.1);
}
