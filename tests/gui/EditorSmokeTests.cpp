#include <catch2/catch_test_macros.hpp>
#include <juce_gui_basics/juce_gui_basics.h>
#include "plugin/PluginProcessor.h"
#include "plugin/PluginEditor.h"
#include "ui/ChordKeyComponent.h"

using namespace chordsynth;

namespace {

juce::Component* findDescendantWithID(juce::Component& component, const juce::String& componentID)
{
    for (auto* child : component.getChildren()) {
        if (child->getComponentID() == componentID)
            return child;

        if (auto* match = findDescendantWithID(*child, componentID))
            return match;
    }

    return nullptr;
}

} // namespace

bool containsExpectedText(const juce::Component& component, const juce::String& expected)
{
    if (const auto* label = dynamic_cast<const juce::Label*>(&component); label != nullptr && label->getText() == expected)
        return true;

    if (const auto* button = dynamic_cast<const juce::Button*>(&component); button != nullptr && button->getButtonText() == expected)
        return true;

    if (const auto* comboBox = dynamic_cast<const juce::ComboBox*>(&component); comboBox != nullptr && comboBox->getText() == expected)
        return true;

    for (auto* child : component.getChildren()) {
        if (containsExpectedText(*child, expected))
            return true;
    }

    return false;
}

TEST_CASE("Production PluginEditor instantiates full performance interface", "[gui][smoke]") {
    const juce::ScopedJuceInitialiser_GUI guiInitialiser;

    ChordSynthAudioProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());

    REQUIRE(editor != nullptr);

    // Minimum size requirements
    REQUIRE(editor->getWidth() >= 900);
    REQUIRE(editor->getHeight() >= 620);

    // Seven chord keys with accessible IDs degree-0 to degree-6
    for (int deg = 0; deg < 7; ++deg) {
        juce::String degreeId = "degree-" + juce::String(deg);
        auto* keyComp = findDescendantWithID(*editor, degreeId);
        INFO("Checking presence of " << degreeId.toStdString());
        REQUIRE(keyComp != nullptr);
    }

    // Four scene buttons scene-0 to scene-3 must be absent in scene-free model
    for (int scene = 0; scene < 4; ++scene) {
        juce::String sceneId = "scene-" + juce::String(scene);
        auto* sceneBtn = findDescendantWithID(*editor, sceneId);
        INFO("Checking absence of " << sceneId.toStdString());
        REQUIRE(sceneBtn == nullptr);
    }

    // Essential controls: key, waveform, cutoff
    auto* keyCombo = findDescendantWithID(*editor, "key-select");
    REQUIRE(keyCombo != nullptr);

    auto* scaleCombo = dynamic_cast<juce::ComboBox*>(findDescendantWithID(*editor, "scale-select"));
    REQUIRE(scaleCombo != nullptr);
    REQUIRE(scaleCombo->isEnabled());
    REQUIRE(scaleCombo->getItemText(0) == "Mayor");
    REQUIRE(scaleCombo->getItemText(1) == "Menor natural");

    auto* waveCombo = findDescendantWithID(*editor, "waveform-select");
    REQUIRE(waveCombo != nullptr);

    auto* cutoffSlider = findDescendantWithID(*editor, "cutoff-slider");
    REQUIRE(cutoffSlider != nullptr);

    // Extended Chord Designer controls from Task 13
    auto* shapeCombo = dynamic_cast<juce::ComboBox*>(findDescendantWithID(*editor, "chord-shape-select"));
    REQUIRE(shapeCombo != nullptr);
    REQUIRE(shapeCombo->getNumItems() == 9);
    REQUIRE(shapeCombo->getItemText(0) == "Triada");
    REQUIRE(shapeCombo->getItemText(1) == "7");
    REQUIRE(shapeCombo->getItemText(2) == "9");
    REQUIRE(shapeCombo->getItemText(3) == "11");
    REQUIRE(shapeCombo->getItemText(4) == "13");
    REQUIRE(shapeCombo->getItemText(5) == "add9");
    REQUIRE(shapeCombo->getItemText(6) == "6/9");
    REQUIRE(shapeCombo->getItemText(7) == "sus2");
    REQUIRE(shapeCombo->getItemText(8) == "sus4");

    auto* qualityCombo = dynamic_cast<juce::ComboBox*>(findDescendantWithID(*editor, "quality-select"));
    REQUIRE(qualityCombo != nullptr);
    REQUIRE(qualityCombo->getNumItems() == 5);
    REQUIRE_FALSE(qualityCombo->isEnabled()); // Default diatonic mode disables quality override

    auto* voicingStyleCombo = dynamic_cast<juce::ComboBox*>(findDescendantWithID(*editor, "voicing-style-select"));
    REQUIRE(voicingStyleCombo != nullptr);
    REQUIRE(voicingStyleCombo->getNumItems() == 3);
    REQUIRE(voicingStyleCombo->getItemText(0) == "Compacto");
    REQUIRE(voicingStyleCombo->getItemText(1) == "Abierto");
    REQUIRE(voicingStyleCombo->getItemText(2) == "Rootless");

    auto* fifthPolicyCombo = dynamic_cast<juce::ComboBox*>(findDescendantWithID(*editor, "fifth-policy-select"));
    REQUIRE(fifthPolicyCombo != nullptr);
    REQUIRE(fifthPolicyCombo->getNumItems() == 3);
    REQUIRE(fifthPolicyCombo->getItemText(0) == "Auto");
    REQUIRE(fifthPolicyCombo->getItemText(1) == "Incluir");
    REQUIRE(fifthPolicyCombo->getItemText(2) == "Omitir");

    auto* bassModeCombo = dynamic_cast<juce::ComboBox*>(findDescendantWithID(*editor, "bass-mode-select"));
    REQUIRE(bassModeCombo != nullptr);
    REQUIRE(bassModeCombo->getNumItems() == 3);
    REQUIRE(bassModeCombo->getItemText(0) == "Sin bajo");
    REQUIRE(bassModeCombo->getItemText(1) == juce::String::fromUTF8("Ra\xc3\xad" "z"));
    REQUIRE(bassModeCombo->getItemText(2) == "Slash");

    auto* slashDegreeCombo = dynamic_cast<juce::ComboBox*>(findDescendantWithID(*editor, "slash-degree-select"));
    REQUIRE(slashDegreeCombo != nullptr);
    REQUIRE(slashDegreeCombo->getNumItems() == 7);
    REQUIRE_FALSE(slashDegreeCombo->isEnabled()); // Initially disabled when bassMode is Sin bajo

    auto* voiceLeadingCombo = dynamic_cast<juce::ComboBox*>(findDescendantWithID(*editor, "voice-leading-select"));
    REQUIRE(voiceLeadingCombo != nullptr);
    REQUIRE(voiceLeadingCombo->getNumItems() == 2);
    REQUIRE(voiceLeadingCombo->getItemText(0) == "Manual");
    REQUIRE(voiceLeadingCombo->getItemText(1) == juce::String::fromUTF8("Autom\xc3\xa1tico"));

    auto* inversionCombo = dynamic_cast<juce::ComboBox*>(findDescendantWithID(*editor, "inversion-select"));
    REQUIRE(inversionCombo != nullptr);
    REQUIRE(inversionCombo->getNumItems() == 3);

    auto* registerCombo = dynamic_cast<juce::ComboBox*>(findDescendantWithID(*editor, "register-select"));
    REQUIRE(registerCombo != nullptr);
    REQUIRE(registerCombo->getNumItems() == 3);

    // Chord Color Performance Panel controls from Task 16 & 18
    auto* midiPerfToggle = dynamic_cast<juce::ToggleButton*>(findDescendantWithID(*editor, "performance-midi-toggle"));
    REQUIRE(midiPerfToggle != nullptr);
    REQUIRE_FALSE(midiPerfToggle->getToggleState());

    auto* paletteSelectCombo = dynamic_cast<juce::ComboBox*>(findDescendantWithID(*editor, "palette-select"));
    REQUIRE(paletteSelectCombo != nullptr);
    REQUIRE(paletteSelectCombo->getNumItems() == 3);

    for (int slot = 0; slot < 8; ++slot) {
        juce::String colorId = "chord-color-" + juce::String(slot);
        auto* colorBtn = findDescendantWithID(*editor, colorId);
        INFO("Checking presence of " << colorId.toStdString());
        REQUIRE(colorBtn != nullptr);
    }

    auto* commitTransformBtn = findDescendantWithID(*editor, "commit-transform-button");
    REQUIRE(commitTransformBtn != nullptr);
    REQUIRE_FALSE(commitTransformBtn->isEnabled());

    // Preset selector in HeaderBar with 5 built-in presets
    auto* presetCombo = dynamic_cast<juce::ComboBox*>(findDescendantWithID(*editor, "preset-select"));
    REQUIRE(presetCombo != nullptr);
    REQUIRE(presetCombo->getNumItems() == 5);
    REQUIRE(presetCombo->getItemText(0) == "Default (Init)");
    REQUIRE(presetCombo->getItemText(1) == "Warm Saw Chords");
    REQUIRE(presetCombo->getItemText(2) == "Ambient Open Keys");
    REQUIRE(presetCombo->getItemText(3) == "Arp Plucks");
    REQUIRE(presetCombo->getItemText(4) == "Jazz Tension");
}

TEST_CASE("Production PluginEditor renders Spanish interface text as UTF-8", "[gui][utf8]") {
    const juce::ScopedJuceInitialiser_GUI guiInitialiser;

    ChordSynthAudioProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());

    REQUIRE(editor != nullptr);

    for (const auto& expected : {
             juce::String::fromUTF8("Diat\xc3\xb3nico"),
             juce::String::fromUTF8("2  B \xc2\xb7 S\xc3\xa9ptimas"),
             juce::String::fromUTF8("Dise\xc3\xb1" "ar acorde"),
             juce::String::fromUTF8("Seg\xc3\xba" "n escala"),
             juce::String::fromUTF8("Ra\xc3\xad" "z")
         }) {
        INFO("Expected UTF-8 UI text: " << expected.toStdString());
        REQUIRE(containsExpectedText(*editor, expected));
    }

    auto* degreeSeven = dynamic_cast<ui::ChordKeyComponent*>(findDescendantWithID(*editor, "degree-6"));
    REQUIRE(degreeSeven != nullptr);
    REQUIRE(degreeSeven->getDegreeLabel() == juce::String::fromUTF8("vii\xc2\xb0"));
}
