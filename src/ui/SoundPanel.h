#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include "parameters/ParameterLayout.h"
#include "parameters/ParameterIds.h"

namespace chordsynth::ui {

class SoundPanel : public juce::Component {
public:
    explicit SoundPanel(parameters::AudioProcessorValueTreeState& apvts);
    ~SoundPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void updateArpControls();

private:
    void updateAdvancedVisibility();

    parameters::AudioProcessorValueTreeState& apvts;

    // Header elements
    juce::Label headerTitleLabel;
    juce::Label headerSubtleLabel;
    juce::TextButton advancedToggleButton{"Avanzado"};

    // 1. Oscillator Module
    juce::Label oscModuleLabel;
    juce::Label waveLabel;
    juce::ComboBox waveComboBox;
    juce::Label detuneLabel;
    juce::Slider detuneSlider;

    // 2. Filter Module
    juce::Label filterModuleLabel;
    juce::Label cutoffLabel;
    juce::Slider cutoffSlider;
    juce::Label resonanceLabel;
    juce::Slider resonanceSlider;

    // 3. Arpeggiator Module
    juce::Label arpModuleLabel;
    juce::Label arpModeLabel;
    juce::ComboBox arpModeComboBox; // Items: 1: Off, 2: Up, 3: Down, 4: Up/Down, 5: Random
    juce::Label arpRateLabel;
    juce::ComboBox arpRateComboBox; // 1/4, 1/8, 1/16
    juce::Label arpGateLabel;
    juce::Slider arpGateSlider;

    // 4. Space / Effects Module (Main mixes)
    juce::Label spaceModuleLabel;
    juce::Label chorusMixLabel;
    juce::Slider chorusMixSlider;
    juce::Label delayMixLabel;
    juce::Slider delayMixSlider;
    juce::Label reverbMixLabel;
    juce::Slider reverbMixSlider;

    // Advanced Controls (Expandable drawer)
    bool isAdvancedOpen{false};

    juce::Label chorusRateLabel;
    juce::Slider chorusRateSlider;
    juce::Label chorusDepthLabel;
    juce::Slider chorusDepthSlider;

    juce::Label delayFeedbackLabel;
    juce::Slider delayFeedbackSlider;
    juce::Label delayTimeLabel;
    juce::Slider delayTimeSlider;
    juce::ToggleButton delaySyncToggle{"Sync"};
    juce::Label delaySyncRateLabel;
    juce::ComboBox delaySyncRateComboBox;

    juce::Label reverbRoomSizeLabel;
    juce::Slider reverbRoomSizeSlider;
    juce::Label reverbDampingLabel;
    juce::Slider reverbDampingSlider;
    juce::Label reverbWidthLabel;
    juce::Slider reverbWidthSlider;

    // Attachments declared AFTER GUI controls to guarantee safe RAII destruction order
    std::unique_ptr<parameters::AudioProcessorValueTreeState::ComboBoxAttachment> waveAttachment;
    std::unique_ptr<parameters::AudioProcessorValueTreeState::SliderAttachment> detuneAttachment;

    std::unique_ptr<parameters::AudioProcessorValueTreeState::SliderAttachment> cutoffAttachment;
    std::unique_ptr<parameters::AudioProcessorValueTreeState::SliderAttachment> resonanceAttachment;

    std::unique_ptr<parameters::AudioProcessorValueTreeState::ComboBoxAttachment> arpRateAttachment;
    std::unique_ptr<parameters::AudioProcessorValueTreeState::SliderAttachment> arpGateAttachment;

    std::unique_ptr<parameters::AudioProcessorValueTreeState::SliderAttachment> chorusMixAttachment;
    std::unique_ptr<parameters::AudioProcessorValueTreeState::SliderAttachment> chorusRateAttachment;
    std::unique_ptr<parameters::AudioProcessorValueTreeState::SliderAttachment> chorusDepthAttachment;

    std::unique_ptr<parameters::AudioProcessorValueTreeState::SliderAttachment> delayMixAttachment;
    std::unique_ptr<parameters::AudioProcessorValueTreeState::SliderAttachment> delayFeedbackAttachment;
    std::unique_ptr<parameters::AudioProcessorValueTreeState::SliderAttachment> delayTimeAttachment;
    std::unique_ptr<parameters::AudioProcessorValueTreeState::ButtonAttachment> delaySyncAttachment;
    std::unique_ptr<parameters::AudioProcessorValueTreeState::ComboBoxAttachment> delaySyncRateAttachment;

    std::unique_ptr<parameters::AudioProcessorValueTreeState::SliderAttachment> reverbMixAttachment;
    std::unique_ptr<parameters::AudioProcessorValueTreeState::SliderAttachment> reverbRoomSizeAttachment;
    std::unique_ptr<parameters::AudioProcessorValueTreeState::SliderAttachment> reverbDampingAttachment;
    std::unique_ptr<parameters::AudioProcessorValueTreeState::SliderAttachment> reverbWidthAttachment;
 
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundPanel)
};

} // namespace chordsynth::ui
