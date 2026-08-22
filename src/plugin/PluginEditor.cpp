#include "PluginEditor.h"

namespace chordsynth {

ChordSynthAudioProcessorEditor::ChordSynthAudioProcessorEditor(ChordSynthAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      midiOutput(p.getUiMidiQueue()),
      performanceController(
          p.getHarmonyState().getConfiguration(),
          chordVoicer,
          midiOutput),
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
    auto bounds = getLocalBounds().reduced(18);

    // 1. Top harmony toolbar (64 px high)
    harmonyToolbar.setBounds(bounds.removeFromTop(64));

    bounds.removeFromTop(12);

    // 2. Middle performance panel (~280 px high)
    performancePanel.setBounds(bounds.removeFromTop(280));

    bounds.removeFromTop(12);

    // 3. Bottom grid: ChordDesigner on the right (width ~380 px), left is SoundPanel
    auto lowerArea = bounds;
    int designerWidth = 380;
    chordDesignerPanel.setBounds(lowerArea.removeFromRight(designerWidth));
    lowerArea.removeFromRight(12);
    soundPanel.setBounds(lowerArea);
}

} // namespace chordsynth
