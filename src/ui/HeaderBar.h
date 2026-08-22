#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace chordsynth::ui {

class HeaderBar : public juce::Component {
public:
    HeaderBar(
        std::function<void(int presetIndex)> onPresetSelectedCallback,
        std::function<void()> onAudioSettingsClickedCallback,
        bool isStandalone);
    ~HeaderBar() override = default;

    void setPresetNames(const juce::StringArray& names, int currentSelectedIndex);
    void setAudioActive(bool isActive);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    std::function<void(int presetIndex)> onPresetSelected;
    std::function<void()> onAudioSettingsClicked;
    bool isStandaloneApp{false};
    bool audioActive{true};

    // Brand section
    juce::Label logoLabel;
    juce::Label brandTitleLabel;
    juce::Label brandSubtitleLabel;

    // Preset section
    juce::Label presetLabel;
    juce::ComboBox presetComboBox;

    // Status / Settings section
    juce::Label statusDotLabel;
    juce::Label statusTextLabel;
    juce::TextButton audioSettingsButton{"Audio / MIDI"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderBar)
};

} // namespace chordsynth::ui
