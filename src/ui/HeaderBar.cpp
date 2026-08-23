#include "HeaderBar.h"
#include "ChordSynthLookAndFeel.h"
#include <algorithm>

namespace chordsynth::ui {

HeaderBar::HeaderBar(
    std::function<void(int presetIndex)> onPresetSelectedCallback,
    std::function<void()> onAudioSettingsClickedCallback,
    bool isStandalone)
    : onPresetSelected(std::move(onPresetSelectedCallback)),
      onAudioSettingsClicked(std::move(onAudioSettingsClickedCallback)),
      isStandaloneApp(isStandalone)
{
    // Brand Logo
    logoLabel.setText("CS", juce::dontSendNotification);
    logoLabel.setFont(juce::FontOptions(18.0f).withStyle("Bold"));
    logoLabel.setColour(juce::Label::backgroundColourId, colors::accent);
    logoLabel.setColour(juce::Label::textColourId, colors::accentInk);
    logoLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(logoLabel);

    // Brand Title
    brandTitleLabel.setText("ChordSynth", juce::dontSendNotification);
    brandTitleLabel.setFont(juce::FontOptions(18.0f).withStyle("Bold"));
    brandTitleLabel.setColour(juce::Label::textColourId, colors::text);
    addAndMakeVisible(brandTitleLabel);

    // Brand Subtitle
    brandSubtitleLabel.setText("Diatonic Chord Performance", juce::dontSendNotification);
    brandSubtitleLabel.setFont(juce::FontOptions(11.0f));
    brandSubtitleLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(brandSubtitleLabel);

    // Preset Label & ComboBox
    presetLabel.setText("PRESET", juce::dontSendNotification);
    presetLabel.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
    presetLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(presetLabel);

    presetComboBox.setComponentID("preset-select");
    presetComboBox.addItem("Default (Init)", 1);
    presetComboBox.setSelectedId(1, juce::dontSendNotification);
    presetComboBox.onChange = [this]() {
        int selectedId = presetComboBox.getSelectedId();
        if (selectedId > 0 && onPresetSelected) {
            onPresetSelected(selectedId - 1);
        }
    };
    addAndMakeVisible(presetComboBox);

    // Status Indicator
    statusDotLabel.setColour(juce::Label::backgroundColourId, colors::cyan);
    addAndMakeVisible(statusDotLabel);

    statusTextLabel.setText(isStandaloneApp ? "Audio Activo" : "Host Sync", juce::dontSendNotification);
    statusTextLabel.setFont(juce::FontOptions(12.0f));
    statusTextLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(statusTextLabel);

    // Audio / MIDI button (for Standalone device setup or Host info for VST3)
    if (isStandaloneApp) {
        audioSettingsButton.setButtonText("Audio / MIDI");
        audioSettingsButton.onClick = [this]() {
            if (onAudioSettingsClicked) {
                onAudioSettingsClicked();
            }
        };
    } else {
        audioSettingsButton.setButtonText("VST3 (Host Managed)");
        audioSettingsButton.setEnabled(false);
    }
    audioSettingsButton.setColour(juce::TextButton::buttonColourId, colors::panelSecondary);
    audioSettingsButton.setColour(juce::TextButton::textColourOffId, colors::text);
    addAndMakeVisible(audioSettingsButton);
}

void HeaderBar::setPresetNames(const juce::StringArray& names, int currentSelectedIndex)
{
    presetComboBox.clear(juce::dontSendNotification);
    for (int i = 0; i < names.size(); ++i) {
        presetComboBox.addItem(names[i], i + 1);
    }
    if (currentSelectedIndex >= 0 && currentSelectedIndex < names.size()) {
        presetComboBox.setSelectedId(currentSelectedIndex + 1, juce::dontSendNotification);
    }
}

void HeaderBar::setAudioActive(bool isActive)
{
    audioActive = isActive;
    statusDotLabel.setColour(
        juce::Label::backgroundColourId,
        isActive ? colors::cyan : colors::danger);
    statusTextLabel.setText(
        isStandaloneApp ? (isActive ? "Audio Activo" : "Audio Inactivo")
                        : (isActive ? "Host Sync" : "Host Offline"),
        juce::dontSendNotification);
    repaint();
}

void HeaderBar::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(juce::Colour(0xff121615));
    g.fillRect(bounds);

    g.setColour(colors::line);
    g.drawHorizontalLine(getHeight() - 1, 0.0f, static_cast<float>(getWidth()));

    // Draw status pill background
    auto pillBounds = statusDotLabel.getBounds().getUnion(statusTextLabel.getBounds()).toFloat().expanded(8.0f, 4.0f);
    g.setColour(colors::panelSecondary);
    g.fillRoundedRectangle(pillBounds, pillBounds.getHeight() * 0.5f);
    g.setColour(colors::line);
    g.drawRoundedRectangle(pillBounds, pillBounds.getHeight() * 0.5f, 1.0f);
}

void HeaderBar::resized()
{
    auto area = getLocalBounds().reduced(18, 10);

    // Left: Brand
    auto brandArea = area.removeFromLeft(220);
    auto logoArea = brandArea.removeFromLeft(38);
    logoLabel.setBounds(logoArea.withSizeKeepingCentre(36, 36));
    brandArea.removeFromLeft(10);
    brandTitleLabel.setBounds(brandArea.removeFromTop(20));
    brandSubtitleLabel.setBounds(brandArea.removeFromTop(16));

    // Right: Settings button & Status pill
    auto rightArea = area.removeFromRight(320);
    audioSettingsButton.setBounds(rightArea.removeFromRight(150).withSizeKeepingCentre(140, 32));
    rightArea.removeFromRight(14);

    auto statusArea = rightArea.withSizeKeepingCentre(130, 28);
    statusDotLabel.setBounds(statusArea.removeFromLeft(10).withSizeKeepingCentre(8, 8));
    statusArea.removeFromLeft(6);
    statusTextLabel.setBounds(statusArea);

    // Center: Preset selector
    auto centerArea = area.withSizeKeepingCentre(std::min(300, area.getWidth()), 40);
    presetLabel.setBounds(centerArea.removeFromLeft(60).withSizeKeepingCentre(60, 28));
    centerArea.removeFromLeft(6);
    presetComboBox.setBounds(centerArea.withSizeKeepingCentre(centerArea.getWidth(), 30));
}

} // namespace chordsynth::ui
