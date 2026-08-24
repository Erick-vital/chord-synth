#pragma once

#include <span>
#include <optional>
#include <utility>
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "music/HarmonyConfiguration.h"
#include "music/DiatonicChordVoicer.h"
#include "music/VoiceLeadingResolver.h"
#include "interaction/ChordTransform.h"
#include "interaction/PerformanceVoicing.h"
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
    std::optional<int> bassMidi{};
    float velocity{0.8f};
    int midiChannel{1};
};

using ActiveTransform = TransformSelection;

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

    void setScale(music::Scale newScale) noexcept;
    [[nodiscard]] music::Scale getScale() const noexcept { return scale; }

    void setDiatonicMode(bool enabled) noexcept;
    [[nodiscard]] bool isDiatonicMode() const noexcept { return diatonicMode; }

    void setLiveRevoice(bool enabled) noexcept { liveRevoice = enabled; }
    [[nodiscard]] bool getLiveRevoice() const noexcept { return liveRevoice; }

    void setMidiChannel(int channel) noexcept { midiChannel = channel; }
    [[nodiscard]] int getMidiChannel() const noexcept { return midiChannel; }

    // Performance Actions
    bool pressDegree(int degree, float velocity = 0.8f) noexcept;
    void releaseActiveChord() noexcept;
    void allNotesOff() noexcept;
    void revoiceActiveChordIfHeld(int degree) noexcept;

    // Temporary Transforms
    bool beginTransform(TransformPalette palette, TransformSlot slot) noexcept;
    void endTransform() noexcept;
    [[nodiscard]] std::optional<music::VoicingSpec> transformedSpecForActiveDegree() const noexcept;
    bool commitActiveTransform(music::HarmonyConfiguration& targetConfig) noexcept;
    [[nodiscard]] std::optional<std::pair<TransformPalette, TransformSlot>> getActiveTransform() const noexcept {
        if (activeTransform.has_value()) {
            return std::make_pair(activeTransform->palette, activeTransform->slot);
        }
        return std::nullopt;
    }
    [[nodiscard]] bool hasActiveTransform() const noexcept {
        return activeTransform.has_value();
    }

    [[nodiscard]] const std::optional<ActiveChord>& getActiveChord() const noexcept {
        return activeChord;
    }

private:
    bool sendVoicingDifferential(const music::VoicedChord& voiced) noexcept;
    [[nodiscard]] music::VoicedChord voiceForPerformance(
        int degree,
        const music::VoicingSpec& spec) const noexcept;
    [[nodiscard]] music::VoicingSpec getEffectiveBaseSpec(int degreeIndex) const noexcept;

    const music::HarmonyConfiguration& config;
    const music::DiatonicChordVoicer& voicer;
    MidiBatchOutput& output;

    int tonic{0}; // 0 = C
    music::Scale scale{music::Scale::major};
    bool diatonicMode{true};
    bool liveRevoice{false};
    int midiChannel{1};

    std::optional<ActiveChord> activeChord{std::nullopt};
    std::optional<ActiveTransform> activeTransform{std::nullopt};
};

} // namespace chordsynth::interaction
