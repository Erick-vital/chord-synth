#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <functional>
#include <memory>
#include "interaction/ChordPerformanceController.h"
#include "music/HarmonyConfiguration.h"
#include "music/DiatonicChordVoicer.h"
#include "parameters/ParameterLayout.h"

namespace chordsynth::ui {

class ChordColorPanel : public juce::Component,
                        private juce::Timer {
public:
    ChordColorPanel(
        interaction::ChordPerformanceController& controller,
        music::HarmonyConfiguration& harmonyConfig,
        const music::DiatonicChordVoicer& chordVoicer,
        parameters::AudioProcessorValueTreeState* apvts = nullptr);
    ~ChordColorPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void updatePaletteDisplay();
    void updateTransformState();

    void triggerColorPress(int slotIndex);
    void triggerColorRelease(int slotIndex);
    void releaseAllColorKeys();

    interaction::TransformPalette getSelectedPalette() const noexcept { return currentPalette; }
    void setSelectedPalette(interaction::TransformPalette palette);

    std::function<void()> onTransformCommitted;

    void focusLost(juce::Component::FocusChangeType cause) override;
    void visibilityChanged() override;

private:
    void timerCallback() override;
    void handleCommitClicked();
    void updateSlotLabels();

    interaction::ChordPerformanceController& performanceController;
    music::HarmonyConfiguration& config;
    const music::DiatonicChordVoicer& voicer;

    interaction::TransformPalette currentPalette{interaction::TransformPalette::loFi};

    // UI Controls
    juce::Label headerLabel;
    juce::ToggleButton midiPerfToggle;
    juce::ComboBox paletteComboBox;
    std::array<juce::TextButton, 8> colorButtons;
    juce::TextButton commitButton;
    juce::Label feedbackLabel;
    juce::Label transformInfoLabel;

    std::unique_ptr<parameters::AudioProcessorValueTreeState::ButtonAttachment> midiPerfAttachment;
    std::unique_ptr<parameters::AudioProcessorValueTreeState::ComboBoxAttachment> paletteAttachment;

    std::array<bool, 8> physicalKeysDown{false, false, false, false, false, false, false, false};
    int feedbackTimerTicks{0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordColorPanel)
};

} // namespace chordsynth::ui
