#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace chordsynth::ui {

namespace colors {
    inline const juce::Colour background      { 0xff0d1010 };
    inline const juce::Colour panel           { 0xff151918 };
    inline const juce::Colour panelSecondary  { 0xff1b201e };
    inline const juce::Colour panelTertiary   { 0xff232a27 };
    inline const juce::Colour line            { 0xff303834 };
    inline const juce::Colour text            { 0xfff1f0e9 };
    inline const juce::Colour textMuted       { 0xff969d97 };
    inline const juce::Colour accent          { 0xffd5ff64 };
    inline const juce::Colour accentInk       { 0xff172000 };
    inline const juce::Colour cyan            { 0xff76d9c2 };
    inline const juce::Colour amber           { 0xfff3b65d };
    inline const juce::Colour danger          { 0xffff7d73 };
    inline const juce::Colour keyTop          { 0xff29302d };
    inline const juce::Colour keyBottom       { 0xff1a1f1d };
    inline const juce::Colour keyBorder       { 0xff3a433f };
    inline const juce::Colour keyBorderHover  { 0xff59655f };
    inline const juce::Colour keyShadow       { 0xff090b0a };
} // namespace colors

class ChordSynthLookAndFeel : public juce::LookAndFeel_V4 {
public:
    ChordSynthLookAndFeel();
    ~ChordSynthLookAndFeel() override = default;

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override;

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    void drawTickBox(juce::Graphics& g, juce::Component& component,
                     float x, float y, float w, float h,
                     bool ticked, bool isEnabled,
                     bool shouldDrawButtonAsHighlighted,
                     bool shouldDrawButtonAsDown) override;

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown) override;

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;
};

} // namespace chordsynth::ui
