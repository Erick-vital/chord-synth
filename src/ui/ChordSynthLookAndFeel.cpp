#include "ChordSynthLookAndFeel.h"
#include <algorithm>

namespace chordsynth::ui {

ChordSynthLookAndFeel::ChordSynthLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, colors::background);

    // Global text colors
    setColour(juce::Label::textColourId, colors::text);
    setColour(juce::TextEditor::textColourId, colors::text);
    setColour(juce::TextEditor::backgroundColourId, colors::panelSecondary);
    setColour(juce::TextEditor::outlineColourId, colors::line);

    // ComboBox colors
    setColour(juce::ComboBox::backgroundColourId, colors::panelSecondary);
    setColour(juce::ComboBox::textColourId, colors::text);
    setColour(juce::ComboBox::outlineColourId, colors::line);
    setColour(juce::ComboBox::arrowColourId, colors::textMuted);
    setColour(juce::PopupMenu::backgroundColourId, colors::panelSecondary);
    setColour(juce::PopupMenu::textColourId, colors::text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, colors::panelTertiary);
    setColour(juce::PopupMenu::highlightedTextColourId, colors::accent);

    // Slider colors
    setColour(juce::Slider::thumbColourId, colors::accent);
    setColour(juce::Slider::trackColourId, colors::accent);
    setColour(juce::Slider::backgroundColourId, colors::panelTertiary);
    setColour(juce::Slider::rotarySliderFillColourId, colors::accent);
    setColour(juce::Slider::rotarySliderOutlineColourId, colors::line);
    setColour(juce::Slider::textBoxTextColourId, colors::text);
    setColour(juce::Slider::textBoxBackgroundColourId, colors::panelSecondary);
    setColour(juce::Slider::textBoxOutlineColourId, colors::line);

    // TextButton colors
    setColour(juce::TextButton::buttonColourId, colors::panelSecondary);
    setColour(juce::TextButton::buttonOnColourId, colors::panelTertiary);
    setColour(juce::TextButton::textColourOffId, colors::text);
    setColour(juce::TextButton::textColourOnId, colors::accent);

    // ToggleButton colors
    setColour(juce::ToggleButton::textColourId, colors::textMuted);
    setColour(juce::ToggleButton::tickColourId, colors::accent);
    setColour(juce::ToggleButton::tickDisabledColourId, colors::line);
}

void ChordSynthLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                             float sliderPosProportional, float rotaryStartAngle,
                                             float rotaryEndAngle, juce::Slider& slider)
{
    auto outline = slider.findColour(juce::Slider::rotarySliderOutlineColourId);
    auto fill = slider.findColour(juce::Slider::rotarySliderFillColourId);

    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(4.0f);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = 3.0f;
    auto arcRadius = radius - lineW * 0.5f;

    juce::Point<float> centre(bounds.getCentreX(), bounds.getCentreY());

    // Background track arc
    juce::Path backgroundArc;
    backgroundArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                                rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(outline);
    g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (slider.isEnabled()) {
        // Value fill arc
        juce::Path valueArc;
        valueArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                               rotaryStartAngle, toAngle, true);
        g.setColour(fill);
        g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Dial thumb indicator
    auto thumbWidth = 3.0f;
    juce::Path thumb;
    thumb.addRectangle(-thumbWidth * 0.5f, -radius + lineW, thumbWidth, radius * 0.45f);
    g.setColour(colors::text);
    g.fillPath(thumb, juce::AffineTransform::rotation(toAngle).translated(centre.x, centre.y));
}

void ChordSynthLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                             float sliderPos, [[maybe_unused]] float minSliderPos,
                                             [[maybe_unused]] float maxSliderPos,
                                             [[maybe_unused]] juce::Slider::SliderStyle style,
                                             juce::Slider& slider)
{
    auto trackBounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                             static_cast<float>(width), static_cast<float>(height));

    if (slider.isHorizontal()) {
        auto trackY = trackBounds.getCentreY() - 2.0f;
        auto trackHeight = 4.0f;

        // Background track
        g.setColour(colors::panelTertiary);
        g.fillRoundedRectangle(trackBounds.getX(), trackY, trackBounds.getWidth(), trackHeight, 2.0f);

        // Filled track
        if (slider.isEnabled()) {
            g.setColour(colors::accent);
            g.fillRoundedRectangle(trackBounds.getX(), trackY, sliderPos - trackBounds.getX(), trackHeight, 2.0f);
        }

        // Thumb pill
        auto thumbWidth = 8.0f;
        auto thumbHeight = 16.0f;
        auto thumbX = std::clamp(sliderPos - thumbWidth * 0.5f,
                                 trackBounds.getX(),
                                 trackBounds.getRight() - thumbWidth);
        auto thumbY = trackBounds.getCentreY() - thumbHeight * 0.5f;

        g.setColour(slider.isMouseOverOrDragging() ? colors::accent : colors::text);
        g.fillRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, 3.0f);
    } else {
        auto trackX = trackBounds.getCentreX() - 2.0f;
        auto trackWidth = 4.0f;

        // Background track
        g.setColour(colors::panelTertiary);
        g.fillRoundedRectangle(trackX, trackBounds.getY(), trackWidth, trackBounds.getHeight(), 2.0f);

        // Filled track
        if (slider.isEnabled()) {
            g.setColour(colors::accent);
            g.fillRoundedRectangle(trackX, sliderPos, trackWidth, trackBounds.getBottom() - sliderPos, 2.0f);
        }

        // Thumb pill
        auto thumbWidth = 16.0f;
        auto thumbHeight = 8.0f;
        auto thumbX = trackBounds.getCentreX() - thumbWidth * 0.5f;
        auto thumbY = std::clamp(sliderPos - thumbHeight * 0.5f,
                                 trackBounds.getY(),
                                 trackBounds.getBottom() - thumbHeight);

        g.setColour(slider.isMouseOverOrDragging() ? colors::accent : colors::text);
        g.fillRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, 3.0f);
    }
}

void ChordSynthLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                 const juce::Colour& backgroundColour,
                                                 bool shouldDrawButtonAsHighlighted,
                                                 bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    auto cornerSize = 7.0f;

    auto baseColour = backgroundColour;
    if (shouldDrawButtonAsDown) {
        baseColour = colors::panelTertiary;
    } else if (shouldDrawButtonAsHighlighted) {
        baseColour = baseColour.brighter(0.08f);
    }

    g.setColour(baseColour);
    g.fillRoundedRectangle(bounds, cornerSize);

    auto outlineColour = button.hasKeyboardFocus(true)
        ? colors::accent
        : (shouldDrawButtonAsHighlighted ? colors::keyBorderHover : colors::line);

    g.setColour(outlineColour);
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
}

void ChordSynthLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height,
                                        [[maybe_unused]] bool isButtonDown,
                                        [[maybe_unused]] int buttonX,
                                        [[maybe_unused]] int buttonY,
                                        [[maybe_unused]] int buttonW,
                                        [[maybe_unused]] int buttonH,
                                        juce::ComboBox& box)
{
    auto cornerSize = 8.0f;
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(0.5f);

    g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(bounds, cornerSize);

    auto outline = box.hasKeyboardFocus(true) ? colors::accent : box.findColour(juce::ComboBox::outlineColourId);
    g.setColour(outline);
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);

    // Draw arrow
    auto arrowZone = juce::Rectangle<float>(static_cast<float>(width - 24), 0.0f, 20.0f, static_cast<float>(height));
    juce::Path path;
    path.startNewSubPath(arrowZone.getX() + 4.0f, arrowZone.getCentreY() - 2.0f);
    path.lineTo(arrowZone.getCentreX(), arrowZone.getCentreY() + 3.0f);
    path.lineTo(arrowZone.getRight() - 4.0f, arrowZone.getCentreY() - 2.0f);

    g.setColour(box.findColour(juce::ComboBox::arrowColourId));
    g.strokePath(path, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void ChordSynthLookAndFeel::drawTickBox(juce::Graphics& g, juce::Component& component,
                                       float x, float y, float w, float h,
                                       bool ticked, bool isEnabled,
                                       [[maybe_unused]] bool shouldDrawButtonAsHighlighted,
                                       [[maybe_unused]] bool shouldDrawButtonAsDown)
{
    juce::Rectangle<float> tickBounds(x, y, w, h);
    auto cornerSize = 4.0f;

    g.setColour(ticked ? colors::accent : colors::panelSecondary);
    g.fillRoundedRectangle(tickBounds, cornerSize);

    auto outline = component.hasKeyboardFocus(true) ? colors::accent : colors::line;
    g.setColour(outline);
    g.drawRoundedRectangle(tickBounds, cornerSize, 1.0f);

    if (ticked) {
        juce::Path tickPath;
        tickPath.startNewSubPath(x + w * 0.22f, y + h * 0.52f);
        tickPath.lineTo(x + w * 0.42f, y + h * 0.72f);
        tickPath.lineTo(x + w * 0.78f, y + h * 0.28f);

        g.setColour(isEnabled ? colors::accentInk : colors::textMuted);
        g.strokePath(tickPath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

void ChordSynthLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                            bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto fontSize = juce::jmin(14.0f, static_cast<float>(button.getHeight()) * 0.75f);
    auto tickWidth = fontSize * 1.1f;

    drawTickBox(g, button, 4.0f, (static_cast<float>(button.getHeight()) - tickWidth) * 0.5f,
                tickWidth, tickWidth,
                button.getToggleState(),
                button.isEnabled(),
                shouldDrawButtonAsHighlighted,
                shouldDrawButtonAsDown);

    g.setColour(button.findColour(juce::ToggleButton::textColourId));
    g.setFont(juce::FontOptions(fontSize));

    if (!button.isEnabled())
        g.setOpacity(0.5f);

    g.drawFittedText(button.getButtonText(),
                     button.getLocalBounds().withTrimmedLeft(juce::roundToInt(tickWidth) + 10)
                                            .withTrimmedRight(2),
                     juce::Justification::centredLeft, 10);
}

juce::Font ChordSynthLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
{
    return juce::FontOptions(std::min(13.0f, static_cast<float>(buttonHeight) * 0.55f));
}

} // namespace chordsynth::ui
