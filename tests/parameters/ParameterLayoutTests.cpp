#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include "parameters/ParameterIds.h"
#include "parameters/ParameterLayout.h"
#include "plugin/PluginProcessor.h"

using namespace chordsynth;

TEST_CASE("APVTS key parameter contract and persistence", "[parameters]") {
    ChordSynthAudioProcessor processor;
    auto& apvts = processor.getAPVTS();

    SECTION("APVTS uses the stable state root type") {
        REQUIRE(apvts.state.getType().toString() == parameters::stateRootType);
    }

    SECTION("Key is a stable 12-choice discrete parameter defaulting to C") {
        auto* keyParam = dynamic_cast<juce::AudioParameterChoice*>(
            apvts.getParameter(parameters::ids::key));
        REQUIRE(keyParam != nullptr);

        const juce::StringArray expectedChoices{
            "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
        };
        const juce::AudioProcessorParameter& hostParameter = *keyParam;
        REQUIRE(keyParam->choices == expectedChoices);
        REQUIRE(keyParam->getParameterID() == parameters::ids::key);
        REQUIRE(keyParam->getVersionHint() == parameters::keyParameterVersion);
        REQUIRE(keyParam->name == parameters::names::key);
        REQUIRE(hostParameter.getNumSteps() == expectedChoices.size());
        REQUIRE(hostParameter.isDiscrete());
        REQUIRE_FALSE(hostParameter.isBoolean());
        REQUIRE(hostParameter.getDefaultValue() == Catch::Approx(0.0f));
        REQUIRE(hostParameter.getValue() == Catch::Approx(hostParameter.getDefaultValue()));
        REQUIRE(keyParam->getIndex() == 0);

        const auto& range = keyParam->getNormalisableRange();
        REQUIRE(range.start == Catch::Approx(0.0f));
        REQUIRE(range.end == Catch::Approx(11.0f));
        REQUIRE(range.interval == Catch::Approx(1.0f));
        REQUIRE(range.convertTo0to1(0.0f) == Catch::Approx(0.0f));
        REQUIRE(range.convertTo0to1(11.0f) == Catch::Approx(1.0f));
    }

    SECTION("APVTS serialization round-trip preserves key selection") {
        auto* keyParam = dynamic_cast<juce::AudioParameterChoice*>(
            apvts.getParameter(parameters::ids::key));
        REQUIRE(keyParam != nullptr);

        *keyParam = 2;
        REQUIRE(keyParam->getIndex() == 2);

        juce::MemoryBlock stateBlock;
        processor.getStateInformation(stateBlock);
        REQUIRE(stateBlock.getSize() > 0);

        ChordSynthAudioProcessor restoredProcessor;
        restoredProcessor.setStateInformation(
            stateBlock.getData(), static_cast<int>(stateBlock.getSize()));

        auto* restoredKeyParam = dynamic_cast<juce::AudioParameterChoice*>(
            restoredProcessor.getAPVTS().getParameter(parameters::ids::key));
        REQUIRE(restoredKeyParam != nullptr);
        REQUIRE(restoredKeyParam->getIndex() == 2);
    }

    SECTION("Stable contract is identical between processor instances") {
        ChordSynthAudioProcessor otherProcessor;
        auto* first = dynamic_cast<juce::AudioParameterChoice*>(
            apvts.getParameter(parameters::ids::key));
        auto* second = dynamic_cast<juce::AudioParameterChoice*>(
            otherProcessor.getAPVTS().getParameter(parameters::ids::key));

        REQUIRE(first != nullptr);
        REQUIRE(second != nullptr);
        const juce::AudioProcessorParameter& firstHostParameter = *first;
        const juce::AudioProcessorParameter& secondHostParameter = *second;
        REQUIRE(first->getParameterID() == second->getParameterID());
        REQUIRE(first->getVersionHint() == second->getVersionHint());
        REQUIRE(first->name == second->name);
        REQUIRE(first->choices == second->choices);
        REQUIRE(firstHostParameter.getNumSteps() == secondHostParameter.getNumSteps());
        REQUIRE(firstHostParameter.getDefaultValue()
                == Catch::Approx(secondHostParameter.getDefaultValue()));
        REQUIRE(first->getNormalisableRange().start
                == Catch::Approx(second->getNormalisableRange().start));
        REQUIRE(first->getNormalisableRange().end
                == Catch::Approx(second->getNormalisableRange().end));
        REQUIRE(first->getNormalisableRange().interval
                == Catch::Approx(second->getNormalisableRange().interval));
    }
}

TEST_CASE("APVTS waveform parameter has a stable automatable contract and persists", "[parameters][waveform]") {
    ChordSynthAudioProcessor processor;
    auto& apvts = processor.getAPVTS();
    auto* waveform = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(parameters::ids::waveform));
    REQUIRE(waveform != nullptr);
    const juce::StringArray expectedChoices{"Sine", "Saw", "Square", "Triangle"};
    const juce::AudioProcessorParameter& hostParameter = *waveform;
    REQUIRE(waveform->getParameterID() == parameters::ids::waveform);
    REQUIRE(waveform->getVersionHint() == parameters::waveformParameterVersion);
    REQUIRE(waveform->name == parameters::names::waveform);
    REQUIRE(waveform->choices == expectedChoices);
    REQUIRE(hostParameter.getNumSteps() == 4);
    REQUIRE(hostParameter.isDiscrete());
    REQUIRE(hostParameter.isAutomatable());
    REQUIRE_FALSE(hostParameter.isBoolean());
    REQUIRE(hostParameter.getDefaultValue() == Catch::Approx(0.0f));
    REQUIRE(waveform->getIndex() == 0);
    const auto& range = waveform->getNormalisableRange();
    REQUIRE(range.start == Catch::Approx(0.0f));
    REQUIRE(range.end == Catch::Approx(3.0f));
    REQUIRE(range.interval == Catch::Approx(1.0f));

    *waveform = 3;
    juce::MemoryBlock stateBlock;
    processor.getStateInformation(stateBlock);
    ChordSynthAudioProcessor restored;
    restored.setStateInformation(stateBlock.getData(), static_cast<int>(stateBlock.getSize()));
    auto* restoredWaveform = dynamic_cast<juce::AudioParameterChoice*>(restored.getAPVTS().getParameter(parameters::ids::waveform));
    REQUIRE(restoredWaveform != nullptr);
    REQUIRE(restoredWaveform->getIndex() == 3);
}

TEST_CASE("State saved before waveform existed restores legacy parameters and keeps sine default",
          "[parameters][waveform][compatibility]") {
    ChordSynthAudioProcessor legacySource;
    auto* legacyKey = dynamic_cast<juce::AudioParameterChoice*>(
        legacySource.getAPVTS().getParameter(parameters::ids::key));
    REQUIRE(legacyKey != nullptr);
    *legacyKey = 7;
    auto legacyState = legacySource.getAPVTS().copyState();

    bool removedWaveform = false;
    for (int index = legacyState.getNumChildren(); --index >= 0;) {
        const auto child = legacyState.getChild(index);
        if (child.getProperty("id").toString() == parameters::ids::waveform) {
            legacyState.removeChild(index, nullptr);
            removedWaveform = true;
        }
    }
    REQUIRE(removedWaveform);

    std::unique_ptr<juce::XmlElement> legacyXml(legacyState.createXml());
    REQUIRE(legacyXml != nullptr);
    juce::MemoryBlock legacyBlock;
    juce::AudioProcessor::copyXmlToBinary(*legacyXml, legacyBlock);

    ChordSynthAudioProcessor restored;
    auto* destinationWaveform = dynamic_cast<juce::AudioParameterChoice*>(
        restored.getAPVTS().getParameter(parameters::ids::waveform));
    REQUIRE(destinationWaveform != nullptr);
    *destinationWaveform = 2;
    REQUIRE(destinationWaveform->getIndex() == 2);
    restored.setStateInformation(
        legacyBlock.getData(), static_cast<int>(legacyBlock.getSize()));
    auto* restoredKey = dynamic_cast<juce::AudioParameterChoice*>(
        restored.getAPVTS().getParameter(parameters::ids::key));
    auto* restoredWaveform = dynamic_cast<juce::AudioParameterChoice*>(
        restored.getAPVTS().getParameter(parameters::ids::waveform));
    REQUIRE(restoredKey != nullptr);
    REQUIRE(restoredWaveform != nullptr);
    REQUIRE(restoredKey->getIndex() == 7);
    REQUIRE(restoredWaveform->getIndex() == 0);

    const auto migratedState = restored.getAPVTS().copyState();
    juce::ValueTree migratedWaveform;
    for (const auto& child : migratedState)
        if (child.getProperty("id").toString() == parameters::ids::waveform)
            migratedWaveform = child;
    REQUIRE(migratedWaveform.isValid());
    REQUIRE(migratedWaveform.getType().toString() == "PARAM");
    REQUIRE(static_cast<float>(migratedWaveform.getProperty("value")) == Catch::Approx(0.0f));
}

TEST_CASE("Low-pass parameters have stable continuous automatable contracts and persist",
          "[parameters][filter]") {
    ChordSynthAudioProcessor processor;
    auto& apvts = processor.getAPVTS();
    auto* cutoff = dynamic_cast<juce::AudioParameterFloat*>(
        apvts.getParameter(parameters::ids::cutoff));
    auto* resonance = dynamic_cast<juce::AudioParameterFloat*>(
        apvts.getParameter(parameters::ids::resonance));
    REQUIRE(cutoff != nullptr);
    REQUIRE(resonance != nullptr);

    const juce::AudioProcessorParameter& cutoffHost = *cutoff;
    const juce::AudioProcessorParameter& resonanceHost = *resonance;
    REQUIRE(cutoff->getParameterID() == parameters::ids::cutoff);
    REQUIRE(cutoff->getVersionHint() == parameters::cutoffParameterVersion);
    REQUIRE(cutoff->name == parameters::names::cutoff);
    REQUIRE(cutoffHost.isAutomatable());
    REQUIRE_FALSE(cutoffHost.isDiscrete());
    REQUIRE_FALSE(cutoffHost.isBoolean());
    const auto& cutoffRange = cutoff->getNormalisableRange();
    REQUIRE(cutoffRange.start == Catch::Approx(20.0f));
    REQUIRE(cutoffRange.end == Catch::Approx(20000.0f));
    REQUIRE(cutoffRange.interval == Catch::Approx(0.0f));
    REQUIRE(cutoffRange.skew != Catch::Approx(1.0f));
    REQUIRE(cutoffRange.convertFrom0to1(0.5f)
            == Catch::Approx(std::sqrt(20.0f * 20000.0f)).margin(0.1f));
    REQUIRE(static_cast<float>(*cutoff) == Catch::Approx(8000.0f));
    REQUIRE(cutoffHost.getDefaultValue()
            == Catch::Approx(cutoffRange.convertTo0to1(8000.0f)));

    REQUIRE(resonance->getParameterID() == parameters::ids::resonance);
    REQUIRE(resonance->getVersionHint() == parameters::resonanceParameterVersion);
    REQUIRE(resonance->name == parameters::names::resonance);
    REQUIRE(resonanceHost.isAutomatable());
    REQUIRE_FALSE(resonanceHost.isDiscrete());
    REQUIRE_FALSE(resonanceHost.isBoolean());
    const auto& resonanceRange = resonance->getNormalisableRange();
    REQUIRE(resonanceRange.start == Catch::Approx(0.1f));
    REQUIRE(resonanceRange.end == Catch::Approx(2.0f));
    REQUIRE(resonanceRange.interval == Catch::Approx(0.0f));
    REQUIRE(resonanceRange.skew == Catch::Approx(1.0f));
    REQUIRE(static_cast<float>(*resonance) == Catch::Approx(0.2f));
    REQUIRE(resonanceHost.getDefaultValue()
            == Catch::Approx(resonanceRange.convertTo0to1(0.2f)));

    *cutoff = 1234.0f;
    *resonance = 1.25f;
    juce::MemoryBlock stateBlock;
    processor.getStateInformation(stateBlock);
    ChordSynthAudioProcessor restored;
    restored.setStateInformation(stateBlock.getData(), static_cast<int>(stateBlock.getSize()));
    auto* restoredCutoff = dynamic_cast<juce::AudioParameterFloat*>(
        restored.getAPVTS().getParameter(parameters::ids::cutoff));
    auto* restoredResonance = dynamic_cast<juce::AudioParameterFloat*>(
        restored.getAPVTS().getParameter(parameters::ids::resonance));
    REQUIRE(restoredCutoff != nullptr);
    REQUIRE(restoredResonance != nullptr);
    REQUIRE(static_cast<float>(*restoredCutoff) == Catch::Approx(1234.0f));
    REQUIRE(static_cast<float>(*restoredResonance) == Catch::Approx(1.25f));
}

TEST_CASE("Legacy state without filter parameters restores new defaults",
          "[parameters][filter][compatibility]") {
    ChordSynthAudioProcessor source;
    auto legacyState = source.getAPVTS().copyState();
    for (int index = legacyState.getNumChildren(); --index >= 0;) {
        const auto id = legacyState.getChild(index).getProperty("id").toString();
        if (id == parameters::ids::cutoff || id == parameters::ids::resonance)
            legacyState.removeChild(index, nullptr);
    }
    std::unique_ptr<juce::XmlElement> xml(legacyState.createXml());
    juce::MemoryBlock block;
    juce::AudioProcessor::copyXmlToBinary(*xml, block);

    ChordSynthAudioProcessor restored;
    auto* cutoff = dynamic_cast<juce::AudioParameterFloat*>(
        restored.getAPVTS().getParameter(parameters::ids::cutoff));
    auto* resonance = dynamic_cast<juce::AudioParameterFloat*>(
        restored.getAPVTS().getParameter(parameters::ids::resonance));
    REQUIRE(cutoff != nullptr);
    REQUIRE(resonance != nullptr);
    *cutoff = 400.0f;
    *resonance = 1.5f;
    restored.setStateInformation(block.getData(), static_cast<int>(block.getSize()));
    REQUIRE(static_cast<float>(*cutoff) == Catch::Approx(8000.0f));
    REQUIRE(static_cast<float>(*resonance) == Catch::Approx(0.2f));
}
