#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include "ui/ChordKeyComponent.h"
#include "interaction/ChordPerformanceController.h"
#include "music/DiatonicChordVoicer.h"

namespace chordsynth::ui {

class PerformancePanel : public juce::Component,
                         private juce::Timer {
public:
    PerformancePanel(
        interaction::ChordPerformanceController& controller,
        const music::HarmonyConfiguration& harmonyConfig,
        const music::DiatonicChordVoicer& chordVoicer);
    ~PerformancePanel() override;

    void updateChordKeys();
    void setHeldChordDisplay(const juce::String& chordName, const juce::String& notes);

    void paint(juce::Graphics& g) override;
    void resized() override;

    bool keyPressed(const juce::KeyPress& key) override;
    void focusLost(juce::Component::FocusChangeType cause) override;
    void visibilityChanged() override;

    void selectDegree(int degreeIndex);
    void selectScene(int sceneIndex);

    std::function<void(int degreeIndex)> onDegreeSelected;
    std::function<void(int sceneIndex)> onSceneSelected;

private:
    void timerCallback() override;
    void handleSceneButtonClicked(int sceneIndex);

    interaction::ChordPerformanceController& performanceController;
    const music::HarmonyConfiguration& config;
    const music::DiatonicChordVoicer& voicer;

    // Header "Ahora" components
    juce::Label nowHeaderLabel;
    juce::Label nowChordLabel;
    juce::Label nowNotesLabel;
    juce::ToggleButton liveRevoiceToggle{"Re-voicing del acorde sostenido"};

    // Scene buttons (1..4 / A..D)
    std::array<juce::TextButton, 4> sceneButtons;

    // 7 Chord Keys (Q..U / I..vii°)
    std::array<ChordKeyComponent, 7> chordKeys;

    // Keyboard state tracking for Q..U (indices 0..6)
    std::array<bool, 7> physicalKeysDown{false, false, false, false, false, false, false};

    int currentSelectedDegree{0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerformancePanel)
};

} // namespace chordsynth::ui
