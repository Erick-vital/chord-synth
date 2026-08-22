#include "ChordKeyComponent.h"
#include "ChordSynthLookAndFeel.h"
#include <algorithm>

namespace chordsynth::ui {

ChordKeyComponent::ChordKeyComponent()
{
    setWantsKeyboardFocus(true);
    setRepaintsOnMouseActivity(true);
}

ChordKeyComponent::~ChordKeyComponent()
{
    triggerRelease();
}

void ChordKeyComponent::setDegreeLabel(const juce::String& text)
{
    if (degreeLabel != text) {
        degreeLabel = text;
        repaint();
    }
}

void ChordKeyComponent::setChordName(const juce::String& text)
{
    if (chordName != text) {
        chordName = text;
        repaint();
    }
}

void ChordKeyComponent::setChordNotes(const juce::String& text)
{
    if (chordNotes != text) {
        chordNotes = text;
        repaint();
    }
}

void ChordKeyComponent::setKeycapShortcut(const juce::String& text)
{
    if (keycapShortcut != text) {
        keycapShortcut = text;
        repaint();
    }
}

void ChordKeyComponent::setSelected(bool shouldBeSelected)
{
    if (selected != shouldBeSelected) {
        selected = shouldBeSelected;
        repaint();
    }
}

void ChordKeyComponent::setPressed(bool shouldBePressed)
{
    if (shouldBePressed) {
        triggerPress();
    } else {
        triggerRelease();
    }
}

void ChordKeyComponent::triggerPress()
{
    if (!pressed) {
        pressed = true;
        repaint();
        if (onPress)
            onPress(degreeIndex);
    }
}

void ChordKeyComponent::triggerRelease()
{
    if (pressed) {
        pressed = false;
        repaint();
        if (onRelease)
            onRelease(degreeIndex);
    }
}

void ChordKeyComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto isHover = isMouseOver();
    auto isFocus = hasKeyboardFocus(true);

    // Visual depression when pressed
    float topOffset = pressed ? 4.0f : 0.0f;
    auto keyBounds = bounds.withTrimmedTop(topOffset).withTrimmedBottom(4.0f - topOffset).reduced(1.0f);
    auto cornerSize = 12.0f;

    // Draw shadow under the key
    if (!pressed) {
        g.setColour(colors::keyShadow);
        g.fillRoundedRectangle(keyBounds.translated(0.0f, 4.0f), cornerSize);
    } else {
        g.setColour(colors::keyShadow.withAlpha(0.4f));
        g.fillRoundedRectangle(keyBounds.translated(0.0f, 1.0f), cornerSize);
    }

    // Key background
    if (pressed) {
        g.setColour(colors::accent.withAlpha(0.15f));
        g.fillRoundedRectangle(keyBounds, cornerSize);
        g.setColour(colors::panelTertiary);
        g.drawRoundedRectangle(keyBounds, cornerSize, 1.0f);
    } else {
        juce::ColourGradient grad(
            isHover ? colors::keyTop.brighter(0.08f) : colors::keyTop,
            keyBounds.getX(), keyBounds.getY(),
            isHover ? colors::keyBottom.brighter(0.08f) : colors::keyBottom,
            keyBounds.getX(), keyBounds.getBottom(),
            false
        );
        g.setGradientFill(grad);
        g.fillRoundedRectangle(keyBounds, cornerSize);
    }

    // Key border / focus outline
    juce::Colour borderCol = colors::keyBorder;
    if (pressed) {
        borderCol = colors::accent;
    } else if (isFocus) {
        borderCol = colors::accent;
    } else if (selected) {
        borderCol = colors::cyan;
    } else if (isHover) {
        borderCol = colors::keyBorderHover;
    }

    g.setColour(borderCol);
    g.drawRoundedRectangle(keyBounds, cornerSize, pressed ? 1.5f : 1.0f);

    // Inner content area
    auto contentBounds = keyBounds.reduced(10.0f, 12.0f);

    // 1. Degree Label (top-left)
    g.setColour(colors::textMuted);
    g.setFont(juce::FontOptions(12.0f).withStyle("Bold"));
    g.drawText(degreeLabel, contentBounds.removeFromTop(16.0f), juce::Justification::topLeft, false);

    // 2. Keycap Shortcut (top-right)
    auto keycapArea = keyBounds.reduced(8.0f, 8.0f).removeFromTop(18.0f).removeFromRight(24.0f);
    g.setColour(colors::panelSecondary);
    g.fillRoundedRectangle(keycapArea, 4.0f);
    g.setColour(colors::keyBorder);
    g.drawRoundedRectangle(keycapArea, 4.0f, 1.0f);
    g.setColour(colors::textMuted);
    g.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
    g.drawText(keycapShortcut, keycapArea, juce::Justification::centred, false);

    // 3. Chord Name (middle-large)
    auto nameArea = contentBounds.removeFromTop(contentBounds.getHeight() - 20.0f);
    g.setColour(pressed ? colors::accent : colors::text);
    g.setFont(juce::FontOptions(24.0f).withStyle("Bold"));
    g.drawText(chordName, nameArea, juce::Justification::centredLeft, true);

    // 4. Chord Notes (bottom)
    g.setColour(colors::cyan);
    g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    g.drawText(chordNotes, contentBounds, juce::Justification::bottomLeft, true);
}

void ChordKeyComponent::resized()
{
}

void ChordKeyComponent::mouseDown(const juce::MouseEvent&)
{
    if (onSelect)
        onSelect(degreeIndex);
    triggerPress();
}

void ChordKeyComponent::mouseUp(const juce::MouseEvent&)
{
    triggerRelease();
}

void ChordKeyComponent::mouseExit(const juce::MouseEvent&)
{
    // If mouse was dragging/pressed and exits without capture, release
    if (!isMouseButtonDown()) {
        triggerRelease();
    }
}

void ChordKeyComponent::focusGained(juce::Component::FocusChangeType)
{
    repaint();
}

void ChordKeyComponent::focusLost(juce::Component::FocusChangeType)
{
    triggerRelease();
    repaint();
}

} // namespace chordsynth::ui
