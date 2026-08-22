#include "HarmonyToolbar.h"
#include "ChordSynthLookAndFeel.h"

namespace chordsynth::ui {

namespace {
constexpr std::array<const char*, 12> pitchNames{
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};
} // namespace

HarmonyToolbar::HarmonyToolbar(
    parameters::AudioProcessorValueTreeState& state,
    std::function<void(int newTonic)> onKeyChangedCallback,
    std::function<void(music::Scale newScale)> onScaleChangedCallback,
    std::function<void(bool isFreeMode)> onRuleModeChangedCallback,
    std::function<void()> onBeforeKeyChangeCallback)
    : apvts(state),
      onKeyChanged(std::move(onKeyChangedCallback)),
      onScaleChanged(std::move(onScaleChangedCallback)),
      onRuleModeChanged(std::move(onRuleModeChangedCallback)),
      onBeforeKeyChange(std::move(onBeforeKeyChangeCallback))
{
    // Field 1: Tonalidad
    keyLabel.setText("TONALIDAD", juce::dontSendNotification);
    keyLabel.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
    keyLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(keyLabel);

    for (int i = 0; i < 12; ++i) {
        keyComboBox.addItem(pitchNames[static_cast<std::size_t>(i)], i + 1);
    }
    keyComboBox.setComponentID("key-select");
    keyComboBox.setSelectedId(1, juce::dontSendNotification);

    keyComboBox.onChange = [this]() {
        if (onBeforeKeyChange) {
            onBeforeKeyChange();
        }
        int selectedIndex = keyComboBox.getSelectedId() - 1;
        if (selectedIndex >= 0 && selectedIndex < 12 && onKeyChanged) {
            onKeyChanged(selectedIndex);
        }
    };

    keyAttachment = std::make_unique<parameters::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts,
        parameters::ids::key,
        keyComboBox);

    addAndMakeVisible(keyComboBox);

    // Field 2: Escala
    scaleLabel.setText("ESCALA", juce::dontSendNotification);
    scaleLabel.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
    scaleLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(scaleLabel);

    scaleComboBox.addItem("Mayor", 1);
    scaleComboBox.addItem("Menor natural", 2);
    scaleComboBox.setComponentID("scale-select");
    scaleComboBox.setSelectedId(1, juce::dontSendNotification);
    scaleComboBox.onChange = [this]() {
        if (onScaleChanged) {
            onScaleChanged(scaleComboBox.getSelectedId() == 2
                ? music::Scale::naturalMinor : music::Scale::major);
        }
    };
    scaleAttachment = std::make_unique<parameters::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts,
        parameters::ids::scale,
        scaleComboBox);
    addAndMakeVisible(scaleComboBox);

    // Field 3: Reglas (Diatónico / Libre)
    rulesLabel.setText("REGLAS", juce::dontSendNotification);
    rulesLabel.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
    rulesLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(rulesLabel);

    diatonicButton.setClickingTogglesState(false);
    diatonicButton.onClick = [this]() { handleRuleButtonClicked(false); };
    addAndMakeVisible(diatonicButton);

    freeButton.setClickingTogglesState(false);
    freeButton.onClick = [this]() { handleRuleButtonClicked(true); };
    addAndMakeVisible(freeButton);

    setRuleMode(false);

    // Hint label
    hintLabel.setText(utf8("Q\xe2\x80\x93" "U toca acordes  \xc2\xb7  1\xe2\x80\x93" "4 cambia voicing"), juce::dontSendNotification);
    hintLabel.setFont(juce::FontOptions(12.0f));
    hintLabel.setColour(juce::Label::textColourId, colors::textMuted);
    hintLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(hintLabel);
}

void HarmonyToolbar::setTonic(int tonicIndex)
{
    if (tonicIndex >= 0 && tonicIndex < 12) {
        keyComboBox.setSelectedId(tonicIndex + 1, juce::dontSendNotification);
    }
}

void HarmonyToolbar::setScale(music::Scale scale)
{
    scaleComboBox.setSelectedId(scale == music::Scale::naturalMinor ? 2 : 1,
                                juce::dontSendNotification);
}

void HarmonyToolbar::setRuleMode(bool isFreeMode)
{
    currentFreeMode = isFreeMode;

    if (!isFreeMode) {
        diatonicButton.setColour(juce::TextButton::buttonColourId, colors::panelTertiary);
        diatonicButton.setColour(juce::TextButton::textColourOffId, colors::text);
        freeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        freeButton.setColour(juce::TextButton::textColourOffId, colors::textMuted);
    } else {
        freeButton.setColour(juce::TextButton::buttonColourId, colors::panelTertiary);
        freeButton.setColour(juce::TextButton::textColourOffId, colors::text);
        diatonicButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        diatonicButton.setColour(juce::TextButton::textColourOffId, colors::textMuted);
    }
}

void HarmonyToolbar::handleRuleButtonClicked(bool isFreeMode)
{
    setRuleMode(isFreeMode);
    if (onRuleModeChanged) {
        onRuleModeChanged(isFreeMode);
    }
}

void HarmonyToolbar::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(colors::panel);
    g.fillRoundedRectangle(bounds, 14.0f);
    g.setColour(colors::line);
    g.drawRoundedRectangle(bounds, 14.0f, 1.0f);

    // Segmented background for rules buttons
    auto ruleBounds = diatonicButton.getBounds().getUnion(freeButton.getBounds()).toFloat().expanded(2.0f);
    g.setColour(juce::Colour(0xff0f1211));
    g.fillRoundedRectangle(ruleBounds, 10.0f);
    g.setColour(colors::line);
    g.drawRoundedRectangle(ruleBounds, 10.0f, 1.0f);
}

void HarmonyToolbar::resized()
{
    auto area = getLocalBounds().reduced(14, 8);

    // Hint label occupies the right side
    auto hintArea = area.removeFromRight(280);
    hintLabel.setBounds(hintArea);

    // Field 1: Key (Tonalidad)
    auto keyArea = area.removeFromLeft(110);
    keyLabel.setBounds(keyArea.removeFromTop(16));
    keyComboBox.setBounds(keyArea.removeFromTop(32));

    area.removeFromLeft(16);

    // Field 2: Scale (Escala)
    auto scaleArea = area.removeFromLeft(120);
    scaleLabel.setBounds(scaleArea.removeFromTop(16));
    scaleComboBox.setBounds(scaleArea.removeFromTop(32));

    area.removeFromLeft(16);

    // Field 3: Rules (Reglas)
    auto rulesArea = area.removeFromLeft(180);
    rulesLabel.setBounds(rulesArea.removeFromTop(16));
    auto buttonArea = rulesArea.removeFromTop(32);
    int halfWidth = buttonArea.getWidth() / 2;
    diatonicButton.setBounds(buttonArea.removeFromLeft(halfWidth).reduced(2));
    freeButton.setBounds(buttonArea.reduced(2));
}

} // namespace chordsynth::ui
