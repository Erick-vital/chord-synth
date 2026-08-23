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
    chordTitleLabel.setFont(juce::FontOptions(16.0f).withStyle("Bold"));
    chordTitleLabel.setColour(juce::Label::textColourId, colors::text);
    addAndMakeVisible(chordTitleLabel);

    badgeLabel.setText(utf8("DIAT\xc3\x93NICO"), juce::dontSendNotification);
    badgeLabel.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
    badgeLabel.setColour(juce::Label::textColourId, colors::cyan);
    badgeLabel.setColour(juce::Label::backgroundColourId, colors::cyan.withAlpha(0.12f));
    badgeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(badgeLabel);

    // 1. Forma (chord-shape-select): Triada, 7, 9, 11, 13, add9, 6/9, sus2, sus4
    shapeLabel.setText("FORMA", juce::dontSendNotification);
    shapeLabel.setFont(juce::FontOptions(9.0f).withStyle("Bold"));
    shapeLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(shapeLabel);

    shapeComboBox.setComponentID("chord-shape-select");
    shapeComboBox.addItem("Triada", 1);
    shapeComboBox.addItem("7", 2);
    shapeComboBox.addItem("9", 3);
    shapeComboBox.addItem("11", 4);
    shapeComboBox.addItem("13", 5);
    shapeComboBox.addItem("add9", 6);
    shapeComboBox.addItem("6/9", 7);
    shapeComboBox.addItem("sus2", 8);
    shapeComboBox.addItem("sus4", 9);
    shapeComboBox.setSelectedId(1, juce::dontSendNotification);
    shapeComboBox.onChange = [this]() { updatePreview(); };
    addAndMakeVisible(shapeComboBox);

    // 2. Calidad (quality-select): Según escala, Mayor, Menor, Dominante, Disminuido
    qualityLabel.setText("CALIDAD", juce::dontSendNotification);
    qualityLabel.setFont(juce::FontOptions(9.0f).withStyle("Bold"));
    qualityLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(qualityLabel);

    qualityComboBox.setComponentID("quality-select");
    qualityComboBox.addItem(utf8("Seg\xc3\xba""n escala"), 1);
    qualityComboBox.addItem("Mayor", 2);
    qualityComboBox.addItem("Menor", 3);
    qualityComboBox.addItem("Dominante", 4);
    qualityComboBox.addItem("Disminuido", 5);
    qualityComboBox.setSelectedId(1, juce::dontSendNotification);
    qualityComboBox.onChange = [this]() { updatePreview(); };
    addAndMakeVisible(qualityComboBox);

    // 3. Distribución / Voicing style (voicing-style-select): Compacto, Abierto, Rootless
    voicingStyleLabel.setText(utf8("DISTRIBUCI\xc3\x93N"), juce::dontSendNotification);
    voicingStyleLabel.setFont(juce::FontOptions(9.0f).withStyle("Bold"));
    voicingStyleLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(voicingStyleLabel);

    voicingStyleComboBox.setComponentID("voicing-style-select");
    voicingStyleComboBox.addItem("Compacto", 1);
    voicingStyleComboBox.addItem("Abierto", 2);
    voicingStyleComboBox.addItem("Rootless", 3);
    voicingStyleComboBox.setSelectedId(1, juce::dontSendNotification);
    voicingStyleComboBox.onChange = [this]() { updatePreview(); };
    addAndMakeVisible(voicingStyleComboBox);

    // 4. Quinta (fifth-policy-select): Auto, Incluir, Omitir
    fifthPolicyLabel.setText("QUINTA", juce::dontSendNotification);
    fifthPolicyLabel.setFont(juce::FontOptions(9.0f).withStyle("Bold"));
    fifthPolicyLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(fifthPolicyLabel);

    fifthPolicyComboBox.setComponentID("fifth-policy-select");
    fifthPolicyComboBox.addItem("Auto", 1);
    fifthPolicyComboBox.addItem("Incluir", 2);
    fifthPolicyComboBox.addItem("Omitir", 3);
    fifthPolicyComboBox.setSelectedId(1, juce::dontSendNotification);
    fifthPolicyComboBox.onChange = [this]() { updatePreview(); };
    addAndMakeVisible(fifthPolicyComboBox);

    // 5. Modo de bajo (bass-mode-select): Sin bajo, Raíz, Slash
    bassModeLabel.setText("MODO DE BAJO", juce::dontSendNotification);
    bassModeLabel.setFont(juce::FontOptions(9.0f).withStyle("Bold"));
    bassModeLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(bassModeLabel);

    bassModeComboBox.setComponentID("bass-mode-select");
    bassModeComboBox.addItem("Sin bajo", 1);
    bassModeComboBox.addItem(utf8("Ra\xc3\xad""z"), 2);
    bassModeComboBox.addItem("Slash", 3);
    bassModeComboBox.setSelectedId(1, juce::dontSendNotification);
    bassModeComboBox.onChange = [this]() {
        const bool isSlash = (bassModeComboBox.getSelectedId() == 3);
        slashDegreeComboBox.setEnabled(isSlash);
        updatePreview();
    };
    addAndMakeVisible(bassModeComboBox);

    // 6. Grado slash (slash-degree-select): I..VII
    slashDegreeLabel.setText("GRADO SLASH", juce::dontSendNotification);
    slashDegreeLabel.setFont(juce::FontOptions(9.0f).withStyle("Bold"));
    slashDegreeLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(slashDegreeLabel);

    slashDegreeComboBox.setComponentID("slash-degree-select");
    updateSlashDegreeItems();
    slashDegreeComboBox.setSelectedId(1, juce::dontSendNotification);
    slashDegreeComboBox.setEnabled(false); // Initially disabled (bassMode is none)
    slashDegreeComboBox.onChange = [this]() { updatePreview(); };
    addAndMakeVisible(slashDegreeComboBox);

    // 7. Enlace de voces (voice-leading-select): Manual, Automático
    voiceLeadingLabel.setText("ENLACE DE VOCES", juce::dontSendNotification);
    voiceLeadingLabel.setFont(juce::FontOptions(9.0f).withStyle("Bold"));
    voiceLeadingLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(voiceLeadingLabel);

    voiceLeadingComboBox.setComponentID("voice-leading-select");
    voiceLeadingComboBox.addItem("Manual", 1);
    voiceLeadingComboBox.addItem(utf8("Autom\xc3\xa1tico"), 2);
    voiceLeadingComboBox.setSelectedId(1, juce::dontSendNotification);
    voiceLeadingComboBox.onChange = [this]() { updatePreview(); };
    addAndMakeVisible(voiceLeadingComboBox);

    // 8. Inversión (inversion-select): Raíz, 1ª inversión, 2ª inversión
    inversionLabel.setText(utf8("INVERSI\xc3\x93N"), juce::dontSendNotification);
    inversionLabel.setFont(juce::FontOptions(9.0f).withStyle("Bold"));
    inversionLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(inversionLabel);

    inversionComboBox.setComponentID("inversion-select");
    inversionComboBox.addItem(utf8("Ra\xc3\xad""z"), 1);
    inversionComboBox.addItem(utf8("1\xc2\xaa inversi\xc3\xb3n"), 2);
    inversionComboBox.addItem(utf8("2\xc2\xaa inversi\xc3\xb3n"), 3);
    inversionComboBox.setSelectedId(1, juce::dontSendNotification);
    inversionComboBox.onChange = [this]() { updatePreview(); };
    addAndMakeVisible(inversionComboBox);

    // 9. Registro / Octava (register-select): 2, 3, 4
    registerLabel.setText("REGISTRO", juce::dontSendNotification);
    registerLabel.setFont(juce::FontOptions(9.0f).withStyle("Bold"));
    registerLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(registerLabel);

    registerComboBox.setComponentID("register-select");
    registerComboBox.addItem("2", 1);
    registerComboBox.addItem("3", 2);
    registerComboBox.addItem("4", 3);
    registerComboBox.setSelectedId(2, juce::dontSendNotification);
    registerComboBox.onChange = [this]() { updatePreview(); };
    addAndMakeVisible(registerComboBox);

    // Preview Label
    previewLabel.setText(utf8("C3 \xc2\xb7 E3 \xc2\xb7 G3"), juce::dontSendNotification);
    previewLabel.setFont(juce::FontOptions(11.5f).withStyle("Bold"));
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

void ChordDesignerPanel::updateSlashDegreeItems()
{
    const int currentSelected = slashDegreeComboBox.getSelectedId();
    slashDegreeComboBox.clear(juce::dontSendNotification);
    const auto scale = getScale ? getScale() : music::Scale::major;
    for (int deg = 0; deg < 7; ++deg) {
        slashDegreeComboBox.addItem(utf8(degreeRomanLabel(scale, deg)), deg + 1);
    }
    slashDegreeComboBox.setSelectedId(currentSelected > 0 ? currentSelected : 1, juce::dontSendNotification);
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

    updateSlashDegreeItems();

    const auto spec = config.getSpec(currentScene, currentDegree);
    syncControlsWithSpec(spec);
    updatePreview();
}

void ChordDesignerPanel::syncControlsWithSpec(const music::VoicingSpec& spec)
{
    // Shape: Triada (1), 7 (2), 9 (3), 11 (4), 13 (5), add9 (6), 6/9 (7), sus2 (8), sus4 (9)
    switch (spec.shape) {
        case music::ChordShape::triad:
            // Check legacy extension
            if (spec.extension == music::ChordExtension::seventh) {
                shapeComboBox.setSelectedId(2, juce::dontSendNotification);
            } else {
                shapeComboBox.setSelectedId(1, juce::dontSendNotification);
            }
            break;
        case music::ChordShape::seventh:    shapeComboBox.setSelectedId(2, juce::dontSendNotification); break;
        case music::ChordShape::ninth:      shapeComboBox.setSelectedId(3, juce::dontSendNotification); break;
        case music::ChordShape::eleventh:   shapeComboBox.setSelectedId(4, juce::dontSendNotification); break;
        case music::ChordShape::thirteenth: shapeComboBox.setSelectedId(5, juce::dontSendNotification); break;
        case music::ChordShape::add9:       shapeComboBox.setSelectedId(6, juce::dontSendNotification); break;
        case music::ChordShape::sixNine:    shapeComboBox.setSelectedId(7, juce::dontSendNotification); break;
        case music::ChordShape::sus2:       shapeComboBox.setSelectedId(8, juce::dontSendNotification); break;
        case music::ChordShape::sus4:       shapeComboBox.setSelectedId(9, juce::dontSendNotification); break;
        default:                            shapeComboBox.setSelectedId(1, juce::dontSendNotification); break;
    }

    // Quality: Según escala (1), Mayor (2), Menor (3), Dominante (4), Disminuido (5)
    if (!freeMode) {
        qualityComboBox.setSelectedId(1, juce::dontSendNotification);
    } else {
        switch (spec.qualityRule) {
            case music::QualityRule::major:      qualityComboBox.setSelectedId(2, juce::dontSendNotification); break;
            case music::QualityRule::minor:      qualityComboBox.setSelectedId(3, juce::dontSendNotification); break;
            case music::QualityRule::dominant:   qualityComboBox.setSelectedId(4, juce::dontSendNotification); break;
            case music::QualityRule::diminished: qualityComboBox.setSelectedId(5, juce::dontSendNotification); break;
            case music::QualityRule::diatonic:
            default:                             qualityComboBox.setSelectedId(1, juce::dontSendNotification); break;
        }
    }

    // Voicing Style: Compacto (1), Abierto (2), Rootless (3)
    switch (spec.style) {
        case music::VoicingStyle::open:     voicingStyleComboBox.setSelectedId(2, juce::dontSendNotification); break;
        case music::VoicingStyle::rootless: voicingStyleComboBox.setSelectedId(3, juce::dontSendNotification); break;
        case music::VoicingStyle::compact:
        default:                            voicingStyleComboBox.setSelectedId(1, juce::dontSendNotification); break;
    }

    // Fifth Policy: Auto (1), Incluir (2), Omitir (3)
    switch (spec.fifthPolicy) {
        case music::FifthPolicy::include: fifthPolicyComboBox.setSelectedId(2, juce::dontSendNotification); break;
        case music::FifthPolicy::omit:    fifthPolicyComboBox.setSelectedId(3, juce::dontSendNotification); break;
        case music::FifthPolicy::automatic:
        default:                          fifthPolicyComboBox.setSelectedId(1, juce::dontSendNotification); break;
    }

    // Bass Mode: Sin bajo (1), Raíz (2), Slash (3)
    switch (spec.bassMode) {
        case music::BassMode::root:        bassModeComboBox.setSelectedId(2, juce::dontSendNotification); break;
        case music::BassMode::slashDegree: bassModeComboBox.setSelectedId(3, juce::dontSendNotification); break;
        case music::BassMode::none:
        default:                           bassModeComboBox.setSelectedId(1, juce::dontSendNotification); break;
    }
    slashDegreeComboBox.setEnabled(spec.bassMode == music::BassMode::slashDegree);

    // Slash Degree: 0..6 -> 1..7
    slashDegreeComboBox.setSelectedId(std::clamp(spec.slashDegree, 0, 6) + 1, juce::dontSendNotification);

    // Voice Leading: Manual (1), Automático (2)
    voiceLeadingComboBox.setSelectedId(spec.voiceLeading == music::VoiceLeadingMode::nearest ? 2 : 1, juce::dontSendNotification);

    // Inversion: 0..2 -> 1..3
    inversionComboBox.setSelectedId(std::clamp(spec.inversion, 0, 2) + 1, juce::dontSendNotification);

    // Register / Base Octave: 2, 3, 4 -> 1, 2, 3
    registerComboBox.setSelectedId(std::clamp(spec.baseOctave, 2, 4) - 1, juce::dontSendNotification);
}

music::VoicingSpec ChordDesignerPanel::buildSpecFromControls() const
{
    music::VoicingSpec spec;

    // Shape
    switch (shapeComboBox.getSelectedId()) {
        case 1: spec.shape = music::ChordShape::triad; break;
        case 2: spec.shape = music::ChordShape::seventh; break;
        case 3: spec.shape = music::ChordShape::ninth; break;
        case 4: spec.shape = music::ChordShape::eleventh; break;
        case 5: spec.shape = music::ChordShape::thirteenth; break;
        case 6: spec.shape = music::ChordShape::add9; break;
        case 7: spec.shape = music::ChordShape::sixNine; break;
        case 8: spec.shape = music::ChordShape::sus2; break;
        case 9: spec.shape = music::ChordShape::sus4; break;
        default: spec.shape = music::ChordShape::triad; break;
    }
    spec.extension = (spec.shape == music::ChordShape::seventh)
        ? music::ChordExtension::seventh
        : music::ChordExtension::triad;

    // Quality
    if (!freeMode || qualityComboBox.getSelectedId() == 1) {
        spec.qualityRule = music::QualityRule::diatonic;
    } else {
        switch (qualityComboBox.getSelectedId()) {
            case 2: spec.qualityRule = music::QualityRule::major; break;
            case 3: spec.qualityRule = music::QualityRule::minor; break;
            case 4: spec.qualityRule = music::QualityRule::dominant; break;
            case 5: spec.qualityRule = music::QualityRule::diminished; break;
            default: spec.qualityRule = music::QualityRule::diatonic; break;
        }
    }

    // Voicing Style
    switch (voicingStyleComboBox.getSelectedId()) {
        case 2: spec.style = music::VoicingStyle::open; break;
        case 3: spec.style = music::VoicingStyle::rootless; break;
        case 1:
        default: spec.style = music::VoicingStyle::compact; break;
    }

    // Fifth Policy
    switch (fifthPolicyComboBox.getSelectedId()) {
        case 2: spec.fifthPolicy = music::FifthPolicy::include; break;
        case 3: spec.fifthPolicy = music::FifthPolicy::omit; break;
        case 1:
        default: spec.fifthPolicy = music::FifthPolicy::automatic; break;
    }

    // Bass Mode
    switch (bassModeComboBox.getSelectedId()) {
        case 2: spec.bassMode = music::BassMode::root; break;
        case 3: spec.bassMode = music::BassMode::slashDegree; break;
        case 1:
        default: spec.bassMode = music::BassMode::none; break;
    }

    // Slash Degree (1..7 -> 0..6)
    spec.slashDegree = std::clamp(slashDegreeComboBox.getSelectedId() - 1, 0, 6);

    // Voice Leading
    spec.voiceLeading = (voiceLeadingComboBox.getSelectedId() == 2)
        ? music::VoiceLeadingMode::nearest
        : music::VoiceLeadingMode::manual;

    // Inversion (1..3 -> 0..2)
    spec.inversion = std::clamp(inversionComboBox.getSelectedId() - 1, 0, 2);

    // Register / Base Octave (1..3 -> 2..4)
    spec.baseOctave = std::clamp(registerComboBox.getSelectedId() + 1, 2, 4);

    return spec;
}

void ChordDesignerPanel::updatePreview()
{
    int tonic = getTonic ? getTonic() : 0;
    auto previewSpec = buildSpecFromControls();
    if (!freeMode) {
        previewSpec.qualityRule = music::QualityRule::diatonic;
    }

    const auto voiced = voicer.voiceChord(tonic, currentDegree, previewSpec, getScale ? getScale() : music::Scale::major);

    chordTitleLabel.setText(
        utf8(degreeRomanLabel(getScale ? getScale() : music::Scale::major, currentDegree)) + utf8(" \xc2\xb7 ") + voiced.label,
        juce::dontSendNotification);

    juce::String previewStr;
    for (int n = 0; n < voiced.notes.size(); ++n) {
        if (n > 0) previewStr << utf8(" \xc2\xb7 ");
        int midiVal = voiced.notes[static_cast<std::size_t>(n)];
        int pc = ((midiVal % 12) + 12) % 12;
        int oct = (midiVal / 12) - 1;
        previewStr << pitchNames[static_cast<std::size_t>(pc)] << oct;
    }

    if (voiced.bassMidi.has_value()) {
        int bMidi = *voiced.bassMidi;
        int bPc = ((bMidi % 12) + 12) % 12;
        int bOct = (bMidi / 12) - 1;
        previewStr << "   |   Bajo: " << pitchNames[static_cast<std::size_t>(bPc)] << bOct;
    }

    previewLabel.setText(previewStr, juce::dontSendNotification);
}

void ChordDesignerPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(colors::panel);
    g.fillRoundedRectangle(bounds, 14.0f);
    g.setColour(colors::line);
    g.drawRoundedRectangle(bounds, 14.0f, 1.0f);

    // Title separator line
    g.drawLine(0.0f, 36.0f, bounds.getWidth(), 36.0f, 1.0f);

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

    // 1. Panel Header (36 px)
    auto headerArea = bounds.removeFromTop(36).reduced(12, 6);
    headerSubtleLabel.setBounds(headerArea.removeFromRight(80));
    headerTitleLabel.setBounds(headerArea);

    auto bodyArea = bounds.reduced(12, 8);

    // 2. Selected Chord Info (34 px)
    auto topInfoArea = bodyArea.removeFromTop(34);
    badgeLabel.setBounds(topInfoArea.removeFromRight(100).reduced(0, 6));
    eyebrowLabel.setBounds(topInfoArea.removeFromTop(12));
    chordTitleLabel.setBounds(topInfoArea);

    bodyArea.removeFromTop(4);

    // 3. Grid area for the 9 controls arranged cleanly in 4 rows:
    // Row 1 (2 cols): Forma (chord-shape-select), Calidad (quality-select)
    // Row 2 (2 cols): Distribución (voicing-style-select), Quinta (fifth-policy-select)
    // Row 3 (2 cols): Modo de bajo (bass-mode-select), Grado Slash (slash-degree-select)
    // Row 4 (3 cols): Enlace de voces (voice-leading-select), Inversión (inversion-select), Registro (register-select)
    const int rowHeight = 42;
    const int rowSpacing = 4;
    const int colSpacing = 8;

    auto layout2Cols = [&](juce::Rectangle<int> row, juce::Label& lbl1, juce::ComboBox& cb1, juce::Label& lbl2, juce::ComboBox& cb2) {
        int w = (row.getWidth() - colSpacing) / 2;
        auto c1 = row.removeFromLeft(w);
        auto c2 = row.removeFromRight(w);

        lbl1.setBounds(c1.removeFromTop(13));
        cb1.setBounds(c1.removeFromTop(25));

        lbl2.setBounds(c2.removeFromTop(13));
        cb2.setBounds(c2.removeFromTop(25));
    };

    auto r1 = bodyArea.removeFromTop(rowHeight);
    layout2Cols(r1, shapeLabel, shapeComboBox, qualityLabel, qualityComboBox);
    bodyArea.removeFromTop(rowSpacing);

    auto r2 = bodyArea.removeFromTop(rowHeight);
    layout2Cols(r2, voicingStyleLabel, voicingStyleComboBox, fifthPolicyLabel, fifthPolicyComboBox);
    bodyArea.removeFromTop(rowSpacing);

    auto r3 = bodyArea.removeFromTop(rowHeight);
    layout2Cols(r3, bassModeLabel, bassModeComboBox, slashDegreeLabel, slashDegreeComboBox);
    bodyArea.removeFromTop(rowSpacing);

    auto r4 = bodyArea.removeFromTop(rowHeight);
    int col3W = (r4.getWidth() - (colSpacing * 2)) / 3;
    auto c4_1 = r4.removeFromLeft(col3W);
    auto c4_2 = r4.removeFromLeft(col3W + colSpacing).removeFromRight(col3W);
    auto c4_3 = r4;

    voiceLeadingLabel.setBounds(c4_1.removeFromTop(13));
    voiceLeadingComboBox.setBounds(c4_1.removeFromTop(25));

    inversionLabel.setBounds(c4_2.removeFromTop(13));
    inversionComboBox.setBounds(c4_2.removeFromTop(25));

    registerLabel.setBounds(c4_3.removeFromTop(13));
    registerComboBox.setBounds(c4_3.removeFromTop(25));

    bodyArea.removeFromTop(6);

    // 4. Note Preview Box (28 px)
    previewLabel.setBounds(bodyArea.removeFromTop(28));

    bodyArea.removeFromTop(6);

    // 5. Action Buttons (30 px)
    auto actionArea = bodyArea.removeFromTop(30);
    resetButton.setBounds(actionArea.removeFromRight(85));
    actionArea.removeFromRight(8);
    saveButton.setBounds(actionArea);
}

} // namespace chordsynth::ui

