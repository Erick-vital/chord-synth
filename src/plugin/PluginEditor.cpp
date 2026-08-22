#include "PluginEditor.h"
#include "presets/PresetSerializer.h"
#include <algorithm>

#if JUCE_STANDALONE_APPLICATION
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif

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
#if JUCE_STANDALONE_APPLICATION
              if (auto* holder = juce::StandalonePluginHolder::getInstance()) {
                  holder->showAudioSettingsDialog();
              }
#endif
          },
          /*isStandalone=*/JUCE_STANDALONE_APPLICATION != 0),
      harmonyToolbar(
          p.getAPVTS(),
          /*onKeyChanged=*/[this](int newTonic) {
              performanceController.setTonic(newTonic);
              performancePanel.updateChordKeys();
              chordDesignerPanel.refresh();
          },
          /*onRuleModeChanged=*/[this](bool isFreeMode) {
              audioProcessor.getHarmonyState().setQualityRule(
                  isFreeMode ? music::QualityRule::major : music::QualityRule::diatonic);
              chordDesignerPanel.setRuleMode(isFreeMode);
          },
          /*onBeforeKeyChange=*/[this]() {
              performanceController.releaseActiveChord();
          }),
      performancePanel(
          performanceController,
          p.getHarmonyState().getConfiguration(),
          chordVoicer),
      soundPanel(p.getAPVTS()),
      chordDesignerPanel(
          p.getHarmonyState().getConfiguration(),
          chordVoicer,
          /*getTonic=*/[this]() { return performanceController.getTonic(); },
          /*getScene=*/[this]() { return performanceController.getScene(); },
          /*onSpecSaved=*/[this](int scene, int degree) {
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

    performanceController.setScene(p.getHarmonyState().getSelectedScene());
    performanceController.setLiveRevoice(p.getHarmonyState().getLiveRevoice());

    bool isFreeMode = (p.getHarmonyState().getQualityRule() != music::QualityRule::diatonic);
    harmonyToolbar.setRuleMode(isFreeMode);
    chordDesignerPanel.setRuleMode(isFreeMode);

    // Wire callbacks between performance panel and designer
    performancePanel.onDegreeSelected = [this](int degreeIndex) {
        chordDesignerPanel.setSelectedDegree(degreeIndex);
    };

    performancePanel.onSceneSelected = [this](int sceneIndex) {
        audioProcessor.getHarmonyState().setSelectedScene(sceneIndex);
        chordDesignerPanel.setSelectedScene(sceneIndex);
    };

    addAndMakeVisible(headerBar);
    addAndMakeVisible(harmonyToolbar);
    addAndMakeVisible(performancePanel);
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

    // Preset 1: Default (Init Synth)
    presets::Preset initPreset;
    initPreset.name = "Default (Init)";
    initPreset.parameters.key = 0; // C
    initPreset.parameters.waveform = "sine";
    initPreset.parameters.cutoffHz = 8000.0f;
    initPreset.parameters.resonance = 0.2f;
    initPreset.parameters.detuneCents = 7.0f;
    initPreset.parameters.masterGainDb = -12.0f;
    builtinPresets.push_back(initPreset);

    // Preset 2: Warm Saw Chords
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
    warmPreset.harmony.setSelectedScene(1); // Seventh chords
    builtinPresets.push_back(warmPreset);

    // Preset 3: Ambient Open Keys
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
    ambientPreset.harmony.setSelectedScene(2); // Open voicing
    ambientPreset.harmony.setLiveRevoice(true);
    builtinPresets.push_back(ambientPreset);

    // Preset 4: Arp Plucks
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
    builtinPresets.push_back(arpPreset);
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
    performanceController.setScene(preset.harmony.getSelectedScene());
    performanceController.setLiveRevoice(preset.harmony.getLiveRevoice());

    bool isFreeMode = (preset.harmony.getQualityRule() != music::QualityRule::diatonic);
    harmonyToolbar.setTonic(preset.parameters.key);
    harmonyToolbar.setRuleMode(isFreeMode);
    chordDesignerPanel.setRuleMode(isFreeMode);
    chordDesignerPanel.setSelectedScene(preset.harmony.getSelectedScene());

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

    auto contentArea = bounds.reduced(18, 14);

    // 2. Harmony toolbar (64 px high)
    harmonyToolbar.setBounds(contentArea.removeFromTop(64));

    contentArea.removeFromTop(12);

    // 3. Middle performance panel (~260-280 px high, never clipped below 220 px)
    int perfHeight = std::clamp(static_cast<int>(contentArea.getHeight() * 0.46f), 220, 280);
    performancePanel.setBounds(contentArea.removeFromTop(perfHeight));

    contentArea.removeFromTop(12);

    // 4. Bottom area: ChordDesigner on the right (width 360-380 px), left is SoundPanel
    auto lowerArea = contentArea;
    int designerWidth = std::clamp(380, 320, std::max(320, lowerArea.getWidth() / 3));
    chordDesignerPanel.setBounds(lowerArea.removeFromRight(designerWidth));
    lowerArea.removeFromRight(12);
    soundPanel.setBounds(lowerArea);
}

} // namespace chordsynth
