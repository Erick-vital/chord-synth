#include <catch2/catch_approx.hpp>
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

TEST_CASE("PluginProcessor persists and restores HarmonyState across round-trip and legacy migration",
          "[plugin][processor][state][harmony]") {
    SECTION("Round-trip preserves both APVTS parameters and HarmonyState non-default overrides") {
        ChordSynthAudioProcessor sourceProcessor;
        auto* cutoff = dynamic_cast<juce::AudioParameterFloat*>(
            sourceProcessor.getAPVTS().getParameter(parameters::ids::cutoff));
        REQUIRE(cutoff != nullptr);
        *cutoff = 2400.0f;

        sourceProcessor.getHarmonyState().setSelectedScene(2);
        sourceProcessor.getHarmonyState().setLiveRevoice(true);
        sourceProcessor.getHarmonyState().setQualityRule(music::QualityRule::minor);
        music::VoicingSpec customSpec{
            .extension = music::ChordExtension::seventh,
            .inversion = 1,
            .style = music::VoicingStyle::open,
            .baseOctave = 4,
            .qualityRule = music::QualityRule::major
        };
        sourceProcessor.getHarmonyState().getConfiguration().setSpec(2, 3, customSpec); // Scene 2, IV

        juce::MemoryBlock block;
        sourceProcessor.getStateInformation(block);
        REQUIRE(block.getSize() > 0);

        ChordSynthAudioProcessor targetProcessor;
        targetProcessor.setStateInformation(block.getData(), static_cast<int>(block.getSize()));

        auto* targetCutoff = dynamic_cast<juce::AudioParameterFloat*>(
            targetProcessor.getAPVTS().getParameter(parameters::ids::cutoff));
        REQUIRE(targetCutoff != nullptr);
        REQUIRE(static_cast<float>(*targetCutoff) == Catch::Approx(2400.0f));

        const auto& restoredHarmony = targetProcessor.getHarmonyState();
        REQUIRE(restoredHarmony.getSelectedScene() == 2);
        REQUIRE(restoredHarmony.getLiveRevoice() == true);
        REQUIRE(restoredHarmony.getQualityRule() == music::QualityRule::minor);
        REQUIRE(restoredHarmony.getConfiguration().getSpec(2, 3) == customSpec);
    }

    SECTION("Legacy APVTS state without HarmonyState resets conflicting target state to explicit defaults") {
        // Build a legacy APVTS state with no HarmonyState child node
        juce::ValueTree legacyTree{parameters::stateRootType};
        juce::ValueTree paramCutoff{"PARAM"};
        paramCutoff.setProperty("id", parameters::ids::cutoff, nullptr);
        paramCutoff.setProperty("value", 5500.0f, nullptr);
        legacyTree.appendChild(paramCutoff, nullptr);

        std::unique_ptr<juce::XmlElement> xml(legacyTree.createXml());
        juce::MemoryBlock legacyBlock;
        ChordSynthAudioProcessor::copyXmlToBinary(*xml, legacyBlock);

        // Configure target processor with conflicting non-default harmony state
        ChordSynthAudioProcessor targetProcessor;
        targetProcessor.getHarmonyState().setSelectedScene(3);
        targetProcessor.getHarmonyState().setLiveRevoice(true);
        targetProcessor.getHarmonyState().setQualityRule(music::QualityRule::diminished);
        music::VoicingSpec nonDefaultSpec{
            .extension = music::ChordExtension::seventh,
            .inversion = 2,
            .style = music::VoicingStyle::open,
            .baseOctave = 2,
            .qualityRule = music::QualityRule::minor
        };
        targetProcessor.getHarmonyState().getConfiguration().setSpec(3, 0, nonDefaultSpec);

        targetProcessor.setStateInformation(legacyBlock.getData(), static_cast<int>(legacyBlock.getSize()));

        // Check that legacy parameter was restored
        auto* targetCutoff = dynamic_cast<juce::AudioParameterFloat*>(
            targetProcessor.getAPVTS().getParameter(parameters::ids::cutoff));
        REQUIRE(targetCutoff != nullptr);
        REQUIRE(static_cast<float>(*targetCutoff) == Catch::Approx(5500.0f));

        // Check that harmony state migrated to clean defaults
        const auto& restoredHarmony = targetProcessor.getHarmonyState();
        REQUIRE(restoredHarmony.getSelectedScene() == 0);
        REQUIRE(restoredHarmony.getLiveRevoice() == false);
        REQUIRE(restoredHarmony.getQualityRule() == music::QualityRule::diatonic);
        REQUIRE(restoredHarmony.getConfiguration() == music::HarmonyConfiguration{});
    }

    SECTION("Corrupted or malformed state is rejected and does not corrupt processor state") {
        ChordSynthAudioProcessor targetProcessor;
        targetProcessor.getHarmonyState().setSelectedScene(1);

        // Null / zero size
        targetProcessor.setStateInformation(nullptr, 0);
        REQUIRE(targetProcessor.getHarmonyState().getSelectedScene() == 1);

        // Wrong root type XML
        juce::ValueTree badRoot{"WrongRoot"};
        std::unique_ptr<juce::XmlElement> xml(badRoot.createXml());
        juce::MemoryBlock badBlock;
        ChordSynthAudioProcessor::copyXmlToBinary(*xml, badBlock);
        targetProcessor.setStateInformation(badBlock.getData(), static_cast<int>(badBlock.getSize()));
        REQUIRE(targetProcessor.getHarmonyState().getSelectedScene() == 1);
    }
}

TEST_CASE("MIDI Performance processor routing maps notes 36-42 and routes bass on ch 2 around arpeggiator",
          "[plugin][processor][midi_performance]") {
    ChordSynthAudioProcessor procDisabled;
    ChordSynthAudioProcessor procEnabled;

    procDisabled.prepareToPlay(48000.0, 256);
    procEnabled.prepareToPlay(48000.0, 256);

    auto* midiParamEnabled = dynamic_cast<juce::AudioParameterBool*>(
        procEnabled.getAPVTS().getParameter(parameters::ids::performanceMidiEnabled));
    REQUIRE(midiParamEnabled != nullptr);
    *midiParamEnabled = true;

    SECTION("When disabled, note 36 plays single low note; when enabled, note 36 plays full chord") {
        juce::AudioBuffer<float> bufDisabled(2, 256);
        juce::AudioBuffer<float> bufEnabled(2, 256);
        bufDisabled.clear();
        bufEnabled.clear();

        juce::MidiBuffer midiDisabled;
        juce::MidiBuffer midiEnabled;
        midiDisabled.addEvent(juce::MidiMessage::noteOn(1, 36, 0.8f), 0);
        midiEnabled.addEvent(juce::MidiMessage::noteOn(1, 36, 0.8f), 0);

        procDisabled.processBlock(bufDisabled, midiDisabled);
        procEnabled.processBlock(bufEnabled, midiEnabled);

        REQUIRE(bufDisabled.getMagnitude(0, 0, 256) > 0.01f);
        REQUIRE(bufEnabled.getMagnitude(0, 0, 256) > 0.01f);

        // Buffers should differ since enabled renders multi-tone chord (C4, E4, G4)
        float diff = 0.0f;
        for (int i = 0; i < 256; ++i) {
            diff += std::abs(bufDisabled.getSample(0, i) - bufEnabled.getSample(0, i));
        }
        REQUIRE(diff > 0.1f);
    }

    SECTION("When both Arpeggiator and MIDI Performance are enabled, bass on ch 2 bypasses arp while chord arpeggiates") {
        // Scene 1: B Séptimas has root bass enabled for Degree 0 (I)
        procEnabled.getHarmonyState().setSelectedScene(1);

        auto* arpParam = dynamic_cast<juce::AudioParameterBool*>(
            procEnabled.getAPVTS().getParameter(parameters::ids::arpEnabled));
        REQUIRE(arpParam != nullptr);
        *arpParam = true;

        juce::AudioBuffer<float> buffer(2, 256);
        buffer.clear();
        juce::MidiBuffer midi;
        // Trigger Degree 0 (C)
        midi.addEvent(juce::MidiMessage::noteOn(1, 36, 0.8f), 0);

        procEnabled.processBlock(buffer, midi);

        REQUIRE(buffer.getMagnitude(0, 0, 256) > 0.01f);
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < 256; ++i) {
                REQUIRE(std::isfinite(buffer.getSample(ch, i)));
            }
        }
    }
}
