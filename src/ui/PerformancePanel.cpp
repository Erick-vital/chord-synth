#include "PerformancePanel.h"
#include "ChordSynthLookAndFeel.h"
#include "Utf8Text.h"

namespace chordsynth::ui {

namespace {
constexpr std::array<char, 7> shortcutChars{'Q', 'W', 'E', 'R', 'T', 'Y', 'U'};

const char* degreeRomanLabel(music::Scale scale, int degree)
{
    static constexpr std::array<const char*, 7> major{"I", "ii", "iii", "IV", "V", "vi", "vii\xc2\xb0"};
    static constexpr std::array<const char*, 7> naturalMinor{"i", "ii\xc2\xb0", "III", "iv", "v", "VI", "VII"};
    return (scale == music::Scale::naturalMinor ? naturalMinor : major)[static_cast<std::size_t>(degree)];
}
constexpr std::array<const char*, 4> sceneLabels{
    "1  A \xc2\xb7 Triadas",
    "2  B \xc2\xb7 S\xc3\xa9ptimas",
    "3  C \xc2\xb7 Abierto",
    "4  D \xc2\xb7 Inversiones"
};

music::VoicingSpec resolvedSpec(
    const music::HarmonyConfiguration& config,
    const interaction::ChordPerformanceController& controller,
    int scene,
    int degree)
{
    auto spec = config.getSpec(scene, degree);
    if (controller.isDiatonicMode()) {
        spec.qualityRule = music::QualityRule::diatonic;
    }
    return spec;
}
} // namespace

PerformancePanel::PerformancePanel(
    interaction::ChordPerformanceController& controller,
    const music::HarmonyConfiguration& harmonyConfig,
    const music::DiatonicChordVoicer& chordVoicer)
    : performanceController(controller),
      config(harmonyConfig),
      voicer(chordVoicer)
{
    setWantsKeyboardFocus(true);

    // Header labels
    nowHeaderLabel.setText("AHORA", juce::dontSendNotification);
    nowHeaderLabel.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
    nowHeaderLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(nowHeaderLabel);

    nowChordLabel.setText("Listo para tocar", juce::dontSendNotification);
    nowChordLabel.setFont(juce::FontOptions(20.0f).withStyle("Bold"));
    nowChordLabel.setColour(juce::Label::textColourId, colors::text);
    addAndMakeVisible(nowChordLabel);

    nowNotesLabel.setText("Pulsa una tecla", juce::dontSendNotification);
    nowNotesLabel.setFont(juce::FontOptions(12.0f).withStyle("Bold"));
    nowNotesLabel.setColour(juce::Label::textColourId, colors::cyan);
    addAndMakeVisible(nowNotesLabel);

    // Live revoice toggle
    liveRevoiceToggle.setToggleState(performanceController.getLiveRevoice(), juce::dontSendNotification);
    liveRevoiceToggle.onClick = [this]() {
        performanceController.setLiveRevoice(liveRevoiceToggle.getToggleState());
    };
    addAndMakeVisible(liveRevoiceToggle);

    // Scene buttons
    for (int i = 0; i < 4; ++i) {
        sceneButtons[static_cast<std::size_t>(i)].setButtonText(utf8(sceneLabels[static_cast<std::size_t>(i)]));
        sceneButtons[static_cast<std::size_t>(i)].setComponentID("scene-" + juce::String(i));
        sceneButtons[static_cast<std::size_t>(i)].setClickingTogglesState(false);
        sceneButtons[static_cast<std::size_t>(i)].onClick = [this, i]() {
            handleSceneButtonClicked(i);
        };
        addAndMakeVisible(sceneButtons[static_cast<std::size_t>(i)]);
    }

    // Chord keys
    for (int i = 0; i < 7; ++i) {
        auto& key = chordKeys[static_cast<std::size_t>(i)];
        key.setDegreeIndex(i);
        key.setComponentID("degree-" + juce::String(i));
        key.setDegreeLabel(utf8(degreeRomanLabel(performanceController.getScale(), i)));
        key.setKeycapShortcut(juce::String::charToString(shortcutChars[static_cast<std::size_t>(i)]));

        key.onPress = [this](int degree) {
            performanceController.pressDegree(degree);
            selectDegree(degree);
            auto active = performanceController.getActiveChord();
            if (active.has_value()) {
                const auto& chord = voicer.voiceChord(
                    performanceController.getTonic(),
                    active->degree,
                    resolvedSpec(
                        config, performanceController, performanceController.getScene(), active->degree),
                    performanceController.getScale());

                juce::String notesStr;
                for (int n = 0; n < chord.notes.size(); ++n) {
                    if (n > 0) notesStr << "  ";
                    int midiVal = chord.notes[static_cast<std::size_t>(n)];
                    static constexpr const char* pNames[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
                    int pc = ((midiVal % 12) + 12) % 12;
                    int oct = (midiVal / 12) - 1;
                    notesStr << pNames[pc] << oct;
                }
                setHeldChordDisplay(utf8(degreeRomanLabel(performanceController.getScale(), degree)) + utf8(" \xc2\xb7 ") + chord.label, notesStr);
            }
        };

        key.onRelease = [this](int degree) {
            auto active = performanceController.getActiveChord();
            if (active.has_value() && active->degree == degree) {
                performanceController.releaseActiveChord();
                setHeldChordDisplay("Listo para tocar", "Pulsa una tecla");
            }
        };

        key.onSelect = [this](int degree) {
            selectDegree(degree);
        };

        addAndMakeVisible(key);
    }

    selectScene(performanceController.getScene());
    updateChordKeys();
    selectDegree(0);

    startTimerHz(30);
}

PerformancePanel::~PerformancePanel()
{
    stopTimer();
    performanceController.allNotesOff();
}

void PerformancePanel::setHeldChordDisplay(const juce::String& chordName, const juce::String& notes)
{
    nowChordLabel.setText(chordName, juce::dontSendNotification);
    nowNotesLabel.setText(notes, juce::dontSendNotification);
}

void PerformancePanel::selectDegree(int degreeIndex)
{
    if (degreeIndex < 0 || degreeIndex > 6)
        return;

    currentSelectedDegree = degreeIndex;
    for (int i = 0; i < 7; ++i) {
        chordKeys[static_cast<std::size_t>(i)].setSelected(i == degreeIndex);
    }

    if (onDegreeSelected)
        onDegreeSelected(degreeIndex);
}

void PerformancePanel::selectScene(int sceneIndex)
{
    if (sceneIndex < 0 || sceneIndex > 3)
        return;

    performanceController.setScene(sceneIndex);

    for (int i = 0; i < 4; ++i) {
        if (i == sceneIndex) {
            sceneButtons[static_cast<std::size_t>(i)].setColour(juce::TextButton::buttonColourId, colors::accent.withAlpha(0.12f));
            sceneButtons[static_cast<std::size_t>(i)].setColour(juce::TextButton::textColourOnId, colors::accent);
            sceneButtons[static_cast<std::size_t>(i)].setColour(juce::TextButton::textColourOffId, colors::accent);
        } else {
            sceneButtons[static_cast<std::size_t>(i)].setColour(juce::TextButton::buttonColourId, colors::panelSecondary);
            sceneButtons[static_cast<std::size_t>(i)].setColour(juce::TextButton::textColourOnId, colors::textMuted);
            sceneButtons[static_cast<std::size_t>(i)].setColour(juce::TextButton::textColourOffId, colors::textMuted);
        }
    }

    updateChordKeys();

    auto active = performanceController.getActiveChord();
    if (active.has_value()) {
        const auto& chord = voicer.voiceChord(
            performanceController.getTonic(),
            active->degree,
            resolvedSpec(
                config, performanceController, performanceController.getScene(), active->degree),
            performanceController.getScale());

        juce::String notesStr;
        for (int n = 0; n < chord.notes.size(); ++n) {
            if (n > 0) notesStr << "  ";
            int midiVal = chord.notes[static_cast<std::size_t>(n)];
            static constexpr const char* pNames[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
            int pc = ((midiVal % 12) + 12) % 12;
            int oct = (midiVal / 12) - 1;
            notesStr << pNames[pc] << oct;
        }
        setHeldChordDisplay(utf8(degreeRomanLabel(performanceController.getScale(), active->degree)) + utf8(" \xc2\xb7 ") + chord.label, notesStr);
    }

    if (onSceneSelected)
        onSceneSelected(sceneIndex);
}

void PerformancePanel::handleSceneButtonClicked(int sceneIndex)
{
    selectScene(sceneIndex);
}

void PerformancePanel::updateChordKeys()
{
    const int tonic = performanceController.getTonic();
    const int currentScene = performanceController.getScene();

    for (int deg = 0; deg < 7; ++deg) {
        const auto spec = resolvedSpec(config, performanceController, currentScene, deg);
        const auto chord = voicer.voiceChord(tonic, deg, spec, performanceController.getScale());

        auto& key = chordKeys[static_cast<std::size_t>(deg)];
        key.setDegreeLabel(utf8(degreeRomanLabel(performanceController.getScale(), deg)));
        key.setChordName(chord.label);

        juce::String notesStr;
        for (int n = 0; n < chord.notes.size(); ++n) {
            if (n > 0) notesStr << utf8(" \xc2\xb7 ");
            int midiVal = chord.notes[static_cast<std::size_t>(n)];
            static constexpr const char* pNames[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
            int pc = ((midiVal % 12) + 12) % 12;
            int oct = (midiVal / 12) - 1;
            notesStr << pNames[pc] << oct;
        }
        key.setChordNotes(notesStr);
    }
}

void PerformancePanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(colors::panel);
    g.fillRoundedRectangle(bounds, 14.0f);
    g.setColour(colors::line);
    g.drawRoundedRectangle(bounds, 14.0f, 1.0f);

    // Separator line between header and scene strip
    g.setColour(colors::line);
    g.drawLine(0.0f, 54.0f, bounds.getWidth(), 54.0f, 1.0f);
}

void PerformancePanel::resized()
{
    auto bounds = getLocalBounds();

    // 1. Header (54 px high)
    auto headerArea = bounds.removeFromTop(54).reduced(14, 8);
    auto toggleArea = headerArea.removeFromRight(230);
    liveRevoiceToggle.setBounds(toggleArea);

    nowHeaderLabel.setBounds(headerArea.removeFromLeft(50));
    nowChordLabel.setBounds(headerArea.removeFromLeft(140));
    nowNotesLabel.setBounds(headerArea);

    // 2. Scene strip (44 px high)
    auto sceneArea = bounds.removeFromTop(44).reduced(14, 4);
    int sceneWidth = (sceneArea.getWidth() - (3 * 8)) / 4;
    for (int i = 0; i < 4; ++i) {
        sceneButtons[static_cast<std::size_t>(i)].setBounds(
            sceneArea.getX() + i * (sceneWidth + 8),
            sceneArea.getY(),
            sceneWidth,
            sceneArea.getHeight());
    }

    // 3. Chord keys (occupies remaining height)
    auto keysArea = bounds.reduced(14, 10);
    int keyWidth = (keysArea.getWidth() - (6 * 8)) / 7;
    for (int i = 0; i < 7; ++i) {
        chordKeys[static_cast<std::size_t>(i)].setBounds(
            keysArea.getX() + i * (keyWidth + 8),
            keysArea.getY(),
            keyWidth,
            keysArea.getHeight());
    }
}

bool PerformancePanel::keyPressed(const juce::KeyPress& key)
{
    // If a subcomponent or text editor has focus, do not swallow shortcuts
    auto* focusComp = juce::Component::getCurrentlyFocusedComponent();
    if (focusComp != nullptr && (dynamic_cast<juce::TextEditor*>(focusComp) != nullptr || dynamic_cast<juce::ComboBox*>(focusComp) != nullptr)) {
        return false;
    }

    // Scene switching with '1'..'4'
    auto keyChar = static_cast<char>(key.getKeyCode());
    if (keyChar >= '1' && keyChar <= '4') {
        selectScene(keyChar - '1');
        return true;
    }

    return false;
}

void PerformancePanel::timerCallback()
{
    // If window lost focus, release any active chords
    auto* topLevel = getTopLevelComponent();
    if (topLevel != nullptr && !topLevel->hasKeyboardFocus(true) && !hasKeyboardFocus(true)) {
        bool hadKey = false;
        for (bool down : physicalKeysDown) {
            if (down) hadKey = true;
        }
        if (hadKey) {
            physicalKeysDown.fill(false);
            for (int i = 0; i < 7; ++i) {
                chordKeys[static_cast<std::size_t>(i)].setPressed(false);
            }
            performanceController.releaseActiveChord();
            setHeldChordDisplay("Listo para tocar", "Pulsa una tecla");
        }
        return;
    }

    // Don't capture Q..U if an editor/combobox has focus
    auto* focusComp = juce::Component::getCurrentlyFocusedComponent();
    if (focusComp != nullptr && (dynamic_cast<juce::TextEditor*>(focusComp) != nullptr || dynamic_cast<juce::ComboBox*>(focusComp) != nullptr)) {
        return;
    }

    for (int i = 0; i < 7; ++i) {
        int keyCode = shortcutChars[static_cast<std::size_t>(i)];
        bool isDown = juce::KeyPress::isKeyCurrentlyDown(keyCode) ||
                      juce::KeyPress::isKeyCurrentlyDown(keyCode + ('a' - 'A'));

        if (isDown != physicalKeysDown[static_cast<std::size_t>(i)]) {
            physicalKeysDown[static_cast<std::size_t>(i)] = isDown;
            chordKeys[static_cast<std::size_t>(i)].setPressed(isDown);
        }
    }
}

void PerformancePanel::focusLost(juce::Component::FocusChangeType)
{
    physicalKeysDown.fill(false);
    for (int i = 0; i < 7; ++i) {
        chordKeys[static_cast<std::size_t>(i)].setPressed(false);
    }
    performanceController.allNotesOff();
    setHeldChordDisplay("Listo para tocar", "Pulsa una tecla");
}

void PerformancePanel::visibilityChanged()
{
    if (!isVisible()) {
        physicalKeysDown.fill(false);
        for (int i = 0; i < 7; ++i) {
            chordKeys[static_cast<std::size_t>(i)].setPressed(false);
        }
        performanceController.allNotesOff();
        setHeldChordDisplay("Listo para tocar", "Pulsa una tecla");
    }
}

} // namespace chordsynth::ui
