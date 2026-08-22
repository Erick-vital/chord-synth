#include "SoundPanel.h"
#include "ChordSynthLookAndFeel.h"

namespace chordsynth::ui {

SoundPanel::SoundPanel(parameters::AudioProcessorValueTreeState& state)
    : apvts(state)
{
    // Header
    headerTitleLabel.setText("Sonido y movimiento", juce::dontSendNotification);
    headerTitleLabel.setFont(juce::FontOptions(14.0f).withStyle("Bold"));
    headerTitleLabel.setColour(juce::Label::textColourId, colors::text);
    addAndMakeVisible(headerTitleLabel);

    headerSubtleLabel.setText("Controles globales", juce::dontSendNotification);
    headerSubtleLabel.setFont(juce::FontOptions(11.0f));
    headerSubtleLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(headerSubtleLabel);

    advancedToggleButton.setButtonText("Avanzado");
    advancedToggleButton.setClickingTogglesState(true);
    advancedToggleButton.onClick = [this]() {
        isAdvancedOpen = advancedToggleButton.getToggleState();
        updateAdvancedVisibility();
        resized();
        repaint();
    };
    addAndMakeVisible(advancedToggleButton);

    // ==========================================
    // 1. Oscillator Module
    // ==========================================
    oscModuleLabel.setText("OSCILLATOR", juce::dontSendNotification);
    oscModuleLabel.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    oscModuleLabel.setColour(juce::Label::textColourId, colors::text);
    addAndMakeVisible(oscModuleLabel);

    waveLabel.setText("Waveform", juce::dontSendNotification);
    waveLabel.setFont(juce::FontOptions(10.0f));
    waveLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(waveLabel);

    waveComboBox.addItem("Sine", 1);
    waveComboBox.addItem("Saw", 2);
    waveComboBox.addItem("Square", 3);
    waveComboBox.addItem("Triangle", 4);
    addAndMakeVisible(waveComboBox);

    detuneLabel.setText("Detune", juce::dontSendNotification);
    detuneLabel.setFont(juce::FontOptions(10.0f));
    detuneLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(detuneLabel);

    detuneSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    detuneSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    detuneSlider.setTextValueSuffix(" ct");
    addAndMakeVisible(detuneSlider);

    // ==========================================
    // 2. Filter Module
    // ==========================================
    filterModuleLabel.setText("FILTER", juce::dontSendNotification);
    filterModuleLabel.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    filterModuleLabel.setColour(juce::Label::textColourId, colors::text);
    addAndMakeVisible(filterModuleLabel);

    cutoffLabel.setText("Cutoff", juce::dontSendNotification);
    cutoffLabel.setFont(juce::FontOptions(10.0f));
    cutoffLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(cutoffLabel);

    cutoffSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    cutoffSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 55, 20);
    cutoffSlider.setTextValueSuffix(" Hz");
    addAndMakeVisible(cutoffSlider);

    resonanceLabel.setText("Resonance", juce::dontSendNotification);
    resonanceLabel.setFont(juce::FontOptions(10.0f));
    resonanceLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(resonanceLabel);

    resonanceSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    resonanceSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    addAndMakeVisible(resonanceSlider);

    // ==========================================
    // 3. Arpeggiator Module
    // ==========================================
    arpModuleLabel.setText("ARPEGGIATOR", juce::dontSendNotification);
    arpModuleLabel.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    arpModuleLabel.setColour(juce::Label::textColourId, colors::text);
    addAndMakeVisible(arpModuleLabel);

    arpModeLabel.setText("Mode", juce::dontSendNotification);
    arpModeLabel.setFont(juce::FontOptions(10.0f));
    arpModeLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(arpModeLabel);

    // 1: Off, 2: Up, 3: Down, 4: Up/Down, 5: Random
    arpModeComboBox.addItem("Off", 1);
    arpModeComboBox.addItem("Up", 2);
    arpModeComboBox.addItem("Down", 3);
    arpModeComboBox.addItem("Up/Down", 4);
    arpModeComboBox.addItem("Random", 5);
    addAndMakeVisible(arpModeComboBox);

    arpRateLabel.setText("Rate", juce::dontSendNotification);
    arpRateLabel.setFont(juce::FontOptions(10.0f));
    arpRateLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(arpRateLabel);

    arpRateComboBox.addItem("1/4", 1);
    arpRateComboBox.addItem("1/8", 2);
    arpRateComboBox.addItem("1/16", 3);
    addAndMakeVisible(arpRateComboBox);

    arpGateLabel.setText("Gate", juce::dontSendNotification);
    arpGateLabel.setFont(juce::FontOptions(10.0f));
    arpGateLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(arpGateLabel);

    arpGateSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    arpGateSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    addAndMakeVisible(arpGateSlider);

    // ==========================================
    // 4. Space / Effects Module (Main mixes)
    // ==========================================
    spaceModuleLabel.setText("SPACE", juce::dontSendNotification);
    spaceModuleLabel.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    spaceModuleLabel.setColour(juce::Label::textColourId, colors::text);
    addAndMakeVisible(spaceModuleLabel);

    chorusMixLabel.setText("Chorus", juce::dontSendNotification);
    chorusMixLabel.setFont(juce::FontOptions(10.0f));
    chorusMixLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(chorusMixLabel);

    chorusMixSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    chorusMixSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    addAndMakeVisible(chorusMixSlider);

    delayMixLabel.setText("Delay", juce::dontSendNotification);
    delayMixLabel.setFont(juce::FontOptions(10.0f));
    delayMixLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(delayMixLabel);

    delayMixSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    delayMixSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    addAndMakeVisible(delayMixSlider);

    reverbMixLabel.setText("Reverb", juce::dontSendNotification);
    reverbMixLabel.setFont(juce::FontOptions(10.0f));
    reverbMixLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addAndMakeVisible(reverbMixLabel);

    reverbMixSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    reverbMixSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    addAndMakeVisible(reverbMixSlider);

    // ==========================================
    // Advanced Controls (Initially Hidden)
    // ==========================================
    chorusRateLabel.setText("Ch. Rate", juce::dontSendNotification);
    chorusRateLabel.setFont(juce::FontOptions(10.0f));
    chorusRateLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addChildComponent(chorusRateLabel);

    chorusRateSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    chorusRateSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    chorusRateSlider.setTextValueSuffix(" Hz");
    addChildComponent(chorusRateSlider);

    chorusDepthLabel.setText("Ch. Depth", juce::dontSendNotification);
    chorusDepthLabel.setFont(juce::FontOptions(10.0f));
    chorusDepthLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addChildComponent(chorusDepthLabel);

    chorusDepthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    chorusDepthSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    addChildComponent(chorusDepthSlider);

    delayFeedbackLabel.setText("Dly Feedb.", juce::dontSendNotification);
    delayFeedbackLabel.setFont(juce::FontOptions(10.0f));
    delayFeedbackLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addChildComponent(delayFeedbackLabel);

    delayFeedbackSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    delayFeedbackSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    addChildComponent(delayFeedbackSlider);

    delayTimeLabel.setText("Dly Time", juce::dontSendNotification);
    delayTimeLabel.setFont(juce::FontOptions(10.0f));
    delayTimeLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addChildComponent(delayTimeLabel);

    delayTimeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    delayTimeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    delayTimeSlider.setTextValueSuffix(" ms");
    addChildComponent(delayTimeSlider);

    addChildComponent(delaySyncToggle);

    delaySyncRateLabel.setText("Sync Rate", juce::dontSendNotification);
    delaySyncRateLabel.setFont(juce::FontOptions(10.0f));
    delaySyncRateLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addChildComponent(delaySyncRateLabel);

    delaySyncRateComboBox.addItem("1/4", 1);
    delaySyncRateComboBox.addItem("1/8", 2);
    delaySyncRateComboBox.addItem("1/16", 3);
    addChildComponent(delaySyncRateComboBox);

    reverbRoomSizeLabel.setText("Rev Room", juce::dontSendNotification);
    reverbRoomSizeLabel.setFont(juce::FontOptions(10.0f));
    reverbRoomSizeLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addChildComponent(reverbRoomSizeLabel);

    reverbRoomSizeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    reverbRoomSizeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    addChildComponent(reverbRoomSizeSlider);

    reverbDampingLabel.setText("Rev Damp", juce::dontSendNotification);
    reverbDampingLabel.setFont(juce::FontOptions(10.0f));
    reverbDampingLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addChildComponent(reverbDampingLabel);

    reverbDampingSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    reverbDampingSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    addChildComponent(reverbDampingSlider);

    reverbWidthLabel.setText("Rev Width", juce::dontSendNotification);
    reverbWidthLabel.setFont(juce::FontOptions(10.0f));
    reverbWidthLabel.setColour(juce::Label::textColourId, colors::textMuted);
    addChildComponent(reverbWidthLabel);

    reverbWidthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    reverbWidthSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    addChildComponent(reverbWidthSlider);

    // ==========================================
    // APVTS Attachments Creation
    // ==========================================
    waveAttachment = std::make_unique<parameters::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, parameters::ids::waveform, waveComboBox);
    detuneAttachment = std::make_unique<parameters::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, parameters::ids::detune, detuneSlider);

    cutoffAttachment = std::make_unique<parameters::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, parameters::ids::cutoff, cutoffSlider);
    resonanceAttachment = std::make_unique<parameters::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, parameters::ids::resonance, resonanceSlider);

    // Arp attachments
    arpRateAttachment = std::make_unique<parameters::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, parameters::ids::arpRate, arpRateComboBox);
    arpGateAttachment = std::make_unique<parameters::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, parameters::ids::arpGate, arpGateSlider);

    // Setup Arp Mode ComboBox with sync logic for arp_enabled and arp_mode APVTS parameters
    arpModeComboBox.onChange = [this]() {
        int selectedId = arpModeComboBox.getSelectedId();
        if (selectedId == 1) { // Off
            if (auto* enabledParam = apvts.getParameter(parameters::ids::arpEnabled)) {
                enabledParam->setValueNotifyingHost(0.0f);
            }
        } else if (selectedId >= 2 && selectedId <= 5) {
            if (auto* enabledParam = apvts.getParameter(parameters::ids::arpEnabled)) {
                enabledParam->setValueNotifyingHost(1.0f);
            }
            if (auto* modeParam = apvts.getParameter(parameters::ids::arpMode)) {
                // Normalized value for choice parameter: (selectedId - 2) / (numChoices - 1)
                float normVal = static_cast<float>(selectedId - 2) / 3.0f;
                modeParam->setValueNotifyingHost(normVal);
            }
        }
    };

    // Initialize arp mode combo box selection from current APVTS values
    updateArpControls();

    // Space / Effects Attachments
    chorusMixAttachment = std::make_unique<parameters::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, parameters::ids::chorusMix, chorusMixSlider);
    chorusRateAttachment = std::make_unique<parameters::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, parameters::ids::chorusRate, chorusRateSlider);
    chorusDepthAttachment = std::make_unique<parameters::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, parameters::ids::chorusDepth, chorusDepthSlider);

    delayMixAttachment = std::make_unique<parameters::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, parameters::ids::delayMix, delayMixSlider);
    delayFeedbackAttachment = std::make_unique<parameters::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, parameters::ids::delayFeedback, delayFeedbackSlider);
    delayTimeAttachment = std::make_unique<parameters::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, parameters::ids::delayTimeMs, delayTimeSlider);
    delaySyncAttachment = std::make_unique<parameters::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, parameters::ids::delaySync, delaySyncToggle);
    delaySyncRateAttachment = std::make_unique<parameters::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, parameters::ids::delaySyncRate, delaySyncRateComboBox);

    reverbMixAttachment = std::make_unique<parameters::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, parameters::ids::reverbMix, reverbMixSlider);
    reverbRoomSizeAttachment = std::make_unique<parameters::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, parameters::ids::reverbRoomSize, reverbRoomSizeSlider);
    reverbDampingAttachment = std::make_unique<parameters::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, parameters::ids::reverbDamping, reverbDampingSlider);
    reverbWidthAttachment = std::make_unique<parameters::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, parameters::ids::reverbWidth, reverbWidthSlider);
}

void SoundPanel::updateArpControls()
{
    bool isEnabled = false;
    if (auto* enabledVal = apvts.getRawParameterValue(parameters::ids::arpEnabled)) {
        isEnabled = (*enabledVal > 0.5f);
    }

    if (!isEnabled) {
        if (!arpModeComboBox.isPopupActive()) {
            arpModeComboBox.setSelectedId(1, juce::dontSendNotification);
        }
    } else {
        int modeIdx = 0;
        if (auto* modeVal = apvts.getRawParameterValue(parameters::ids::arpMode)) {
            modeIdx = static_cast<int>(*modeVal);
        }
        if (!arpModeComboBox.isPopupActive()) {
            arpModeComboBox.setSelectedId(modeIdx + 2, juce::dontSendNotification);
        }
    }
}

void SoundPanel::updateAdvancedVisibility()
{
    chorusRateLabel.setVisible(isAdvancedOpen);
    chorusRateSlider.setVisible(isAdvancedOpen);
    chorusDepthLabel.setVisible(isAdvancedOpen);
    chorusDepthSlider.setVisible(isAdvancedOpen);

    delayFeedbackLabel.setVisible(isAdvancedOpen);
    delayFeedbackSlider.setVisible(isAdvancedOpen);
    delayTimeLabel.setVisible(isAdvancedOpen);
    delayTimeSlider.setVisible(isAdvancedOpen);
    delaySyncToggle.setVisible(isAdvancedOpen);
    delaySyncRateLabel.setVisible(isAdvancedOpen);
    delaySyncRateComboBox.setVisible(isAdvancedOpen);

    reverbRoomSizeLabel.setVisible(isAdvancedOpen);
    reverbRoomSizeSlider.setVisible(isAdvancedOpen);
    reverbDampingLabel.setVisible(isAdvancedOpen);
    reverbDampingSlider.setVisible(isAdvancedOpen);
    reverbWidthLabel.setVisible(isAdvancedOpen);
    reverbWidthSlider.setVisible(isAdvancedOpen);
}

void SoundPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(colors::panel);
    g.fillRoundedRectangle(bounds, 14.0f);
    g.setColour(colors::line);
    g.drawRoundedRectangle(bounds, 14.0f, 1.0f);

    // Title separator line
    g.drawLine(0.0f, 40.0f, bounds.getWidth(), 40.0f, 1.0f);

    // Module division lines if not advanced
    if (!isAdvancedOpen) {
        float colWidth = bounds.getWidth() / 4.0f;
        g.setColour(colors::line.withAlpha(0.5f));
        g.drawLine(colWidth, 40.0f, colWidth, bounds.getHeight(), 1.0f);
        g.drawLine(colWidth * 2.0f, 40.0f, colWidth * 2.0f, bounds.getHeight(), 1.0f);
        g.drawLine(colWidth * 3.0f, 40.0f, colWidth * 3.0f, bounds.getHeight(), 1.0f);
    }
}

void SoundPanel::resized()
{
    auto bounds = getLocalBounds();

    // 1. Panel Header (40 px)
    auto headerArea = bounds.removeFromTop(40).reduced(14, 6);
    advancedToggleButton.setBounds(headerArea.removeFromRight(90));
    headerSubtleLabel.setBounds(headerArea.removeFromRight(120));
    headerTitleLabel.setBounds(headerArea);

    auto bodyArea = bounds.reduced(14, 10);

    if (!isAdvancedOpen) {
        // Standard 4-column layout
        int colWidth = (bodyArea.getWidth() - 30) / 4;

        // Col 1: Oscillator
        auto col1 = bodyArea.removeFromLeft(colWidth);
        bodyArea.removeFromLeft(10);
        oscModuleLabel.setBounds(col1.removeFromTop(16));
        col1.removeFromTop(8);
        waveLabel.setBounds(col1.removeFromTop(14));
        waveComboBox.setBounds(col1.removeFromTop(28));
        col1.removeFromTop(8);
        detuneLabel.setBounds(col1.removeFromTop(14));
        detuneSlider.setBounds(col1.removeFromTop(28));

        // Col 2: Filter
        auto col2 = bodyArea.removeFromLeft(colWidth);
        bodyArea.removeFromLeft(10);
        filterModuleLabel.setBounds(col2.removeFromTop(16));
        col2.removeFromTop(8);
        cutoffLabel.setBounds(col2.removeFromTop(14));
        cutoffSlider.setBounds(col2.removeFromTop(28));
        col2.removeFromTop(8);
        resonanceLabel.setBounds(col2.removeFromTop(14));
        resonanceSlider.setBounds(col2.removeFromTop(28));

        // Col 3: Arpeggiator
        auto col3 = bodyArea.removeFromLeft(colWidth);
        bodyArea.removeFromLeft(10);
        arpModuleLabel.setBounds(col3.removeFromTop(16));
        col3.removeFromTop(8);
        arpModeLabel.setBounds(col3.removeFromTop(14));
        arpModeComboBox.setBounds(col3.removeFromTop(28));
        col3.removeFromTop(8);
        arpRateLabel.setBounds(col3.removeFromTop(14));
        arpRateComboBox.setBounds(col3.removeFromTop(28));
        col3.removeFromTop(8);
        arpGateLabel.setBounds(col3.removeFromTop(14));
        arpGateSlider.setBounds(col3.removeFromTop(28));

        // Col 4: Space
        auto col4 = bodyArea;
        spaceModuleLabel.setBounds(col4.removeFromTop(16));
        col4.removeFromTop(8);
        chorusMixLabel.setBounds(col4.removeFromTop(14));
        chorusMixSlider.setBounds(col4.removeFromTop(26));
        col4.removeFromTop(4);
        delayMixLabel.setBounds(col4.removeFromTop(14));
        delayMixSlider.setBounds(col4.removeFromTop(26));
        col4.removeFromTop(4);
        reverbMixLabel.setBounds(col4.removeFromTop(14));
        reverbMixSlider.setBounds(col4.removeFromTop(26));
    } else {
        // Advanced Expandable view: 4 columns with advanced sub-controls
        int colWidth = (bodyArea.getWidth() - 30) / 4;

        // Col 1: Oscillator (Standard)
        auto col1 = bodyArea.removeFromLeft(colWidth);
        bodyArea.removeFromLeft(10);
        oscModuleLabel.setBounds(col1.removeFromTop(16));
        col1.removeFromTop(6);
        waveLabel.setBounds(col1.removeFromTop(14));
        waveComboBox.setBounds(col1.removeFromTop(26));
        col1.removeFromTop(6);
        detuneLabel.setBounds(col1.removeFromTop(14));
        detuneSlider.setBounds(col1.removeFromTop(26));

        // Col 2: Filter & Arp
        auto col2 = bodyArea.removeFromLeft(colWidth);
        bodyArea.removeFromLeft(10);
        filterModuleLabel.setBounds(col2.removeFromTop(16));
        col2.removeFromTop(6);
        cutoffLabel.setBounds(col2.removeFromTop(14));
        cutoffSlider.setBounds(col2.removeFromTop(24));
        col2.removeFromTop(4);
        resonanceLabel.setBounds(col2.removeFromTop(14));
        resonanceSlider.setBounds(col2.removeFromTop(24));
        col2.removeFromTop(6);
        arpModuleLabel.setBounds(col2.removeFromTop(16));
        col2.removeFromTop(4);
        arpModeLabel.setBounds(col2.removeFromTop(14));
        arpModeComboBox.setBounds(col2.removeFromTop(24));
        col2.removeFromTop(4);
        arpRateLabel.setBounds(col2.removeFromTop(14));
        arpRateComboBox.setBounds(col2.removeFromTop(24));
        col2.removeFromTop(4);
        arpGateLabel.setBounds(col2.removeFromTop(14));
        arpGateSlider.setBounds(col2.removeFromTop(24));

        // Col 3: Chorus & Delay Advanced
        auto col3 = bodyArea.removeFromLeft(colWidth);
        bodyArea.removeFromLeft(10);
        chorusMixLabel.setBounds(col3.removeFromTop(14));
        chorusMixSlider.setBounds(col3.removeFromTop(24));
        chorusRateLabel.setBounds(col3.removeFromTop(14));
        chorusRateSlider.setBounds(col3.removeFromTop(24));
        chorusDepthLabel.setBounds(col3.removeFromTop(14));
        chorusDepthSlider.setBounds(col3.removeFromTop(24));
        col3.removeFromTop(6);
        delayMixLabel.setBounds(col3.removeFromTop(14));
        delayMixSlider.setBounds(col3.removeFromTop(24));
        delayFeedbackLabel.setBounds(col3.removeFromTop(14));
        delayFeedbackSlider.setBounds(col3.removeFromTop(24));
        delayTimeLabel.setBounds(col3.removeFromTop(14));
        delayTimeSlider.setBounds(col3.removeFromTop(24));

        // Col 4: Reverb & Delay Sync
        auto col4 = bodyArea;
        delaySyncToggle.setBounds(col4.removeFromTop(22));
        delaySyncRateLabel.setBounds(col4.removeFromTop(14));
        delaySyncRateComboBox.setBounds(col4.removeFromTop(24));
        col4.removeFromTop(6);
        reverbMixLabel.setBounds(col4.removeFromTop(14));
        reverbMixSlider.setBounds(col4.removeFromTop(24));
        reverbRoomSizeLabel.setBounds(col4.removeFromTop(14));
        reverbRoomSizeSlider.setBounds(col4.removeFromTop(24));
        reverbDampingLabel.setBounds(col4.removeFromTop(14));
        reverbDampingSlider.setBounds(col4.removeFromTop(24));
        reverbWidthLabel.setBounds(col4.removeFromTop(14));
        reverbWidthSlider.setBounds(col4.removeFromTop(24));
    }
}

} // namespace chordsynth::ui
