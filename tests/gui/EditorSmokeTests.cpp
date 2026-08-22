#include <catch2/catch_test_macros.hpp>
#include <juce_gui_basics/juce_gui_basics.h>
#include "plugin/PluginProcessor.h"
#include "plugin/PluginEditor.h"

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

    auto* waveCombo = findDescendantWithID(*editor, "waveform-select");
    REQUIRE(waveCombo != nullptr);

    auto* cutoffSlider = findDescendantWithID(*editor, "cutoff-slider");
    REQUIRE(cutoffSlider != nullptr);
}
