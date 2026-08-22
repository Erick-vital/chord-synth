#include "ChordDesignerPanel.h"
#include "ChordSynthLookAndFeel.h"
#include "Utf8Text.h"

namespace chordsynth::ui {

namespace {
const char* degreeRomanLabel(music::Scale scale, int degree)
{
    static constexpr std::array<const char*, 7> major{"I", "ii", "iii", "IV", "V", "vi", "vii\xc2\xb0"};
    static constexpr std::array<const char*, 7> naturalMinor{"i", "ii\xc2\xb0", "III", "iv", "v", "VI", "VII"};
    return (scale == music::Scale::naturalMinor ? naturalMinor : major)[static_cast<std::size_t>(degree)];
}

constexpr std::array<const char*, 12> pitchNames{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
} // namespace

ChordDesignerPanel::ChordDesignerPanel(
    music::HarmonyConfiguration& harmonyConfig,
    const music::DiatonicChordVoicer& chordVoicer,
    std::function<int()> getTonicCallback,
    std::function<music::Scale()> getScaleCallback,
    std::function<int()> getSceneCallback,
    std::function<void(int scene, int degree)> onSpecSavedCallback)
    : config(harmonyConfig),
      voicer(chordVoicer),
      getTonic(std::move(getTonicCallback)),
      getScale(std::move(getScaleCallback)),
      getScene(std::move(getSceneCallback)),
      onSpecSaved(std::move(onSpecSavedCallback))
{
    // Panel Header
    headerTitleLabel.setText(utf8("Dise\xc3\xb1""ar acorde"), juce::dontSendNotification);
    headerTitleLabel.setFont(juce::FontOptions(14.0f).withStyle("Bold"));
    headerTitleLabel.setColour(juce::Label::textColourId, colors::text);
    addAndMakeVisible(headerTitleLabel);

    headerSubtleLabel.setText("Por grado", juce::dontSendNotification);
    headerSubtleLabel.setFont(juce::FontOptions(11.0f));
    headerSubtleLabel.setColour(juce::Label::textColourId, colors::textMuted);
    headerSubtleLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(headerSubtleLabel);

    // Selected Chord Info
    eyebrowLabel.setText("GRADO SELECCIONADO", juce::dontSendNotification);
    eyebrowLabel.setFont(juce::FontOptions(9.0f).withStyle("Bold"));
    eyebrowLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(eyebrowLabel);

    chordTitleLabel.setText(utf8("I \xc2\xb7 C"), juce::dontSendNotification);
    chordTitleLabel.setFont(juce::FontOptions(18.0f).withStyle("Bold"));
    chordTitleLabel.setColour(juce::Label::textColourId, colors::text);
    addAndMakeVisible(chordTitleLabel);

    badgeLabel.setText(utf8("DIAT\xc3\x93NICO"), juce::dontSendNotification);
    badgeLabel.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
    badgeLabel.setColour(juce::Label::textColourId, colors::cyan);
    badgeLabel.setColour(juce::Label::backgroundColourId, colors::cyan.withAlpha(0.12f));
    badgeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(badgeLabel);

    // Field 1: Calidad
    qualityLabel.setText("CALIDAD", juce::dontSendNotification);
    qualityLabel.setFont(juce::FontOptions(9.0f).withStyle("Bold"));
    qualityLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(qualityLabel);

    qualityComboBox.addItem(utf8("Seg\xc3\xba""n escala"), 1);
    qualityComboBox.addItem("Mayor", 2);
    qualityComboBox.addItem("Menor", 3);
    qualityComboBox.addItem("Disminuido", 4);
    qualityComboBox.setSelectedId(1, juce::dontSendNotification);
    qualityComboBox.onChange = [this]() { updatePreview(); };
    addAndMakeVisible(qualityComboBox);

    // Field 2: Extensión
    extensionLabel.setText(utf8("EXTENSI\xc3\x93N"), juce::dontSendNotification);
    extensionLabel.setFont(juce::FontOptions(9.0f).withStyle("Bold"));
    extensionLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(extensionLabel);

    extensionComboBox.addItem("Triada", 1);
    extensionComboBox.addItem(utf8("S\xc3\xa9ptima"), 2);
    extensionComboBox.setSelectedId(1, juce::dontSendNotification);
    extensionComboBox.onChange = [this]() { updatePreview(); };
    addAndMakeVisible(extensionComboBox);

    // Field 3: Inversión
    inversionLabel.setText(utf8("INVERSI\xc3\x93N"), juce::dontSendNotification);
    inversionLabel.setFont(juce::FontOptions(9.0f).withStyle("Bold"));
    inversionLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(inversionLabel);

    inversionComboBox.addItem(utf8("Ra\xc3\xad""z"), 1);
    inversionComboBox.addItem(utf8("1\xc2\xaa inversi\xc3\xb3n"), 2);
    inversionComboBox.addItem(utf8("2\xc2\xaa inversi\xc3\xb3n"), 3);
    inversionComboBox.setSelectedId(1, juce::dontSendNotification);
    inversionComboBox.onChange = [this]() { updatePreview(); };
    addAndMakeVisible(inversionComboBox);

    // Field 4: Distribución
    styleLabel.setText(utf8("DISTRIBUCI\xc3\x93N"), juce::dontSendNotification);
    styleLabel.setFont(juce::FontOptions(9.0f).withStyle("Bold"));
    styleLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(styleLabel);

    styleComboBox.addItem("Cerrado", 1);
    styleComboBox.addItem("Abierto", 2);
    styleComboBox.setSelectedId(1, juce::dontSendNotification);
    styleComboBox.onChange = [this]() { updatePreview(); };
    addAndMakeVisible(styleComboBox);

    // Field 5: Registro / Octava
    octaveLabel.setText("REGISTRO", juce::dontSendNotification);
    octaveLabel.setFont(juce::FontOptions(9.0f).withStyle("Bold"));
    octaveLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(octaveLabel);

    octaveComboBox.addItem("2", 1);
    octaveComboBox.addItem("3", 2);
    octaveComboBox.addItem("4", 3);
    octaveComboBox.setSelectedId(2, juce::dontSendNotification);
    octaveComboBox.onChange = [this]() { updatePreview(); };
    addAndMakeVisible(octaveComboBox);

    // Preview Label
    previewLabel.setText(utf8("C3 \xc2\xb7 E3 \xc2\xb7 G3"), juce::dontSendNotification);
    previewLabel.setFont(juce::FontOptions(12.0f).withStyle("Bold"));
    previewLabel.setColour(juce::Label::textColourId, colors::cyan);
    previewLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(previewLabel);

    // Buttons
    saveButton.setColour(juce::TextButton::buttonColourId, colors::accent);
    saveButton.setColour(juce::TextButton::textColourOffId, colors::accentInk);
    saveButton.setColour(juce::TextButton::textColourOnId, colors::accentInk);
    saveButton.onClick = [this]() {
        const auto newSpec = buildSpecFromControls();
        config.setSpec(currentScene, currentDegree, newSpec);

        isSavedFlashActive = true;
        saveButton.setButtonText(utf8("Guardado \xe2\x9c\x93"));
        startTimer(900);

        if (onSpecSaved) {
            onSpecSaved(currentScene, currentDegree);
        }
        updatePreview();
    };
    addAndMakeVisible(saveButton);

    resetButton.setColour(juce::TextButton::buttonColourId, colors::panelSecondary);
    resetButton.setColour(juce::TextButton::textColourOffId, colors::text);
    resetButton.onClick = [this]() {
        config.resetDegree(currentScene, currentDegree);
        refresh();
        if (onSpecSaved) {
            onSpecSaved(currentScene, currentDegree);
        }
    };
    addAndMakeVisible(resetButton);

    setRuleMode(false);
    refresh();
}

ChordDesignerPanel::~ChordDesignerPanel()
{
    stopTimer();
}

void ChordDesignerPanel::timerCallback()
{
    stopTimer();
    isSavedFlashActive = false;
    saveButton.setButtonText("Guardar en este grado");
}

void ChordDesignerPanel::setSelectedDegree(int degreeIndex)
{
    if (degreeIndex < 0 || degreeIndex > 6) return;
    currentDegree = degreeIndex;
    refresh();
}

void ChordDesignerPanel::setSelectedScene(int sceneIndex)
{
    if (sceneIndex < 0 || sceneIndex > 3) return;
    currentScene = sceneIndex;
    refresh();
}

void ChordDesignerPanel::setRuleMode(bool isFreeMode)
{
    freeMode = isFreeMode;

    if (!freeMode) {
        badgeLabel.setText(utf8("DIAT\xc3\x93NICO"), juce::dontSendNotification);
        badgeLabel.setColour(juce::Label::textColourId, colors::cyan);
        badgeLabel.setColour(juce::Label::backgroundColourId, colors::cyan.withAlpha(0.12f));
        qualityComboBox.setEnabled(false);
        qualityComboBox.setSelectedId(1, juce::dontSendNotification);
    } else {
        badgeLabel.setText("PERSONALIZABLE", juce::dontSendNotification);
        badgeLabel.setColour(juce::Label::textColourId, colors::amber);
        badgeLabel.setColour(juce::Label::backgroundColourId, colors::amber.withAlpha(0.12f));
        qualityComboBox.setEnabled(true);
    }

    updatePreview();
    repaint();
}

void ChordDesignerPanel::refresh()
{
    if (getScene) {
        currentScene = getScene();
    }

    const auto spec = config.getSpec(currentScene, currentDegree);
    syncControlsWithSpec(spec);
    updatePreview();
}

void ChordDesignerPanel::syncControlsWithSpec(const music::VoicingSpec& spec)
{
    // Quality
    if (!freeMode) {
        qualityComboBox.setSelectedId(1, juce::dontSendNotification);
    } else {
        switch (spec.qualityRule) {
            case music::QualityRule::major:      qualityComboBox.setSelectedId(2, juce::dontSendNotification); break;
            case music::QualityRule::minor:      qualityComboBox.setSelectedId(3, juce::dontSendNotification); break;
            case music::QualityRule::diminished: qualityComboBox.setSelectedId(4, juce::dontSendNotification); break;
            case music::QualityRule::diatonic:
            default:                             qualityComboBox.setSelectedId(1, juce::dontSendNotification); break;
        }
    }

    // Extension
    extensionComboBox.setSelectedId(spec.extension == music::ChordExtension::seventh ? 2 : 1, juce::dontSendNotification);

    // Inversion
    inversionComboBox.setSelectedId(std::clamp(spec.inversion, 0, 2) + 1, juce::dontSendNotification);

    // Style
    styleComboBox.setSelectedId(spec.style == music::VoicingStyle::open ? 2 : 1, juce::dontSendNotification);

    // Octave
    octaveComboBox.setSelectedId(std::clamp(spec.baseOctave, 2, 4) - 1, juce::dontSendNotification);
}

music::VoicingSpec ChordDesignerPanel::buildSpecFromControls() const
{
    music::VoicingSpec spec;

    if (!freeMode || qualityComboBox.getSelectedId() == 1) {
        spec.qualityRule = music::QualityRule::diatonic;
    } else {
        switch (qualityComboBox.getSelectedId()) {
            case 2: spec.qualityRule = music::QualityRule::major; break;
            case 3: spec.qualityRule = music::QualityRule::minor; break;
            case 4: spec.qualityRule = music::QualityRule::diminished; break;
            default: spec.qualityRule = music::QualityRule::diatonic; break;
        }
    }

    spec.extension = (extensionComboBox.getSelectedId() == 2)
        ? music::ChordExtension::seventh
        : music::ChordExtension::triad;

    spec.inversion = inversionComboBox.getSelectedId() - 1;
    spec.style = (styleComboBox.getSelectedId() == 2)
        ? music::VoicingStyle::open
        : music::VoicingStyle::close;

    spec.baseOctave = octaveComboBox.getSelectedId() + 1; // 1->2, 2->3, 3->4

    return spec;
}

void ChordDesignerPanel::updatePreview()
{
    int tonic = getTonic ? getTonic() : 0;
    auto previewSpec = buildSpecFromControls();
    if (!freeMode) {
        previewSpec.qualityRule = music::QualityRule::diatonic;
    }

    const auto voiced = voicer.voiceChord(tonic, currentDegree, previewSpec, getScale());

    chordTitleLabel.setText(
        utf8(degreeRomanLabel(getScale(), currentDegree)) + utf8(" \xc2\xb7 ") + voiced.label,
        juce::dontSendNotification);

    juce::String notesStr;
    for (int n = 0; n < voiced.notes.size(); ++n) {
        if (n > 0) notesStr << utf8(" \xc2\xb7 ");
        int midiVal = voiced.notes[static_cast<std::size_t>(n)];
        int pc = ((midiVal % 12) + 12) % 12;
        int oct = (midiVal / 12) - 1;
        notesStr << pitchNames[static_cast<std::size_t>(pc)] << oct;
    }
    previewLabel.setText(notesStr, juce::dontSendNotification);
}

void ChordDesignerPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(colors::panel);
    g.fillRoundedRectangle(bounds, 14.0f);
    g.setColour(colors::line);
    g.drawRoundedRectangle(bounds, 14.0f, 1.0f);

    // Title separator line
    g.drawLine(0.0f, 40.0f, bounds.getWidth(), 40.0f, 1.0f);

    // Note preview dashed border
    auto previewBounds = previewLabel.getBounds().toFloat();
    float dashLengths[2] = { 4.0f, 4.0f };
    g.setColour(juce::Colour(0xff435048));
    g.drawDashedLine(juce::Line<float>(previewBounds.getX(), previewBounds.getY(), previewBounds.getRight(), previewBounds.getY()), dashLengths, 2);
    g.drawDashedLine(juce::Line<float>(previewBounds.getX(), previewBounds.getBottom(), previewBounds.getRight(), previewBounds.getBottom()), dashLengths, 2);
    g.drawDashedLine(juce::Line<float>(previewBounds.getX(), previewBounds.getY(), previewBounds.getX(), previewBounds.getBottom()), dashLengths, 2);
    g.drawDashedLine(juce::Line<float>(previewBounds.getRight(), previewBounds.getY(), previewBounds.getRight(), previewBounds.getBottom()), dashLengths, 2);
}

void ChordDesignerPanel::resized()
{
    auto bounds = getLocalBounds();

    // 1. Panel Header (40 px)
    auto headerArea = bounds.removeFromTop(40).reduced(14, 8);
    headerSubtleLabel.setBounds(headerArea.removeFromRight(100));
    headerTitleLabel.setBounds(headerArea);

    auto bodyArea = bounds.reduced(14, 10);

    // 2. Selected Chord Info
    auto topInfoArea = bodyArea.removeFromTop(38);
    badgeLabel.setBounds(topInfoArea.removeFromRight(95).reduced(0, 8));
    eyebrowLabel.setBounds(topInfoArea.removeFromTop(12));
    chordTitleLabel.setBounds(topInfoArea);

    bodyArea.removeFromTop(6);

    // 3. 2-column Editor Grid
    auto gridArea = bodyArea.removeFromTop(108);
    int colWidth = (gridArea.getWidth() - 10) / 2;

    auto row1 = gridArea.removeFromTop(50);
    auto col1_1 = row1.removeFromLeft(colWidth);
    auto col1_2 = row1.removeFromRight(colWidth);

    qualityLabel.setBounds(col1_1.removeFromTop(14));
    qualityComboBox.setBounds(col1_1.removeFromTop(30));

    extensionLabel.setBounds(col1_2.removeFromTop(14));
    extensionComboBox.setBounds(col1_2.removeFromTop(30));

    gridArea.removeFromTop(8);

    auto row2 = gridArea.removeFromTop(50);
    int col3Width = (gridArea.getWidth() - 16) / 3;
    auto col2_1 = row2.removeFromLeft(col3Width);
    auto col2_2 = row2.removeFromLeft(col3Width);
    auto col2_3 = row2;

    inversionLabel.setBounds(col2_1.removeFromTop(14));
    inversionComboBox.setBounds(col2_1.removeFromTop(30));

    styleLabel.setBounds(col2_2.removeFromTop(14));
    styleComboBox.setBounds(col2_2.removeFromTop(30));

    octaveLabel.setBounds(col2_3.removeFromTop(14));
    octaveComboBox.setBounds(col2_3.removeFromTop(30));

    bodyArea.removeFromTop(8);

    // 4. Note Preview Box
    previewLabel.setBounds(bodyArea.removeFromTop(34));

    bodyArea.removeFromTop(10);

    // 5. Action Buttons
    auto actionArea = bodyArea.removeFromTop(34);
    resetButton.setBounds(actionArea.removeFromRight(85));
    actionArea.removeFromRight(8);
    saveButton.setBounds(actionArea);
}

} // namespace chordsynth::ui
