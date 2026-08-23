#include "ChordColorPanel.h"
#include "ChordSynthLookAndFeel.h"
#include "ui/Utf8Text.h"
#include "parameters/ParameterIds.h"
#include <algorithm>

namespace chordsynth::ui {

namespace {

constexpr std::array<char, 8> colorShortcutChars{'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K'};

const char* basicSlotNames[8] = {
    "Flip (M/m)", "Dominante 7", "Séptima Color", "add9",
    "sus4", "sus2", "6/9", "Disminuido"
};

const char* loFiSlotNames[8] = {
    "Novena (9)", "add9", "6/9", "Oncena (11)",
    "Open 9", "Rootless 7", "Warm 13", "Nearest Open"
};

const char* spiceSlotNames[8] = {
    "Dominante 7", "Disminuido 7", "sus4 Tensión", "Dominante 9",
    "Dominante 13", "Menor 9 Tens", "Rootless 9", "Open 11"
};

[[nodiscard]] const char* degreeRomanLabel(music::Scale scale, int degree) noexcept
{
    static constexpr std::array<const char*, 7> major{"I", "ii", "iii", "IV", "V", "vi", "vii\xc2\xb0"};
    static constexpr std::array<const char*, 7> naturalMinor{"i", "ii\xc2\xb0", "III", "iv", "v", "VI", "VII"};
    const auto clampedDegree = static_cast<std::size_t>(std::clamp(degree, 0, 6));
    return (scale == music::Scale::naturalMinor ? naturalMinor : major)[clampedDegree];
}

[[nodiscard]] const char* sceneLetter(int sceneIndex) noexcept
{
    static constexpr std::array<const char*, 4> letters{"A", "B", "C", "D"};
    return letters[static_cast<std::size_t>(std::clamp(sceneIndex, 0, 3))];
}

} // namespace

ChordColorPanel::ChordColorPanel(
    interaction::ChordPerformanceController& controller,
    music::HarmonyConfiguration& harmonyConfig,
    const music::DiatonicChordVoicer& chordVoicer,
    parameters::AudioProcessorValueTreeState* apvts)
    : performanceController(controller),
      config(harmonyConfig),
      voicer(chordVoicer)
{
    setWantsKeyboardFocus(true);

    // Header label
    headerLabel.setText(utf8("COLOR / TRANSFORMACI\xc3\x93N"), juce::dontSendNotification);
    headerLabel.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
    headerLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(headerLabel);

    // MIDI Performance Toggle
    midiPerfToggle.setButtonText("MIDI Perf");
    midiPerfToggle.setComponentID("performance-midi-toggle");
    midiPerfToggle.setTooltip("Habilita mapeo de notas MIDI 36-42 a grados I-VII y CC 20-27 a colores");
    addAndMakeVisible(midiPerfToggle);

    // Palette combo
    paletteComboBox.setComponentID("palette-select");
    paletteComboBox.addItem(utf8("B\xc3\xa1sica"), 1);
    paletteComboBox.addItem(utf8("Lo\xe2\x80\x91" "Fi"), 2);
    paletteComboBox.addItem("Spice", 3);
    paletteComboBox.setSelectedId(2, juce::dontSendNotification); // Default Lo-Fi
    paletteComboBox.onChange = [this]() {
        int id = paletteComboBox.getSelectedId();
        if (id == 1) setSelectedPalette(interaction::TransformPalette::basic);
        else if (id == 2) setSelectedPalette(interaction::TransformPalette::loFi);
        else if (id == 3) setSelectedPalette(interaction::TransformPalette::spice);
    };
    addAndMakeVisible(paletteComboBox);

    if (apvts != nullptr) {
        midiPerfAttachment = std::make_unique<parameters::AudioProcessorValueTreeState::ButtonAttachment>(
            *apvts, parameters::ids::performanceMidiEnabled, midiPerfToggle);
        paletteAttachment = std::make_unique<parameters::AudioProcessorValueTreeState::ComboBoxAttachment>(
            *apvts, parameters::ids::transformPalette, paletteComboBox);
    }

    // 8 Color buttons
    for (int i = 0; i < 8; ++i) {
        auto& btn = colorButtons[static_cast<std::size_t>(i)];
        btn.setComponentID("chord-color-" + juce::String(i));
        btn.setClickingTogglesState(false);

        // Custom mouse callbacks for press & hold
        btn.addMouseListener(this, false);

        addAndMakeVisible(btn);
    }
    updateSlotLabels();

    // Commit button
    commitButton.setComponentID("commit-transform-button");
    commitButton.setButtonText("Fijar en grado");
    commitButton.setColour(juce::TextButton::buttonColourId, colors::panelSecondary);
    commitButton.setColour(juce::TextButton::textColourOffId, colors::accent);
    commitButton.setEnabled(false);
    commitButton.onClick = [this]() {
        handleCommitClicked();
    };
    addAndMakeVisible(commitButton);

    // Transform info label
    transformInfoLabel.setText("", juce::dontSendNotification);
    transformInfoLabel.setFont(juce::FontOptions(11.0f));
    transformInfoLabel.setColour(juce::Label::textColourId, colors::cyan);
    addAndMakeVisible(transformInfoLabel);

    // Feedback label
    feedbackLabel.setText("", juce::dontSendNotification);
    feedbackLabel.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    feedbackLabel.setColour(juce::Label::textColourId, colors::amber);
    addAndMakeVisible(feedbackLabel);

    startTimerHz(30);
}

ChordColorPanel::~ChordColorPanel()
{
    stopTimer();
    releaseAllColorKeys();
}

void ChordColorPanel::setSelectedPalette(interaction::TransformPalette palette)
{
    currentPalette = palette;
    int id = 2;
    if (palette == interaction::TransformPalette::basic) id = 1;
    else if (palette == interaction::TransformPalette::loFi) id = 2;
    else if (palette == interaction::TransformPalette::spice) id = 3;
    paletteComboBox.setSelectedId(id, juce::dontSendNotification);

    updateSlotLabels();
}

void ChordColorPanel::updateSlotLabels()
{
    const char** names = loFiSlotNames;
    if (currentPalette == interaction::TransformPalette::basic) names = basicSlotNames;
    else if (currentPalette == interaction::TransformPalette::spice) names = spiceSlotNames;

    for (int i = 0; i < 8; ++i) {
        juce::String text = juce::String::charToString(colorShortcutChars[static_cast<std::size_t>(i)]) +
                            " \xc2\xb7 " + utf8(names[i]);
        colorButtons[static_cast<std::size_t>(i)].setButtonText(text);
    }
}

void ChordColorPanel::triggerColorPress(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= 8) return;
    auto slot = static_cast<interaction::TransformSlot>(slotIndex);
    performanceController.beginTransform(currentPalette, slot);
    updateTransformState();
}

void ChordColorPanel::triggerColorRelease(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= 8) return;
    performanceController.endTransform();
    updateTransformState();
}

void ChordColorPanel::releaseAllColorKeys()
{
    physicalKeysDown.fill(false);
    for (auto& btn : colorButtons) {
        btn.setState(juce::Button::buttonNormal);
    }
    performanceController.endTransform();
    updateTransformState();
}

void ChordColorPanel::handleCommitClicked()
{
    if (!performanceController.hasActiveTransform() || !performanceController.getActiveChord().has_value()) {
        feedbackLabel.setColour(juce::Label::textColourId, colors::danger);
        feedbackLabel.setText("Sin transformacion activa", juce::dontSendNotification);
        feedbackTimerTicks = 60; // 2 seconds
        return;
    }

    int scene = performanceController.getScene();
    int degree = performanceController.getActiveChord()->degree;

    if (performanceController.commitActiveTransform(config)) {
        feedbackLabel.setColour(juce::Label::textColourId, colors::accent);
        juce::String successMsg = utf8("Fijado en escena ") + juce::String(sceneLetter(scene)) +
                                  utf8(" \xc2\xb7 grado ") +
                                  utf8(degreeRomanLabel(performanceController.getScale(), degree));
        feedbackLabel.setText(successMsg, juce::dontSendNotification);
        feedbackTimerTicks = 90; // 3 seconds

        if (onTransformCommitted) {
            onTransformCommitted();
        }
    } else {
        feedbackLabel.setColour(juce::Label::textColourId, colors::danger);
        feedbackLabel.setText("Error al fijar transformacion", juce::dontSendNotification);
        feedbackTimerTicks = 60;
    }
    updateTransformState();
}

void ChordColorPanel::updateTransformState()
{
    bool hasTransform = performanceController.hasActiveTransform() && performanceController.getActiveChord().has_value();
    commitButton.setEnabled(hasTransform);

    auto activeTrans = performanceController.getActiveTransform();
    for (int i = 0; i < 8; ++i) {
        auto& btn = colorButtons[static_cast<std::size_t>(i)];
        bool isActive = activeTrans.has_value() &&
                        activeTrans->first == currentPalette &&
                        static_cast<int>(activeTrans->second) == i;
        if (isActive) {
            btn.setColour(juce::TextButton::buttonColourId, colors::accent.withAlpha(0.2f));
            btn.setColour(juce::TextButton::textColourOffId, colors::accent);
        } else {
            btn.setColour(juce::TextButton::buttonColourId, colors::panelSecondary);
            btn.setColour(juce::TextButton::textColourOffId, colors::text);
        }
    }

    if (hasTransform) {
        auto specOpt = performanceController.transformedSpecForActiveDegree();
        if (specOpt.has_value()) {
            const int tonic = performanceController.getTonic();
            const int degree = performanceController.getActiveChord()->degree;
            const auto voiced = voicer.voiceChord(tonic, degree, *specOpt, performanceController.getScale());

            juce::String notesStr;
            for (int n = 0; n < voiced.notes.size(); ++n) {
                if (n > 0) notesStr << " ";
                int midiVal = voiced.notes[static_cast<std::size_t>(n)];
                static constexpr const char* pNames[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
                int pc = ((midiVal % 12) + 12) % 12;
                int oct = (midiVal / 12) - 1;
                notesStr << pNames[pc] << oct;
            }
            juce::String infoText = juce::String(voiced.label) + " (" + notesStr + ")";
            transformInfoLabel.setText(infoText, juce::dontSendNotification);
        } else {
            transformInfoLabel.setText("", juce::dontSendNotification);
        }
    } else {
        transformInfoLabel.setText("", juce::dontSendNotification);
    }
}

void ChordColorPanel::timerCallback()
{
    // Check feedback timeout
    if (feedbackTimerTicks > 0) {
        if (--feedbackTimerTicks == 0) {
            feedbackLabel.setText("", juce::dontSendNotification);
        }
    }

    // Check keyboard shortcuts A S D F G H J K
    auto* topLevel = getTopLevelComponent();
    if (topLevel != nullptr && !topLevel->hasKeyboardFocus(true) && !hasKeyboardFocus(true)) {
        bool hadKey = false;
        for (bool down : physicalKeysDown) {
            if (down) hadKey = true;
        }
        if (hadKey) {
            releaseAllColorKeys();
        }
        return;
    }

    auto* focusComp = juce::Component::getCurrentlyFocusedComponent();
    if (focusComp != nullptr && (dynamic_cast<juce::TextEditor*>(focusComp) != nullptr || dynamic_cast<juce::ComboBox*>(focusComp) != nullptr)) {
        return;
    }

    for (int i = 0; i < 8; ++i) {
        int keyCode = colorShortcutChars[static_cast<std::size_t>(i)];
        bool isDown = juce::KeyPress::isKeyCurrentlyDown(keyCode) ||
                      juce::KeyPress::isKeyCurrentlyDown(keyCode + ('a' - 'A'));

        if (isDown != physicalKeysDown[static_cast<std::size_t>(i)]) {
            physicalKeysDown[static_cast<std::size_t>(i)] = isDown;
            if (isDown) {
                triggerColorPress(i);
            } else {
                triggerColorRelease(i);
            }
        }
    }

    updateTransformState();
}

void ChordColorPanel::focusLost(juce::Component::FocusChangeType)
{
    releaseAllColorKeys();
}

void ChordColorPanel::visibilityChanged()
{
    if (!isVisible()) {
        releaseAllColorKeys();
    }
}

void ChordColorPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(colors::panel);
    g.fillRoundedRectangle(bounds, 12.0f);
    g.setColour(colors::line);
    g.drawRoundedRectangle(bounds, 12.0f, 1.0f);
}

void ChordColorPanel::resized()
{
    auto bounds = getLocalBounds().reduced(10, 8);

    // Top row: Header (left), MIDI perf toggle (left-center), Palette combo (center-left), Info label (center-right), Commit button & Feedback (right)
    auto topRow = bounds.removeFromTop(28);
    headerLabel.setBounds(topRow.removeFromLeft(130));
    midiPerfToggle.setBounds(topRow.removeFromLeft(85));
    topRow.removeFromLeft(6);
    paletteComboBox.setBounds(topRow.removeFromLeft(105));
    topRow.removeFromLeft(8);

    commitButton.setBounds(topRow.removeFromRight(110));
    topRow.removeFromRight(8);
    feedbackLabel.setBounds(topRow.removeFromRight(150));
    transformInfoLabel.setBounds(topRow);

    bounds.removeFromTop(6);

    // Buttons grid: 8 color buttons in 1 row of 8 (or 2 rows of 4 if height allows)
    int btnGap = 6;
    int btnWidth = (bounds.getWidth() - (7 * btnGap)) / 8;
    for (int i = 0; i < 8; ++i) {
        colorButtons[static_cast<std::size_t>(i)].setBounds(
            bounds.getX() + i * (btnWidth + btnGap),
            bounds.getY(),
            btnWidth,
            bounds.getHeight());
    }
}

} // namespace chordsynth::ui
