#pragma once

#if JUCE_HEADLESS_PLUGIN_CLIENT
#include <juce_audio_processors_headless/juce_audio_processors_headless.h>
#else
#include <juce_audio_processors/juce_audio_processors.h>
#endif

#include "dsp/ChordSound.h"
#include "dsp/ChordVoice.h"
#include "dsp/Chorus.h"
#include "dsp/Filter.h"
#include "dsp/Reverb.h"
#include "dsp/TempoDelay.h"
#include "dsp/UiMidiQueue.h"
#include "interaction/MidiPerformanceMapper.h"
#include "music/Arpeggiator.h"
#include "music/DiatonicChordVoicer.h"
#include "parameters/ParameterLayout.h"
#include "state/HarmonyState.h"
#include <atomic>

namespace chordsynth {

namespace dsp {

// This private synthesiser is populated once with ChordVoice instances in the
// processor constructor, then its voice collection remains immutable.
class ChordSynthesiser final : public juce::Synthesiser {
public:
    void setWaveformForAllVoices(Waveform waveform) noexcept
    {
        for (auto* baseVoice : voices) {
            jassert(dynamic_cast<ChordVoice*>(baseVoice) != nullptr);
            static_cast<ChordVoice*>(baseVoice)->setWaveform(waveform);
        }
    }

    void setDetuneCentsForAllVoices(float detuneCents) noexcept
    {
        for (auto* baseVoice : voices) {
            jassert(dynamic_cast<ChordVoice*>(baseVoice) != nullptr);
            static_cast<ChordVoice*>(baseVoice)->setDetuneCents(detuneCents);
        }
    }

protected:
    juce::SynthesiserVoice* findVoiceToSteal(
        juce::SynthesiserSound* soundToPlay,
        int /*midiChannel*/,
        int midiNoteNumber) const override
    {
        juce::SynthesiserVoice* oldestMatchingNote = nullptr;
        juce::SynthesiserVoice* oldestReleased = nullptr;
        juce::SynthesiserVoice* oldestUnprotected = nullptr;
        juce::SynthesiserVoice* oldestNonProtected = nullptr;
        juce::SynthesiserVoice* low = nullptr;
        juce::SynthesiserVoice* top = nullptr;

        const auto chooseOldest = [](juce::SynthesiserVoice*& candidate,
                                     juce::SynthesiserVoice* voice) noexcept {
            if (candidate == nullptr || voice->wasStartedBefore(*candidate)) {
                candidate = voice;
            }
        };

        for (int index = 0; index < getNumVoices(); ++index) {
            auto* voice = getVoice(index);
            if (voice == nullptr || !voice->canPlaySound(soundToPlay)) {
                continue;
            }

            if (voice->getCurrentlyPlayingNote() == midiNoteNumber) {
                chooseOldest(oldestMatchingNote, voice);
            }
            if (!voice->isPlayingButReleased()) {
                if (low == nullptr || voice->getCurrentlyPlayingNote() < low->getCurrentlyPlayingNote()) {
                    low = voice;
                }
                if (top == nullptr || voice->getCurrentlyPlayingNote() > top->getCurrentlyPlayingNote()) {
                    top = voice;
                }
            }
        }

        if (top == low) {
            top = nullptr;
        }
        if (oldestMatchingNote != nullptr) {
            return oldestMatchingNote;
        }

        for (int index = 0; index < getNumVoices(); ++index) {
            auto* voice = getVoice(index);
            if (voice == nullptr || !voice->canPlaySound(soundToPlay)
                || voice == low || voice == top) {
                continue;
            }
            chooseOldest(oldestNonProtected, voice);
            if (voice->isPlayingButReleased()) {
                chooseOldest(oldestReleased, voice);
            }
            if (!voice->isKeyDown()) {
                chooseOldest(oldestUnprotected, voice);
            }
        }

        if (oldestReleased != nullptr) {
            return oldestReleased;
        }
        if (oldestUnprotected != nullptr) {
            return oldestUnprotected;
        }

        return oldestNonProtected != nullptr ? oldestNonProtected : (top != nullptr ? top : low);
    }
};

} // namespace dsp

class ChordSynthAudioProcessor : public juce::AudioProcessor {
public:
    ChordSynthAudioProcessor();
    ~ChordSynthAudioProcessor() override;

    dsp::UiMidiQueue& getUiMidiQueue() noexcept { return uiMidiQueue; }
    parameters::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
    state::HarmonyState& getHarmonyState() noexcept { return harmonyState; }
    const state::HarmonyState& getHarmonyState() const noexcept { return harmonyState; }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

private:
    static constexpr int numVoices = 16;
    dsp::ChordSynthesiser synth;
    dsp::Filter globalFilter;
    dsp::Chorus chorus;
    dsp::TempoDelay delay;
    dsp::Reverb reverb;
    music::Arpeggiator arpeggiator;
    music::DiatonicChordVoicer chordVoicer;
    state::HarmonyState harmonyState;
    interaction::MidiPerformanceMapper midiPerformanceMapper;
    dsp::UiMidiQueue uiMidiQueue;
    juce::MidiBuffer workingMidi;
    juce::MidiBuffer synthMidiScratch;
    parameters::AudioProcessorValueTreeState apvts;
    std::atomic<float>* waveformParameter{nullptr};
    std::atomic<float>* cutoffParameter{nullptr};
    std::atomic<float>* resonanceParameter{nullptr};
    std::atomic<float>* detuneParameter{nullptr};
    std::atomic<float>* chorusMixParameter{nullptr};
    std::atomic<float>* chorusRateParameter{nullptr};
    std::atomic<float>* chorusDepthParameter{nullptr};
    std::atomic<float>* delayMixParameter{nullptr};
    std::atomic<float>* delayFeedbackParameter{nullptr};
    std::atomic<float>* delayTimeMsParameter{nullptr};
    std::atomic<float>* delaySyncParameter{nullptr};
    std::atomic<float>* delaySyncRateParameter{nullptr};
    std::atomic<float>* reverbMixParameter{nullptr};
    std::atomic<float>* reverbRoomSizeParameter{nullptr};
    std::atomic<float>* reverbDampingParameter{nullptr};
    std::atomic<float>* reverbWidthParameter{nullptr};
    std::atomic<float>* arpEnabledParameter{nullptr};
    std::atomic<float>* arpModeParameter{nullptr};
    std::atomic<float>* arpRateParameter{nullptr};
    std::atomic<float>* arpGateParameter{nullptr};
    std::atomic<float>* keyParameter{nullptr};
    std::atomic<float>* scaleParameter{nullptr};
    std::atomic<float>* performanceMidiEnabledParameter{nullptr};
    std::atomic<float>* transformPaletteParameter{nullptr};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordSynthAudioProcessor)
};

} // namespace chordsynth
