#pragma once

#include <span>
#include <optional>
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "music/HarmonyConfiguration.h"
#include "music/DiatonicChordVoicer.h"
#include "dsp/UiMidiQueue.h"

namespace chordsynth::interaction {

class MidiBatchOutput {
public:
    virtual ~MidiBatchOutput() = default;
    virtual bool tryPushBatch(std::span<const juce::MidiMessage> messages) noexcept = 0;
};

class QueueMidiBatchOutput final : public MidiBatchOutput {
public:
    explicit QueueMidiBatchOutput(dsp::UiMidiQueue& queueRef) noexcept : queue(queueRef) {}
    bool tryPushBatch(std::span<const juce::MidiMessage> messages) noexcept override {
        return queue.tryPushBatch(messages);
    }
private:
    dsp::UiMidiQueue& queue;
};

struct ActiveChord {
    int degree{0};
    music::NoteSet notes{};
    float velocity{0.8f};
    int midiChannel{1};
};

class ChordPerformanceController {
public:
    ChordPerformanceController(
        const music::HarmonyConfiguration& harmonyConfig,
        const music::DiatonicChordVoicer& chordVoicer,
        MidiBatchOutput& midiOutput) noexcept;

    ~ChordPerformanceController();

    // Context & State
    void setTonic(int newTonic) noexcept;
    [[nodiscard]] int getTonic() const noexcept { return tonic; }

    void setScale(music::Scale newScale) noexcept { scale = newScale; }
    [[nodiscard]] music::Scale getScale() const noexcept { return scale; }

    void setDiatonicMode(bool enabled) noexcept { diatonicMode = enabled; }
    [[nodiscard]] bool isDiatonicMode() const noexcept { return diatonicMode; }

    void setScene(int newSceneIndex) noexcept;
    [[nodiscard]] int getScene() const noexcept { return currentScene; }

    void setLiveRevoice(bool enabled) noexcept { liveRevoice = enabled; }
    [[nodiscard]] bool getLiveRevoice() const noexcept { return liveRevoice; }

    void setMidiChannel(int channel) noexcept { midiChannel = channel; }
    [[nodiscard]] int getMidiChannel() const noexcept { return midiChannel; }

    // Performance Actions
    bool pressDegree(int degree, float velocity = 0.8f) noexcept;
    void releaseActiveChord() noexcept;
    void allNotesOff() noexcept;
    void revoiceActiveChordIfHeld(int degree) noexcept;

    [[nodiscard]] const std::optional<ActiveChord>& getActiveChord() const noexcept {
        return activeChord;
    }

private:
    void applyLiveRevoicing(int targetScene) noexcept;

    const music::HarmonyConfiguration& config;
    const music::DiatonicChordVoicer& voicer;
    MidiBatchOutput& output;

    int tonic{0}; // 0 = C
    music::Scale scale{music::Scale::major};
    bool diatonicMode{true};
    int currentScene{0}; // 0..3 (A..D)
    bool liveRevoice{false};
    int midiChannel{1};

    std::optional<ActiveChord> activeChord{std::nullopt};
};

} // namespace chordsynth::interaction
