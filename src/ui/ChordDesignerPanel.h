#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "music/HarmonyConfiguration.h"
#include "music/DiatonicChordVoicer.h"
#include "music/VoicedChord.h"

namespace chordsynth::ui {

class ChordDesignerPanel : public juce::Component,
                           private juce::Timer {
public:
    ChordDesignerPanel(
        music::HarmonyConfiguration& config,
        const music::DiatonicChordVoicer& voicer,
        std::function<int()> getTonicCallback,
        std::function<music::Scale()> getScaleCallback,
        std::function<int()> getSceneCallback,
        std::function<void(int scene, int degree)> onSpecSavedCallback);
    ~ChordDesignerPanel() override;

    void setSelectedDegree(int degreeIndex);
    void setSelectedScene(int sceneIndex);
    void setRuleMode(bool isFreeMode);
    void refresh();

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void updatePreview();
    void syncControlsWithSpec(const music::VoicingSpec& spec);
    [[nodiscard]] music::VoicingSpec buildSpecFromControls() const;
    void updateSlashDegreeItems();

    music::HarmonyConfiguration& config;
    const music::DiatonicChordVoicer& voicer;
    std::function<int()> getTonic;
    std::function<music::Scale()> getScale;
    std::function<int()> getScene;
    std::function<void(int scene, int degree)> onSpecSaved;

    int currentDegree{0};
    int currentScene{0};
    bool freeMode{false};

    // Header elements
    juce::Label headerTitleLabel;
    juce::Label headerSubtleLabel;
    juce::Label eyebrowLabel;
    juce::Label chordTitleLabel;
    juce::Label badgeLabel;

    // Control fields
    juce::Label shapeLabel;
    juce::ComboBox shapeComboBox;

    juce::Label qualityLabel;
    juce::ComboBox qualityComboBox;

    juce::Label voicingStyleLabel;
    juce::ComboBox voicingStyleComboBox;

    juce::Label fifthPolicyLabel;
    juce::ComboBox fifthPolicyComboBox;

    juce::Label bassModeLabel;
    juce::ComboBox bassModeComboBox;

    juce::Label slashDegreeLabel;
    juce::ComboBox slashDegreeComboBox;

    juce::Label voiceLeadingLabel;
    juce::ComboBox voiceLeadingComboBox;

    juce::Label inversionLabel;
    juce::ComboBox inversionComboBox;

    juce::Label registerLabel;
    juce::ComboBox registerComboBox;

    // Preview
    juce::Label previewLabel;

    // Action buttons
    juce::TextButton saveButton{"Guardar en este grado"};
    juce::TextButton resetButton{"Restaurar"};

    bool isSavedFlashActive{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordDesignerPanel)
};

} // namespace chordsynth::ui

