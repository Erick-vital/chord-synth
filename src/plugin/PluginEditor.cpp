#include "PluginEditor.h"
#include "presets/PresetSerializer.h"
#include <algorithm>

namespace chordsynth {

ChordSynthAudioProcessorEditor::ChordSynthAudioProcessorEditor(ChordSynthAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      midiOutput(p.getUiMidiQueue()),
      performanceController(
          p.getHarmonyState().getConfiguration(),
          chordVoicer,
          midiOutput),
      headerBar(
          /*onPresetSelected=*/[this](int presetIndex) {
              loadPresetAtIndex(presetIndex);
          },
          /*onAudioSettingsClicked=*/[this]() {
          },
          /*isStandalone=*/JUCE_STANDALONE_APPLICATION != 0),
      harmonyToolbar(
          p.getAPVTS(),
          /*onKeyChanged=*/[this](int newTonic) {
              performanceController.setTonic(newTonic);
              performancePanel.updateChordKeys();
              chordDesignerPanel.refresh();
          },
          /*onScaleChanged=*/[this](music::Scale newScale) {
              performanceController.releaseActiveChord();
              performanceController.setScale(newScale);
              performancePanel.updateChordKeys();
              chordDesignerPanel.refresh();
          },
          /*onRuleModeChanged=*/[this](bool isFreeMode) {
              audioProcessor.getHarmonyState().setQualityRule(
                  isFreeMode ? music::QualityRule::major : music::QualityRule::diatonic);
              performanceController.setDiatonicMode(!isFreeMode);
              chordDesignerPanel.setRuleMode(isFreeMode);
              performancePanel.updateChordKeys();
          },
          /*onBeforeKeyChange=*/[this]() {
              performanceController.releaseActiveChord();
          }),
      performancePanel(
          performanceController,
          p.getHarmonyState().getConfiguration(),
          chordVoicer),
      chordColorPanel(
          performanceController,
          p.getHarmonyState().getConfiguration(),
          chordVoicer,
          &p.getAPVTS()),
      soundPanel(p.getAPVTS()),
      chordDesignerPanel(
          p.getHarmonyState().getConfiguration(),
          chordVoicer,
          /*getTonic=*/[this]() { return performanceController.getTonic(); },
          /*getScale=*/[this]() { return performanceController.getScale(); },
          /*onSpecSaved=*/[this](int degree) {
              performancePanel.updateChordKeys();
              performanceController.revoiceActiveChordIfHeld(degree);
          })
{
    setLookAndFeel(&lookAndFeel);

    setupBuiltinPresets();
    juce::StringArray presetNames;
    for (const auto& pr : builtinPresets) {
        presetNames.add(pr.name);
    }
    headerBar.setPresetNames(presetNames, 0);

    // Initial tonic from key parameter if present
    if (auto* keyParam = p.getAPVTS().getRawParameterValue(parameters::ids::key)) {
        int initialTonic = static_cast<int>(*keyParam);
        performanceController.setTonic(initialTonic);
        harmonyToolbar.setTonic(initialTonic);
        lastPolledKeyIndex = initialTonic;
    }
    if (auto* scaleParam = p.getAPVTS().getRawParameterValue(parameters::ids::scale)) {
        const auto initialScale = *scaleParam >= 0.5f ? music::Scale::naturalMinor : music::Scale::major;
        performanceController.setScale(initialScale);
        harmonyToolbar.setScale(initialScale);
    }

    performanceController.setLiveRevoice(p.getHarmonyState().getLiveRevoice());
    performancePanel.setLiveRevoice(p.getHarmonyState().getLiveRevoice());

    bool isFreeMode = (p.getHarmonyState().getQualityRule() != music::QualityRule::diatonic);
    performanceController.setDiatonicMode(!isFreeMode);
    harmonyToolbar.setRuleMode(isFreeMode);
    chordDesignerPanel.setRuleMode(isFreeMode);

    // Wire callbacks between performance panel, color panel and designer
    performancePanel.onDegreeSelected = [this](int degreeIndex) {
        chordDesignerPanel.setSelectedDegree(degreeIndex);
    };

    performancePanel.onLiveRevoiceChanged = [this](bool enabled) {
        audioProcessor.getHarmonyState().setLiveRevoice(enabled);
    };

    chordColorPanel.onTransformCommitted = [this]() {
        performancePanel.updateChordKeys();
        chordDesignerPanel.refresh();
    };

    addAndMakeVisible(headerBar);
    addAndMakeVisible(harmonyToolbar);
    addAndMakeVisible(performancePanel);
    addAndMakeVisible(chordColorPanel);
    addAndMakeVisible(soundPanel);
    addAndMakeVisible(chordDesignerPanel);

    setSize(1180, 760);
    setResizable(true, true);
    setResizeLimits(900, 620, 1920, 1200);

    startTimerHz(10); // Polling for external host automation on key parameter
}

ChordSynthAudioProcessorEditor::~ChordSynthAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void ChordSynthAudioProcessorEditor::setupBuiltinPresets()
{
    builtinPresets.clear();

    // Preset 1: Default (Init) -> Diatonic compact triads
    presets::Preset initPreset;
    initPreset.name = "Default (Init)";
    initPreset.parameters.key = 0; // C
    initPreset.parameters.waveform = "sine";
    initPreset.parameters.cutoffHz = 8000.0f;
    initPreset.parameters.resonance = 0.2f;
    initPreset.parameters.detuneCents = 7.0f;
    initPreset.parameters.masterGainDb = -12.0f;
    initPreset.harmony.setConfiguration(music::HarmonyConfiguration::makeDiatonic());
    initPreset.harmony.setLiveRevoice(false);
    builtinPresets.push_back(initPreset);

    // Preset 2: Warm Saw Chords -> Lo‑Fi Warm recipe
    presets::Preset warmPreset;
    warmPreset.name = "Warm Saw Chords";
    warmPreset.parameters.key = 0;
    warmPreset.parameters.waveform = "saw";
    warmPreset.parameters.cutoffHz = 3200.0f;
    warmPreset.parameters.resonance = 0.4f;
    warmPreset.parameters.detuneCents = 12.0f;
    warmPreset.parameters.chorusMix = 0.4f;
    warmPreset.parameters.reverbMix = 0.3f;
    warmPreset.parameters.masterGainDb = -12.0f;
    warmPreset.harmony.setConfiguration(music::HarmonyConfiguration::makeLofiWarm());
    warmPreset.harmony.setLiveRevoice(false);
    builtinPresets.push_back(warmPreset);

    // Preset 3: Ambient Open Keys -> Lo‑Fi Warm recipe with Live Revoice
    presets::Preset ambientPreset;
    ambientPreset.name = "Ambient Open Keys";
    ambientPreset.parameters.key = 0;
    ambientPreset.parameters.waveform = "triangle";
    ambientPreset.parameters.cutoffHz = 4500.0f;
    ambientPreset.parameters.resonance = 0.3f;
    ambientPreset.parameters.detuneCents = 8.0f;
    ambientPreset.parameters.delayMix = 0.35f;
    ambientPreset.parameters.delayFeedback = 0.4f;
    ambientPreset.parameters.reverbMix = 0.5f;
    ambientPreset.parameters.masterGainDb = -12.0f;
    ambientPreset.harmony.setConfiguration(music::HarmonyConfiguration::makeLofiWarm());
    ambientPreset.harmony.setLiveRevoice(true);
    builtinPresets.push_back(ambientPreset);

    // Preset 4: Arp Plucks -> Compact diatonic sevenths
    presets::Preset arpPreset;
    arpPreset.name = "Arp Plucks";
    arpPreset.parameters.key = 0;
    arpPreset.parameters.waveform = "square";
    arpPreset.parameters.cutoffHz = 2400.0f;
    arpPreset.parameters.resonance = 0.8f;
    arpPreset.parameters.detuneCents = 5.0f;
    arpPreset.parameters.arpEnabled = true;
    arpPreset.parameters.arpMode = 0; // Up
    arpPreset.parameters.arpRate = 1; // 1/8
    arpPreset.parameters.arpGate = 0.7f;
    arpPreset.parameters.delayMix = 0.25f;
    arpPreset.parameters.masterGainDb = -12.0f;
    arpPreset.harmony.setConfiguration(music::HarmonyConfiguration::makeSevenths());
    arpPreset.harmony.setLiveRevoice(false);
    builtinPresets.push_back(arpPreset);

    // Preset 5: Jazz Tension -> Jazz Tension recipe with Live Revoice
    presets::Preset jazzPreset;
    jazzPreset.name = "Jazz Tension";
    jazzPreset.parameters.key = 0;
    jazzPreset.parameters.waveform = "triangle";
    jazzPreset.parameters.cutoffHz = 3800.0f;
    jazzPreset.parameters.resonance = 0.35f;
    jazzPreset.parameters.detuneCents = 9.0f;
    jazzPreset.parameters.chorusMix = 0.25f;
    jazzPreset.parameters.reverbMix = 0.35f;
    jazzPreset.parameters.masterGainDb = -12.0f;
    jazzPreset.harmony.setConfiguration(music::HarmonyConfiguration::makeJazzTension());
    jazzPreset.harmony.setLiveRevoice(true);
    builtinPresets.push_back(jazzPreset);
}

void ChordSynthAudioProcessorEditor::loadPresetAtIndex(int index)
{
    if (index < 0 || index >= static_cast<int>(builtinPresets.size()))
        return;

    performanceController.releaseActiveChord();

    const auto& preset = builtinPresets[static_cast<std::size_t>(index)];
    presets::PresetSerializer::applyToProcessorState(
        preset,
        audioProcessor.getAPVTS(),
        audioProcessor.getHarmonyState());

    // Sync performance controller & UI components
    performanceController.setTonic(preset.parameters.key);
    const auto presetScale = preset.parameters.scale == 1
        ? music::Scale::naturalMinor : music::Scale::major;
    performanceController.setScale(presetScale);
    performanceController.setLiveRevoice(preset.harmony.getLiveRevoice());
    performancePanel.setLiveRevoice(preset.harmony.getLiveRevoice());

    bool isFreeMode = (preset.harmony.getQualityRule() != music::QualityRule::diatonic);
    performanceController.setDiatonicMode(!isFreeMode);
    harmonyToolbar.setTonic(preset.parameters.key);
    harmonyToolbar.setScale(presetScale);
    harmonyToolbar.setRuleMode(isFreeMode);
    chordDesignerPanel.setRuleMode(isFreeMode);

    performancePanel.updateChordKeys();
    chordDesignerPanel.refresh();
}

void ChordSynthAudioProcessorEditor::timerCallback()
{
    // Check if key parameter changed from host automation
    if (auto* keyParam = audioProcessor.getAPVTS().getRawParameterValue(parameters::ids::key)) {
        int currentKey = static_cast<int>(*keyParam);
        if (currentKey != lastPolledKeyIndex) {
            lastPolledKeyIndex = currentKey;
            performanceController.setTonic(currentKey);
            harmonyToolbar.setTonic(currentKey);
            performancePanel.updateChordKeys();
            chordDesignerPanel.refresh();
        }
    }

    if (auto* scaleParam = audioProcessor.getAPVTS().getRawParameterValue(parameters::ids::scale)) {
        const auto scale = *scaleParam >= 0.5f ? music::Scale::naturalMinor : music::Scale::major;
        if (scale != performanceController.getScale()) {
            performanceController.releaseActiveChord();
            performanceController.setScale(scale);
            harmonyToolbar.setScale(scale);
            performancePanel.updateChordKeys();
            chordDesignerPanel.refresh();
        }
    }

    soundPanel.updateArpControls();
}

void ChordSynthAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(ui::colors::background);
}

void ChordSynthAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // 1. Top Header bar (56 px high, full width)
    headerBar.setBounds(bounds.removeFromTop(56));

    auto contentArea = bounds.reduced(18, 10);

    // 2. Harmony toolbar (60 px high)
    harmonyToolbar.setBounds(contentArea.removeFromTop(60));

    contentArea.removeFromTop(8);

    // 3. Middle performance panel (~180-220 px high)
    int perfHeight = std::clamp(static_cast<int>(contentArea.getHeight() * 0.36f), 170, 220);
    performancePanel.setBounds(contentArea.removeFromTop(perfHeight));

    contentArea.removeFromTop(8);

    // 4. Chord color panel (70 px high)
    chordColorPanel.setBounds(contentArea.removeFromTop(70));

    contentArea.removeFromTop(8);

    // 5. Bottom area: ChordDesigner on the right (width 360-380 px), left is SoundPanel
    auto lowerArea = contentArea;
    int designerWidth = std::clamp(380, 320, std::max(320, lowerArea.getWidth() / 3));
    chordDesignerPanel.setBounds(lowerArea.removeFromRight(designerWidth));
    lowerArea.removeFromRight(12);
    soundPanel.setBounds(lowerArea);
}

} // namespace chordsynth
