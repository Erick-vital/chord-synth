#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <string>
#include "ui/Utf8Text.h"

namespace chordsynth::ui {

class ChordKeyComponent : public juce::Component {
public:
    ChordKeyComponent();
    ~ChordKeyComponent() override;

    void setDegreeIndex(int index) noexcept { degreeIndex = index; }
    [[nodiscard]] int getDegreeIndex() const noexcept { return degreeIndex; }

    void setDegreeLabel(const juce::String& text);
    void setChordName(const juce::String& text);
    void setChordNotes(const juce::String& text);
    void setKeycapShortcut(const juce::String& text);

    void setSelected(bool shouldBeSelected);
    [[nodiscard]] bool isSelected() const noexcept { return selected; }

    void setPressed(bool shouldBePressed);
    [[nodiscard]] bool isPressed() const noexcept { return pressed; }

    std::function<void(int degreeIndex)> onPress;
    std::function<void(int degreeIndex)> onRelease;
    std::function<void(int degreeIndex)> onSelect;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void focusGained(juce::Component::FocusChangeType cause) override;
    void focusLost(juce::Component::FocusChangeType cause) override;

private:
    void triggerPress();
    void triggerRelease();

    int degreeIndex{0};
    juce::String degreeLabel{"I"};
    juce::String chordName{"C"};
    juce::String chordNotes{utf8("C \xc2\xb7 E \xc2\xb7 G")};
    juce::String keycapShortcut{"Q"};

    bool selected{false};
    bool pressed{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordKeyComponent)
};

} // namespace chordsynth::ui
