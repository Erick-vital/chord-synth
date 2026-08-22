#include <catch2/catch_test_macros.hpp>
#include <juce_gui_basics/juce_gui_basics.h>
#include "plugin/PluginProcessor.h"
#include "plugin/PluginEditor.h"

using namespace chordsynth;

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
        auto* keyComp = editor->findChildWithID(degreeId);
        INFO("Checking presence of " << degreeId.toStdString());
        REQUIRE(keyComp != nullptr);
    }

    // Four scene buttons scene-0 to scene-3
    for (int scene = 0; scene < 4; ++scene) {
        juce::String sceneId = "scene-" + juce::String(scene);
        auto* sceneBtn = editor->findChildWithID(sceneId);
        INFO("Checking presence of " << sceneId.toStdString());
        REQUIRE(sceneBtn != nullptr);
    }

    // Essential controls: key, waveform, cutoff
    auto* keyCombo = editor->findChildWithID("key-select");
    REQUIRE(keyCombo != nullptr);

    auto* waveCombo = editor->findChildWithID("waveform-select");
    REQUIRE(waveCombo != nullptr);

    auto* cutoffSlider = editor->findChildWithID("cutoff-slider");
    REQUIRE(cutoffSlider != nullptr);
}
