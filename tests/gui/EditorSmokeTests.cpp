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

    // Four scene buttons scene-0 to scene-3
    for (int scene = 0; scene < 4; ++scene) {
        juce::String sceneId = "scene-" + juce::String(scene);
        auto* sceneBtn = findDescendantWithID(*editor, sceneId);
        INFO("Checking presence of " << sceneId.toStdString());
        REQUIRE(sceneBtn != nullptr);
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
