#include <catch2/catch_test_macros.hpp>
#include "plugin/PluginProcessor.h"
#include "parameters/ParameterIds.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

using namespace chordsynth;

namespace {

// Deterministic linear congruential generator or fixed-seed mt19937 for repeatable soak test
constexpr std::uint32_t soakSeed = 0xC001D00Du;

struct DeterministicEventGenerator {
    std::mt19937 rng{soakSeed};
    std::uniform_int_distribution<int> noteDist{36, 84};       // 4 octaves
    std::uniform_real_distribution<float> velDist{0.2f, 1.0f};
    std::uniform_int_distribution<int> actionDist{0, 9};       // 0..6 noteOn, 7..8 noteOff, 9 paramChange
    std::uniform_int_distribution<int> waveformDist{0, 3};
    std::uniform_real_distribution<float> cutoffDist{20.0f, 20000.0f};
    std::uniform_real_distribution<float> resDist{0.1f, 2.0f};
    std::uniform_real_distribution<float> detuneDist{0.0f, 20.0f};

    std::vector<int> activeNotes;

    void generateBlockEvents(ChordSynthAudioProcessor& processor,
                             juce::MidiBuffer& midi,
                             int blockSize)
    {
        int action = actionDist(rng);
        if (action <= 6) { // Note on
            int note = noteDist(rng);
            float vel = velDist(rng);
            int offset = std::uniform_int_distribution<int>{0, blockSize - 1}(rng);
            midi.addEvent(juce::MidiMessage::noteOn(1, note, vel), offset);
            activeNotes.push_back(note);
        } else if (action <= 8 && !activeNotes.empty()) { // Note off
            std::uniform_int_distribution<size_t> idxDist{0, activeNotes.size() - 1};
            size_t idx = idxDist(rng);
            int note = activeNotes[idx];
            activeNotes.erase(activeNotes.begin() + idx);
            int offset = std::uniform_int_distribution<int>{0, blockSize - 1}(rng);
            midi.addEvent(juce::MidiMessage::noteOff(1, note), offset);
        } else { // Parameter change
            auto* rawWaveform = processor.getAPVTS().getRawParameterValue(parameters::ids::waveform);
            auto* rawCutoff = processor.getAPVTS().getRawParameterValue(parameters::ids::cutoff);
            auto* rawRes = processor.getAPVTS().getRawParameterValue(parameters::ids::resonance);
            auto* rawDetune = processor.getAPVTS().getRawParameterValue(parameters::ids::detune);
            auto* rawChorusMix = processor.getAPVTS().getRawParameterValue(parameters::ids::chorusMix);

            if (rawWaveform) rawWaveform->store(static_cast<float>(waveformDist(rng)), std::memory_order_relaxed);
            if (rawCutoff) rawCutoff->store(cutoffDist(rng), std::memory_order_relaxed);
            if (rawRes) rawRes->store(resDist(rng), std::memory_order_relaxed);
            if (rawDetune) rawDetune->store(detuneDist(rng), std::memory_order_relaxed);
            if (rawChorusMix) rawChorusMix->store(std::uniform_real_distribution<float>{0.0f, 1.0f}(rng), std::memory_order_relaxed);
        }
    }
};

} // namespace

TEST_CASE("RenderSafety: Sample rates and block sizes matrix", "[dsp][safety]") {
    const std::array<double, 3> sampleRates{44100.0, 48000.0, 96000.0};
    const std::array<int, 6> blockSizes{1, 16, 64, 256, 512, 1024};

    for (const auto sr : sampleRates) {
        for (const auto bs : blockSizes) {
            ChordSynthAudioProcessor processor;
            processor.prepareToPlay(sr, bs);

            juce::AudioBuffer<float> buffer(2, bs);
            buffer.clear();
            juce::MidiBuffer midi;

            // 1. Silent block
            processor.processBlock(buffer, midi);
            for (int ch = 0; ch < 2; ++ch) {
                for (int s = 0; s < bs; ++s) {
                    REQUIRE(std::isfinite(buffer.getSample(ch, s)));
                }
            }

            // 2. Note On
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);
            buffer.clear();
            processor.processBlock(buffer, midi);
            for (int ch = 0; ch < 2; ++ch) {
                for (int s = 0; s < bs; ++s) {
                    REQUIRE(std::isfinite(buffer.getSample(ch, s)));
                }
            }

            // 3. Render 10 blocks
            for (int b = 0; b < 10; ++b) {
                buffer.clear();
                midi.clear();
                processor.processBlock(buffer, midi);
                for (int ch = 0; ch < 2; ++ch) {
                    for (int s = 0; s < bs; ++s) {
                        float sample = buffer.getSample(ch, s);
                        REQUIRE(std::isfinite(sample));
                        REQUIRE(std::abs(sample) <= 10.0f); // bounded
                    }
                }
            }

            // 4. Note Off
            midi.clear();
            midi.addEvent(juce::MidiMessage::noteOff(1, 60), 0);
            buffer.clear();
            processor.processBlock(buffer, midi);
            for (int ch = 0; ch < 2; ++ch) {
                for (int s = 0; s < bs; ++s) {
                    REQUIRE(std::isfinite(buffer.getSample(ch, s)));
                }
            }
        }
    }
}

TEST_CASE("RenderSafety: Deterministic 60-second offline soak render", "[dsp][safety][soak]") {
    const double sampleRate = 48000.0;
    const int blockSize = 256;
    const int totalBlocks = static_cast<int>((60.0 * sampleRate) / blockSize); // 11,250 blocks

    ChordSynthAudioProcessor processor1;
    processor1.prepareToPlay(sampleRate, blockSize);
    DeterministicEventGenerator gen1;

    ChordSynthAudioProcessor processor2;
    processor2.prepareToPlay(sampleRate, blockSize);
    DeterministicEventGenerator gen2;

    juce::AudioBuffer<float> buffer1(2, blockSize);
    juce::AudioBuffer<float> buffer2(2, blockSize);

    float maxPeak1 = 0.0f;
    float maxPeak2 = 0.0f;

    for (int block = 0; block < totalBlocks; ++block) {
        buffer1.clear();
        juce::MidiBuffer midi1;
        gen1.generateBlockEvents(processor1, midi1, blockSize);
        processor1.processBlock(buffer1, midi1);

        buffer2.clear();
        juce::MidiBuffer midi2;
        gen2.generateBlockEvents(processor2, midi2, blockSize);
        processor2.processBlock(buffer2, midi2);

        for (int ch = 0; ch < 2; ++ch) {
            for (int s = 0; s < blockSize; ++s) {
                float s1 = buffer1.getSample(ch, s);
                float s2 = buffer2.getSample(ch, s);

                REQUIRE(std::isfinite(s1));
                REQUIRE(std::isfinite(s2));

                maxPeak1 = std::max(maxPeak1, std::abs(s1));
                maxPeak2 = std::max(maxPeak2, std::abs(s2));

                // Strict determinism between runs with same seed
                REQUIRE(s1 == s2);
            }
        }
    }

    REQUIRE(maxPeak1 > 0.01f); // Verify that audio actually played
    REQUIRE(maxPeak1 <= 8.0f);  // Output is well-bounded across polyphony
}

TEST_CASE("RenderSafety: Extreme parameter automation and edge values", "[dsp][safety]") {
    ChordSynthAudioProcessor processor;
    processor.prepareToPlay(48000.0, 128);

    auto* rawWaveform = processor.getAPVTS().getRawParameterValue(parameters::ids::waveform);
    auto* rawCutoff = processor.getAPVTS().getRawParameterValue(parameters::ids::cutoff);
    auto* rawRes = processor.getAPVTS().getRawParameterValue(parameters::ids::resonance);
    auto* rawDetune = processor.getAPVTS().getRawParameterValue(parameters::ids::detune);
    auto* rawChorusMix = processor.getAPVTS().getRawParameterValue(parameters::ids::chorusMix);
    auto* rawDelayMix = processor.getAPVTS().getRawParameterValue(parameters::ids::delayMix);
    auto* rawDelayFeedback = processor.getAPVTS().getRawParameterValue(parameters::ids::delayFeedback);
    auto* rawDelayTime = processor.getAPVTS().getRawParameterValue(parameters::ids::delayTimeMs);
    auto* rawReverbMix = processor.getAPVTS().getRawParameterValue(parameters::ids::reverbMix);
    auto* rawReverbRoom = processor.getAPVTS().getRawParameterValue(parameters::ids::reverbRoomSize);
    auto* rawReverbDamp = processor.getAPVTS().getRawParameterValue(parameters::ids::reverbDamping);

    juce::AudioBuffer<float> buffer(2, 128);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.0f), 0);
    midi.addEvent(juce::MidiMessage::noteOn(1, 64, 1.0f), 0);
    midi.addEvent(juce::MidiMessage::noteOn(1, 67, 1.0f), 0);

    const std::vector<float> extremeCutoffs{-100.0f, 0.0f, 5.0f, 20.0f, 1000.0f, 24000.0f, 100000.0f,
                                           std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()};
    const std::vector<float> extremeResonances{-10.0f, 0.0f, 0.1f, 1.0f, 2.0f, 50.0f,
                                              std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()};
    const std::vector<float> extremeDetunes{-50.0f, 0.0f, 7.0f, 20.0f, 100.0f,
                                            std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()};
    const std::vector<float> extremeWaveforms{-5.0f, 0.0f, 1.0f, 2.0f, 3.0f, 10.0f,
                                              std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()};

    for (size_t i = 0; i < extremeCutoffs.size(); ++i) {
        if (rawCutoff) rawCutoff->store(extremeCutoffs[i], std::memory_order_relaxed);
        if (rawRes) rawRes->store(extremeResonances[i % extremeResonances.size()], std::memory_order_relaxed);
        if (rawDetune) rawDetune->store(extremeDetunes[i % extremeDetunes.size()], std::memory_order_relaxed);
        if (rawWaveform) rawWaveform->store(extremeWaveforms[i % extremeWaveforms.size()], std::memory_order_relaxed);
        if (rawChorusMix) rawChorusMix->store(extremeWaveforms[i % extremeWaveforms.size()], std::memory_order_relaxed);
        if (rawDelayMix) rawDelayMix->store(extremeWaveforms[i % extremeWaveforms.size()], std::memory_order_relaxed);
        if (rawDelayFeedback) rawDelayFeedback->store(extremeWaveforms[i % extremeWaveforms.size()], std::memory_order_relaxed);
        if (rawDelayTime) rawDelayTime->store(extremeCutoffs[i], std::memory_order_relaxed);
        if (rawReverbMix) rawReverbMix->store(extremeWaveforms[i % extremeWaveforms.size()], std::memory_order_relaxed);
        if (rawReverbRoom) rawReverbRoom->store(extremeWaveforms[i % extremeWaveforms.size()], std::memory_order_relaxed);
        if (rawReverbDamp) rawReverbDamp->store(extremeWaveforms[i % extremeWaveforms.size()], std::memory_order_relaxed);

        buffer.clear();
        processor.processBlock(buffer, midi);
        midi.clear();

        for (int ch = 0; ch < 2; ++ch) {
            for (int s = 0; s < 128; ++s) {
                float sample = buffer.getSample(ch, s);
                REQUIRE(std::isfinite(sample));
            }
        }
    }
}

TEST_CASE("RenderSafety: All-notes-off and release decay", "[dsp][safety]") {
    ChordSynthAudioProcessor processor;
    processor.prepareToPlay(48000.0, 256);

    juce::AudioBuffer<float> buffer(2, 256);
    juce::MidiBuffer midi;

    // Trigger all 16 voices
    for (int i = 0; i < 16; ++i) {
        midi.addEvent(juce::MidiMessage::noteOn(1, 40 + i, 0.8f), 0);
    }
    buffer.clear();
    processor.processBlock(buffer, midi);
    midi.clear();

    REQUIRE(buffer.getMagnitude(0, 0, 256) > 0.01f);

    // Render 5 blocks to stabilize
    for (int b = 0; b < 5; ++b) {
        buffer.clear();
        processor.processBlock(buffer, midi);
    }

    // Send allNotesOff message
    midi.addEvent(juce::MidiMessage::allNotesOff(1), 0);
    buffer.clear();
    processor.processBlock(buffer, midi);
    midi.clear();

    // Render 100 blocks (~0.53 seconds, envelope release is ~120ms)
    for (int b = 0; b < 100; ++b) {
        buffer.clear();
        processor.processBlock(buffer, midi);
    }

    // After release time has passed, magnitude should be essentially zero
    float magL = buffer.getMagnitude(0, 0, 256);
    float magR = buffer.getMagnitude(1, 0, 256);
    REQUIRE(magL < 1e-4f);
    REQUIRE(magR < 1e-4f);
}

TEST_CASE("RenderSafety: Repeated processor lifecycle recreation", "[dsp][safety]") {
    for (int cycle = 0; cycle < 50; ++cycle) {
        auto processor = std::make_unique<ChordSynthAudioProcessor>();
        processor->prepareToPlay(48000.0, 512);

        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);

        processor->processBlock(buffer, midi);
        REQUIRE(buffer.getMagnitude(0, 0, 512) > 0.001f);

        processor->releaseResources();
    }
}

TEST_CASE("RenderSafety: State save and restore during host lifecycle", "[dsp][safety]") {
    ChordSynthAudioProcessor processor;
    processor.prepareToPlay(48000.0, 256);

    juce::AudioBuffer<float> buffer(2, 256);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);

    processor.processBlock(buffer, midi);
    midi.clear();

    // Save state while voice is active
    juce::MemoryBlock stateData;
    processor.getStateInformation(stateData);
    REQUIRE(stateData.getSize() > 0);

    // Modify parameters
    auto* rawCutoff = processor.getAPVTS().getRawParameterValue(parameters::ids::cutoff);
    if (rawCutoff) rawCutoff->store(500.0f, std::memory_order_relaxed);

    // Restore state
    processor.setStateInformation(stateData.getData(), static_cast<int>(stateData.getSize()));

    // Continue processing seamlessly
    buffer.clear();
    processor.processBlock(buffer, midi);
    for (int ch = 0; ch < 2; ++ch) {
        for (int s = 0; s < 256; ++s) {
            REQUIRE(std::isfinite(buffer.getSample(ch, s)));
        }
    }
}
