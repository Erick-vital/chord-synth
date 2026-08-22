#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "parameters/ParameterLayout.h"
#include "parameters/ParameterIds.h"
#include "music/VoicedChord.h"
#include "ui/Utf8Text.h"

namespace chordsynth::ui {

class HarmonyToolbar : public juce::Component {
public:
    HarmonyToolbar(
        parameters::AudioProcessorValueTreeState& apvts,
        std::function<void(int newTonic)> onKeyChangedCallback,
        std::function<void(music::Scale newScale)> onScaleChangedCallback,
        std::function<void(bool isFreeMode)> onRuleModeChangedCallback,
        std::function<void()> onBeforeKeyChangeCallback = nullptr);
    ~HarmonyToolbar() override = default;

    void setTonic(int tonicIndex);
    void setScale(music::Scale scale);
    void setRuleMode(bool isFreeMode);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void handleRuleButtonClicked(bool isFreeMode);

    parameters::AudioProcessorValueTreeState& apvts;
    std::function<void(int newTonic)> onKeyChanged;
    std::function<void(music::Scale newScale)> onScaleChanged;
    std::function<void(bool isFreeMode)> onRuleModeChanged;
    std::function<void()> onBeforeKeyChange;

    // Field 1: Key / Tonalidad
    juce::Label keyLabel;
    juce::ComboBox keyComboBox;

    // Field 2: Scale / Escala
    juce::Label scaleLabel;
    juce::ComboBox scaleComboBox;

    // Field 3: Rules / Reglas
    juce::Label rulesLabel;
    juce::TextButton diatonicButton{utf8("Diat\xc3\xb3nico")};
    juce::TextButton freeButton{"Libre"};

    // Hint label
    juce::Label hintLabel;

    bool currentFreeMode{false};

    // Attachments declared after controls so they are destroyed before the ComboBox
    std::unique_ptr<parameters::AudioProcessorValueTreeState::ComboBoxAttachment> keyAttachment;
    std::unique_ptr<parameters::AudioProcessorValueTreeState::ComboBoxAttachment> scaleAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HarmonyToolbar)
};

} // namespace chordsynth::ui
