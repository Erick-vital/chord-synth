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

TEST_CASE("APVTS scale parameter exposes and persists major and natural minor", "[parameters][scale]") {
    ChordSynthAudioProcessor processor;
    auto& apvts = processor.getAPVTS();
    auto* scale = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(parameters::ids::scale));
    REQUIRE(scale != nullptr);

    const juce::StringArray expectedChoices{"Mayor", "Menor natural"};
    const juce::AudioProcessorParameter& hostParameter = *scale;
    REQUIRE(scale->getParameterID() == parameters::ids::scale);
    REQUIRE(scale->getVersionHint() == parameters::scaleParameterVersion);
    REQUIRE(scale->name == parameters::names::scale);
    REQUIRE(scale->choices == expectedChoices);
    REQUIRE(hostParameter.isDiscrete());
    REQUIRE(hostParameter.getNumSteps() == 2);
    REQUIRE(scale->getIndex() == 0);

    *scale = 1;
    juce::MemoryBlock stateBlock;
    processor.getStateInformation(stateBlock);
    ChordSynthAudioProcessor restored;
    restored.setStateInformation(stateBlock.getData(), static_cast<int>(stateBlock.getSize()));
    auto* restoredScale = dynamic_cast<juce::AudioParameterChoice*>(
        restored.getAPVTS().getParameter(parameters::ids::scale));
    REQUIRE(restoredScale != nullptr);
    REQUIRE(restoredScale->getIndex() == 1);
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

TEST_CASE("Detune parameter has stable continuous automatable contract and persists",
          "[parameters][detune]") {
    ChordSynthAudioProcessor processor;
    auto& apvts = processor.getAPVTS();
    auto* detune = dynamic_cast<juce::AudioParameterFloat*>(
        apvts.getParameter(parameters::ids::detune));
    REQUIRE(detune != nullptr);

    const juce::AudioProcessorParameter& detuneHost = *detune;
    REQUIRE(detune->getParameterID() == parameters::ids::detune);
    REQUIRE(detune->getVersionHint() == parameters::detuneParameterVersion);
    REQUIRE(detune->name == parameters::names::detune);
    REQUIRE(detuneHost.isAutomatable());
    REQUIRE_FALSE(detuneHost.isDiscrete());
    REQUIRE_FALSE(detuneHost.isBoolean());
    const auto& range = detune->getNormalisableRange();
    REQUIRE(range.start == Catch::Approx(0.0f));
    REQUIRE(range.end == Catch::Approx(20.0f));
    REQUIRE(range.interval == Catch::Approx(0.0f));
    REQUIRE(static_cast<float>(*detune) == Catch::Approx(7.0f));
    REQUIRE(detuneHost.getDefaultValue()
            == Catch::Approx(range.convertTo0to1(7.0f)));

    *detune = 14.5f;
    juce::MemoryBlock stateBlock;
    processor.getStateInformation(stateBlock);
    ChordSynthAudioProcessor restored;
    restored.setStateInformation(stateBlock.getData(), static_cast<int>(stateBlock.getSize()));
    auto* restoredDetune = dynamic_cast<juce::AudioParameterFloat*>(
        restored.getAPVTS().getParameter(parameters::ids::detune));
    REQUIRE(restoredDetune != nullptr);
    REQUIRE(static_cast<float>(*restoredDetune) == Catch::Approx(14.5f));
}

TEST_CASE("Legacy state without detune parameter restores new default (7 cents)",
          "[parameters][detune][compatibility]") {
    ChordSynthAudioProcessor source;
    auto legacyState = source.getAPVTS().copyState();
    for (int index = legacyState.getNumChildren(); --index >= 0;) {
        const auto id = legacyState.getChild(index).getProperty("id").toString();
        if (id == parameters::ids::detune)
            legacyState.removeChild(index, nullptr);
    }
    std::unique_ptr<juce::XmlElement> xml(legacyState.createXml());
    juce::MemoryBlock block;
    juce::AudioProcessor::copyXmlToBinary(*xml, block);

    ChordSynthAudioProcessor restored;
    auto* detune = dynamic_cast<juce::AudioParameterFloat*>(
        restored.getAPVTS().getParameter(parameters::ids::detune));
    REQUIRE(detune != nullptr);
    *detune = 18.0f;
    restored.setStateInformation(block.getData(), static_cast<int>(block.getSize()));
    REQUIRE(static_cast<float>(*detune) == Catch::Approx(7.0f));
}

TEST_CASE("Chorus parameters have stable continuous automatable contracts and persist",
          "[parameters][chorus]") {
    chordsynth::ChordSynthAudioProcessor processor;
    auto& apvts = processor.getAPVTS();

    auto* chorusMix = dynamic_cast<juce::AudioParameterFloat*>(
        apvts.getParameter(parameters::ids::chorusMix));
    auto* chorusRate = dynamic_cast<juce::AudioParameterFloat*>(
        apvts.getParameter(parameters::ids::chorusRate));
    auto* chorusDepth = dynamic_cast<juce::AudioParameterFloat*>(
        apvts.getParameter(parameters::ids::chorusDepth));

    REQUIRE(chorusMix != nullptr);
    REQUIRE(chorusRate != nullptr);
    REQUIRE(chorusDepth != nullptr);

    REQUIRE(chorusMix->getParameterID() == parameters::ids::chorusMix);
    REQUIRE(chorusMix->getVersionHint() == parameters::chorusMixParameterVersion);
    REQUIRE(chorusMix->name == parameters::names::chorusMix);
    REQUIRE(static_cast<const juce::AudioProcessorParameter&>(*chorusMix).isAutomatable());
    REQUIRE(static_cast<float>(*chorusMix) == Catch::Approx(0.0f));

    REQUIRE(chorusRate->getParameterID() == parameters::ids::chorusRate);
    REQUIRE(chorusRate->getVersionHint() == parameters::chorusRateParameterVersion);
    REQUIRE(chorusRate->name == parameters::names::chorusRate);
    REQUIRE(static_cast<const juce::AudioProcessorParameter&>(*chorusRate).isAutomatable());
    REQUIRE(static_cast<float>(*chorusRate) == Catch::Approx(1.0f));

    REQUIRE(chorusDepth->getParameterID() == parameters::ids::chorusDepth);
    REQUIRE(chorusDepth->getVersionHint() == parameters::chorusDepthParameterVersion);
    REQUIRE(chorusDepth->name == parameters::names::chorusDepth);
    REQUIRE(static_cast<const juce::AudioProcessorParameter&>(*chorusDepth).isAutomatable());
    REQUIRE(static_cast<float>(*chorusDepth) == Catch::Approx(0.25f));

    *chorusMix = 0.75f;
    *chorusRate = 2.5f;
    *chorusDepth = 0.8f;

    juce::MemoryBlock stateBlock;
    processor.getStateInformation(stateBlock);

    chordsynth::ChordSynthAudioProcessor restored;
    restored.setStateInformation(stateBlock.getData(), static_cast<int>(stateBlock.getSize()));

    auto* restoredMix = dynamic_cast<juce::AudioParameterFloat*>(
        restored.getAPVTS().getParameter(parameters::ids::chorusMix));
    auto* restoredRate = dynamic_cast<juce::AudioParameterFloat*>(
        restored.getAPVTS().getParameter(parameters::ids::chorusRate));
    auto* restoredDepth = dynamic_cast<juce::AudioParameterFloat*>(
        restored.getAPVTS().getParameter(parameters::ids::chorusDepth));

    REQUIRE(restoredMix != nullptr);
    REQUIRE(restoredRate != nullptr);
    REQUIRE(restoredDepth != nullptr);
    REQUIRE(static_cast<float>(*restoredMix) == Catch::Approx(0.75f));
    REQUIRE(static_cast<float>(*restoredRate) == Catch::Approx(2.5f));
    REQUIRE(static_cast<float>(*restoredDepth) == Catch::Approx(0.8f));
}

TEST_CASE("Legacy state without chorus parameters restores new defaults",
          "[parameters][chorus][legacy]") {
    chordsynth::ChordSynthAudioProcessor legacySource;
    auto* key = dynamic_cast<juce::AudioParameterChoice*>(
        legacySource.getAPVTS().getParameter(parameters::ids::key));
    if (key != nullptr) *key = 5;

    auto legacyState = legacySource.getAPVTS().copyState();
    for (int i = legacyState.getNumChildren() - 1; i >= 0; --i) {
        auto child = legacyState.getChild(i);
        const auto id = child.getProperty("id").toString();
        if (id == parameters::ids::chorusMix || id == parameters::ids::chorusRate || id == parameters::ids::chorusDepth)
            legacyState.removeChild(i, nullptr);
    }

    std::unique_ptr<juce::XmlElement> xml(legacyState.createXml());
    juce::MemoryBlock block;
    juce::AudioProcessor::copyXmlToBinary(*xml, block);

    chordsynth::ChordSynthAudioProcessor restored;
    auto* mix = dynamic_cast<juce::AudioParameterFloat*>(
        restored.getAPVTS().getParameter(parameters::ids::chorusMix));
    auto* rate = dynamic_cast<juce::AudioParameterFloat*>(
        restored.getAPVTS().getParameter(parameters::ids::chorusRate));
    auto* depth = dynamic_cast<juce::AudioParameterFloat*>(
        restored.getAPVTS().getParameter(parameters::ids::chorusDepth));
    REQUIRE(mix != nullptr);
    REQUIRE(rate != nullptr);
    REQUIRE(depth != nullptr);

    // Set non-defaults before loading
    *mix = 0.9f;
    *rate = 5.0f;
    *depth = 0.9f;

    restored.setStateInformation(block.getData(), static_cast<int>(block.getSize()));

    REQUIRE(static_cast<float>(*mix) == Catch::Approx(0.0f));
    REQUIRE(static_cast<float>(*rate) == Catch::Approx(1.0f));
    REQUIRE(static_cast<float>(*depth) == Catch::Approx(0.25f));

    auto* restoredKey = dynamic_cast<juce::AudioParameterChoice*>(
        restored.getAPVTS().getParameter(parameters::ids::key));
    REQUIRE(restoredKey != nullptr);
    REQUIRE(restoredKey->getIndex() == 5);
}

TEST_CASE("Delay parameters have stable continuous automatable contracts and persist",
          "[parameters][delay]") {
    chordsynth::ChordSynthAudioProcessor processor;
    auto& apvts = processor.getAPVTS();

    auto* delayMix = dynamic_cast<juce::AudioParameterFloat*>(
        apvts.getParameter(parameters::ids::delayMix));
    auto* delayFeedback = dynamic_cast<juce::AudioParameterFloat*>(
        apvts.getParameter(parameters::ids::delayFeedback));
    auto* delayTimeMs = dynamic_cast<juce::AudioParameterFloat*>(
        apvts.getParameter(parameters::ids::delayTimeMs));
    auto* delaySync = dynamic_cast<juce::AudioParameterBool*>(
        apvts.getParameter(parameters::ids::delaySync));
    auto* delaySyncRate = dynamic_cast<juce::AudioParameterChoice*>(
        apvts.getParameter(parameters::ids::delaySyncRate));

    REQUIRE(delayMix != nullptr);
    REQUIRE(delayFeedback != nullptr);
    REQUIRE(delayTimeMs != nullptr);
    REQUIRE(delaySync != nullptr);
    REQUIRE(delaySyncRate != nullptr);

    REQUIRE(delayMix->getParameterID() == parameters::ids::delayMix);
    REQUIRE(delayMix->getVersionHint() == parameters::delayMixParameterVersion);
    REQUIRE(delayMix->name == parameters::names::delayMix);
    REQUIRE(static_cast<const juce::AudioProcessorParameter&>(*delayMix).isAutomatable());
    REQUIRE(static_cast<float>(*delayMix) == Catch::Approx(0.0f));

    REQUIRE(delayFeedback->getParameterID() == parameters::ids::delayFeedback);
    REQUIRE(delayFeedback->getVersionHint() == parameters::delayFeedbackParameterVersion);
    REQUIRE(delayFeedback->name == parameters::names::delayFeedback);
    REQUIRE(static_cast<const juce::AudioProcessorParameter&>(*delayFeedback).isAutomatable());
    REQUIRE(static_cast<float>(*delayFeedback) == Catch::Approx(0.3f));

    REQUIRE(delayTimeMs->getParameterID() == parameters::ids::delayTimeMs);
    REQUIRE(delayTimeMs->getVersionHint() == parameters::delayTimeMsParameterVersion);
    REQUIRE(delayTimeMs->name == parameters::names::delayTimeMs);
    REQUIRE(static_cast<const juce::AudioProcessorParameter&>(*delayTimeMs).isAutomatable());
    REQUIRE(static_cast<float>(*delayTimeMs) == Catch::Approx(250.0f));

    REQUIRE(delaySync->getParameterID() == parameters::ids::delaySync);
    REQUIRE(delaySync->getVersionHint() == parameters::delaySyncParameterVersion);
    REQUIRE(delaySync->name == parameters::names::delaySync);
    REQUIRE(static_cast<const juce::AudioProcessorParameter&>(*delaySync).isAutomatable());
    REQUIRE(*delaySync == true);

    REQUIRE(delaySyncRate->getParameterID() == parameters::ids::delaySyncRate);
    REQUIRE(delaySyncRate->getVersionHint() == parameters::delaySyncRateParameterVersion);
    REQUIRE(delaySyncRate->name == parameters::names::delaySyncRate);
    REQUIRE(static_cast<const juce::AudioProcessorParameter&>(*delaySyncRate).isAutomatable());
    REQUIRE(delaySyncRate->getIndex() == 0);

    *delayMix = 0.6f;
    *delayFeedback = 0.45f;
    *delayTimeMs = 350.0f;
    *delaySync = false;
    *delaySyncRate = 1;

    juce::MemoryBlock stateBlock;
    processor.getStateInformation(stateBlock);

    chordsynth::ChordSynthAudioProcessor restored;
    restored.setStateInformation(stateBlock.getData(), static_cast<int>(stateBlock.getSize()));

    auto* restoredMix = dynamic_cast<juce::AudioParameterFloat*>(
        restored.getAPVTS().getParameter(parameters::ids::delayMix));
    auto* restoredFeedback = dynamic_cast<juce::AudioParameterFloat*>(
        restored.getAPVTS().getParameter(parameters::ids::delayFeedback));
    auto* restoredTimeMs = dynamic_cast<juce::AudioParameterFloat*>(
        restored.getAPVTS().getParameter(parameters::ids::delayTimeMs));
    auto* restoredSync = dynamic_cast<juce::AudioParameterBool*>(
        restored.getAPVTS().getParameter(parameters::ids::delaySync));
    auto* restoredSyncRate = dynamic_cast<juce::AudioParameterChoice*>(
        restored.getAPVTS().getParameter(parameters::ids::delaySyncRate));

    REQUIRE(restoredMix != nullptr);
    REQUIRE(restoredFeedback != nullptr);
    REQUIRE(restoredTimeMs != nullptr);
    REQUIRE(restoredSync != nullptr);
    REQUIRE(restoredSyncRate != nullptr);

    REQUIRE(static_cast<float>(*restoredMix) == Catch::Approx(0.6f));
    REQUIRE(static_cast<float>(*restoredFeedback) == Catch::Approx(0.45f));
    REQUIRE(static_cast<float>(*restoredTimeMs) == Catch::Approx(350.0f));
    REQUIRE(*restoredSync == false);
    REQUIRE(restoredSyncRate->getIndex() == 1);
}

TEST_CASE("Legacy state without delay parameters restores new defaults",
          "[parameters][delay][legacy]") {
    chordsynth::ChordSynthAudioProcessor legacySource;
    auto* key = dynamic_cast<juce::AudioParameterChoice*>(
        legacySource.getAPVTS().getParameter(parameters::ids::key));
    if (key != nullptr) *key = 7;

    auto legacyState = legacySource.getAPVTS().copyState();
    for (int i = legacyState.getNumChildren() - 1; i >= 0; --i) {
        auto child = legacyState.getChild(i);
        const auto id = child.getProperty("id").toString();
        if (id == parameters::ids::delayMix || id == parameters::ids::delayFeedback ||
            id == parameters::ids::delayTimeMs || id == parameters::ids::delaySync ||
            id == parameters::ids::delaySyncRate)
            legacyState.removeChild(i, nullptr);
    }

    std::unique_ptr<juce::XmlElement> xml(legacyState.createXml());
    juce::MemoryBlock block;
    juce::AudioProcessor::copyXmlToBinary(*xml, block);

    chordsynth::ChordSynthAudioProcessor restored;
    auto* mix = dynamic_cast<juce::AudioParameterFloat*>(
        restored.getAPVTS().getParameter(parameters::ids::delayMix));
    auto* feedback = dynamic_cast<juce::AudioParameterFloat*>(
        restored.getAPVTS().getParameter(parameters::ids::delayFeedback));
    auto* timeMs = dynamic_cast<juce::AudioParameterFloat*>(
        restored.getAPVTS().getParameter(parameters::ids::delayTimeMs));
    auto* sync = dynamic_cast<juce::AudioParameterBool*>(
        restored.getAPVTS().getParameter(parameters::ids::delaySync));
    auto* syncRate = dynamic_cast<juce::AudioParameterChoice*>(
        restored.getAPVTS().getParameter(parameters::ids::delaySyncRate));

    REQUIRE(mix != nullptr);
    REQUIRE(feedback != nullptr);
    REQUIRE(timeMs != nullptr);
    REQUIRE(sync != nullptr);
    REQUIRE(syncRate != nullptr);

    // Set non-defaults before loading
    *mix = 0.9f;
    *feedback = 0.8f;
    *timeMs = 900.0f;
    *sync = false;
    *syncRate = 2;

    restored.setStateInformation(block.getData(), static_cast<int>(block.getSize()));

    REQUIRE(static_cast<float>(*mix) == Catch::Approx(0.0f));
    REQUIRE(static_cast<float>(*feedback) == Catch::Approx(0.3f));
    REQUIRE(static_cast<float>(*timeMs) == Catch::Approx(250.0f));
    REQUIRE(*sync == true);
    REQUIRE(syncRate->getIndex() == 0);

    auto* restoredKey = dynamic_cast<juce::AudioParameterChoice*>(
        restored.getAPVTS().getParameter(parameters::ids::key));
    REQUIRE(restoredKey != nullptr);
    REQUIRE(restoredKey->getIndex() == 7);
}

TEST_CASE("Reverb parameters have stable continuous automatable contracts and persist",
          "[parameters][reverb]") {
    chordsynth::ChordSynthAudioProcessor processor;
    auto& apvts = processor.getAPVTS();

    auto* reverbMix = dynamic_cast<juce::AudioParameterFloat*>(
        apvts.getParameter(parameters::ids::reverbMix));
    auto* reverbRoomSize = dynamic_cast<juce::AudioParameterFloat*>(
        apvts.getParameter(parameters::ids::reverbRoomSize));
    auto* reverbDamping = dynamic_cast<juce::AudioParameterFloat*>(
        apvts.getParameter(parameters::ids::reverbDamping));
    auto* reverbWidth = dynamic_cast<juce::AudioParameterFloat*>(
        apvts.getParameter(parameters::ids::reverbWidth));

    REQUIRE(reverbMix != nullptr);
    REQUIRE(reverbRoomSize != nullptr);
    REQUIRE(reverbDamping != nullptr);
    REQUIRE(reverbWidth != nullptr);

    REQUIRE(reverbMix->getParameterID() == parameters::ids::reverbMix);
    REQUIRE(reverbMix->getVersionHint() == parameters::reverbMixParameterVersion);
    REQUIRE(reverbMix->name == parameters::names::reverbMix);
    REQUIRE(static_cast<const juce::AudioProcessorParameter&>(*reverbMix).isAutomatable());
    REQUIRE(static_cast<float>(*reverbMix) == Catch::Approx(0.0f));

    REQUIRE(reverbRoomSize->getParameterID() == parameters::ids::reverbRoomSize);
    REQUIRE(reverbRoomSize->getVersionHint() == parameters::reverbRoomSizeParameterVersion);
    REQUIRE(reverbRoomSize->name == parameters::names::reverbRoomSize);
    REQUIRE(static_cast<const juce::AudioProcessorParameter&>(*reverbRoomSize).isAutomatable());
    REQUIRE(static_cast<float>(*reverbRoomSize) == Catch::Approx(0.5f));

    REQUIRE(reverbDamping->getParameterID() == parameters::ids::reverbDamping);
    REQUIRE(reverbDamping->getVersionHint() == parameters::reverbDampingParameterVersion);
    REQUIRE(reverbDamping->name == parameters::names::reverbDamping);
    REQUIRE(static_cast<const juce::AudioProcessorParameter&>(*reverbDamping).isAutomatable());
    REQUIRE(static_cast<float>(*reverbDamping) == Catch::Approx(0.5f));

    REQUIRE(reverbWidth->getParameterID() == parameters::ids::reverbWidth);
    REQUIRE(reverbWidth->getVersionHint() == parameters::reverbWidthParameterVersion);
    REQUIRE(reverbWidth->name == parameters::names::reverbWidth);
    REQUIRE(static_cast<const juce::AudioProcessorParameter&>(*reverbWidth).isAutomatable());
    REQUIRE(static_cast<float>(*reverbWidth) == Catch::Approx(1.0f));

    *reverbMix = 0.4f;
    *reverbRoomSize = 0.75f;
    *reverbDamping = 0.3f;
    *reverbWidth = 0.8f;

    juce::MemoryBlock stateBlock;
    processor.getStateInformation(stateBlock);

    chordsynth::ChordSynthAudioProcessor restored;
    restored.setStateInformation(stateBlock.getData(), static_cast<int>(stateBlock.getSize()));

    auto* restoredMix = dynamic_cast<juce::AudioParameterFloat*>(
        restored.getAPVTS().getParameter(parameters::ids::reverbMix));
    auto* restoredRoomSize = dynamic_cast<juce::AudioParameterFloat*>(
        restored.getAPVTS().getParameter(parameters::ids::reverbRoomSize));
    auto* restoredDamping = dynamic_cast<juce::AudioParameterFloat*>(
        restored.getAPVTS().getParameter(parameters::ids::reverbDamping));
    auto* restoredWidth = dynamic_cast<juce::AudioParameterFloat*>(
        restored.getAPVTS().getParameter(parameters::ids::reverbWidth));

    REQUIRE(restoredMix != nullptr);
    REQUIRE(restoredRoomSize != nullptr);
    REQUIRE(restoredDamping != nullptr);
    REQUIRE(restoredWidth != nullptr);

    REQUIRE(static_cast<float>(*restoredMix) == Catch::Approx(0.4f));
    REQUIRE(static_cast<float>(*restoredRoomSize) == Catch::Approx(0.75f));
    REQUIRE(static_cast<float>(*restoredDamping) == Catch::Approx(0.3f));
    REQUIRE(static_cast<float>(*restoredWidth) == Catch::Approx(0.8f));
}

TEST_CASE("Legacy state without reverb parameters restores new defaults",
          "[parameters][reverb][legacy]") {
    chordsynth::ChordSynthAudioProcessor legacySource;
    auto* key = dynamic_cast<juce::AudioParameterChoice*>(
        legacySource.getAPVTS().getParameter(parameters::ids::key));
    if (key != nullptr) *key = 9;

    auto legacyState = legacySource.getAPVTS().copyState();
    for (int i = legacyState.getNumChildren() - 1; i >= 0; --i) {
        auto child = legacyState.getChild(i);
        const auto id = child.getProperty("id").toString();
        if (id == parameters::ids::reverbMix || id == parameters::ids::reverbRoomSize ||
            id == parameters::ids::reverbDamping || id == parameters::ids::reverbWidth)
            legacyState.removeChild(i, nullptr);
    }

    std::unique_ptr<juce::XmlElement> xml(legacyState.createXml());
    juce::MemoryBlock block;
    juce::AudioProcessor::copyXmlToBinary(*xml, block);

    chordsynth::ChordSynthAudioProcessor restored;
    auto* mix = dynamic_cast<juce::AudioParameterFloat*>(
        restored.getAPVTS().getParameter(parameters::ids::reverbMix));
    auto* roomSize = dynamic_cast<juce::AudioParameterFloat*>(
        restored.getAPVTS().getParameter(parameters::ids::reverbRoomSize));
    auto* damping = dynamic_cast<juce::AudioParameterFloat*>(
        restored.getAPVTS().getParameter(parameters::ids::reverbDamping));
    auto* width = dynamic_cast<juce::AudioParameterFloat*>(
        restored.getAPVTS().getParameter(parameters::ids::reverbWidth));

    REQUIRE(mix != nullptr);
    REQUIRE(roomSize != nullptr);
    REQUIRE(damping != nullptr);
    REQUIRE(width != nullptr);

    // Set non-defaults before loading
    *mix = 0.9f;
    *roomSize = 0.1f;
    *damping = 0.9f;
    *width = 0.1f;

    restored.setStateInformation(block.getData(), static_cast<int>(block.getSize()));

    REQUIRE(static_cast<float>(*mix) == Catch::Approx(0.0f));
    REQUIRE(static_cast<float>(*roomSize) == Catch::Approx(0.5f));
    REQUIRE(static_cast<float>(*damping) == Catch::Approx(0.5f));
    REQUIRE(static_cast<float>(*width) == Catch::Approx(1.0f));

    auto* restoredKey = dynamic_cast<juce::AudioParameterChoice*>(
        restored.getAPVTS().getParameter(parameters::ids::key));
    REQUIRE(restoredKey != nullptr);
    REQUIRE(restoredKey->getIndex() == 9);
}

TEST_CASE("Arpeggiator parameters have stable continuous automatable contracts and persist",
          "[parameters][arp]") {
    chordsynth::ChordSynthAudioProcessor processor;
    auto& apvts = processor.getAPVTS();

    auto* arpEnabled = dynamic_cast<juce::AudioParameterBool*>(
        apvts.getParameter(parameters::ids::arpEnabled));
    auto* arpMode = dynamic_cast<juce::AudioParameterChoice*>(
        apvts.getParameter(parameters::ids::arpMode));
    auto* arpRate = dynamic_cast<juce::AudioParameterChoice*>(
        apvts.getParameter(parameters::ids::arpRate));
    auto* arpGate = dynamic_cast<juce::AudioParameterFloat*>(
        apvts.getParameter(parameters::ids::arpGate));

    REQUIRE(arpEnabled != nullptr);
    REQUIRE(arpMode != nullptr);
    REQUIRE(arpRate != nullptr);
    REQUIRE(arpGate != nullptr);

    REQUIRE(arpEnabled->getParameterID() == parameters::ids::arpEnabled);
    REQUIRE(arpEnabled->getVersionHint() == parameters::arpEnabledParameterVersion);
    REQUIRE(arpEnabled->name == parameters::names::arpEnabled);
    REQUIRE(static_cast<const juce::AudioProcessorParameter&>(*arpEnabled).isAutomatable());
    REQUIRE(*arpEnabled == false);

    REQUIRE(arpMode->getParameterID() == parameters::ids::arpMode);
    REQUIRE(arpMode->getVersionHint() == parameters::arpModeParameterVersion);
    REQUIRE(arpMode->name == parameters::names::arpMode);
    REQUIRE(static_cast<const juce::AudioProcessorParameter&>(*arpMode).isAutomatable());
    REQUIRE(arpMode->getIndex() == 0);

    REQUIRE(arpRate->getParameterID() == parameters::ids::arpRate);
    REQUIRE(arpRate->getVersionHint() == parameters::arpRateParameterVersion);
    REQUIRE(arpRate->name == parameters::names::arpRate);
    REQUIRE(static_cast<const juce::AudioProcessorParameter&>(*arpRate).isAutomatable());
    REQUIRE(arpRate->getIndex() == 1);

    REQUIRE(arpGate->getParameterID() == parameters::ids::arpGate);
    REQUIRE(arpGate->getVersionHint() == parameters::arpGateParameterVersion);
    REQUIRE(arpGate->name == parameters::names::arpGate);
    REQUIRE(static_cast<const juce::AudioProcessorParameter&>(*arpGate).isAutomatable());
    REQUIRE(static_cast<float>(*arpGate) == Catch::Approx(0.8f));

    *arpEnabled = true;
    *arpMode = 2; // Up/Down
    *arpRate = 2; // 1/16
    *arpGate = 0.5f;

    juce::MemoryBlock stateBlock;
    processor.getStateInformation(stateBlock);

    chordsynth::ChordSynthAudioProcessor restored;
    restored.setStateInformation(stateBlock.getData(), static_cast<int>(stateBlock.getSize()));

    auto* restoredEnabled = dynamic_cast<juce::AudioParameterBool*>(
        restored.getAPVTS().getParameter(parameters::ids::arpEnabled));
    auto* restoredMode = dynamic_cast<juce::AudioParameterChoice*>(
        restored.getAPVTS().getParameter(parameters::ids::arpMode));
    auto* restoredRate = dynamic_cast<juce::AudioParameterChoice*>(
        restored.getAPVTS().getParameter(parameters::ids::arpRate));
    auto* restoredGate = dynamic_cast<juce::AudioParameterFloat*>(
        restored.getAPVTS().getParameter(parameters::ids::arpGate));

    REQUIRE(restoredEnabled != nullptr);
    REQUIRE(restoredMode != nullptr);
    REQUIRE(restoredRate != nullptr);
    REQUIRE(restoredGate != nullptr);

    REQUIRE(*restoredEnabled == true);
    REQUIRE(restoredMode->getIndex() == 2);
    REQUIRE(restoredRate->getIndex() == 2);
    REQUIRE(static_cast<float>(*restoredGate) == Catch::Approx(0.5f));
}

TEST_CASE("Legacy state without arpeggiator parameters restores new defaults",
          "[parameters][arp][legacy]") {
    chordsynth::ChordSynthAudioProcessor legacySource;
    auto* key = dynamic_cast<juce::AudioParameterChoice*>(
        legacySource.getAPVTS().getParameter(parameters::ids::key));
    if (key != nullptr) *key = 11;

    auto legacyState = legacySource.getAPVTS().copyState();
    for (int i = legacyState.getNumChildren() - 1; i >= 0; --i) {
        auto child = legacyState.getChild(i);
        const auto id = child.getProperty("id").toString();
        if (id == parameters::ids::arpEnabled || id == parameters::ids::arpMode ||
            id == parameters::ids::arpRate || id == parameters::ids::arpGate)
            legacyState.removeChild(i, nullptr);
    }

    std::unique_ptr<juce::XmlElement> xml(legacyState.createXml());
    juce::MemoryBlock block;
    juce::AudioProcessor::copyXmlToBinary(*xml, block);

    chordsynth::ChordSynthAudioProcessor restored;
    auto* enabled = dynamic_cast<juce::AudioParameterBool*>(
        restored.getAPVTS().getParameter(parameters::ids::arpEnabled));
    auto* mode = dynamic_cast<juce::AudioParameterChoice*>(
        restored.getAPVTS().getParameter(parameters::ids::arpMode));
    auto* rate = dynamic_cast<juce::AudioParameterChoice*>(
        restored.getAPVTS().getParameter(parameters::ids::arpRate));
    auto* gate = dynamic_cast<juce::AudioParameterFloat*>(
        restored.getAPVTS().getParameter(parameters::ids::arpGate));

    REQUIRE(enabled != nullptr);
    REQUIRE(mode != nullptr);
    REQUIRE(rate != nullptr);
    REQUIRE(gate != nullptr);

    // Set non-defaults before loading
    *enabled = true;
    *mode = 3;
    *rate = 0;
    *gate = 0.2f;

    restored.setStateInformation(block.getData(), static_cast<int>(block.getSize()));

    REQUIRE(*enabled == false);
    REQUIRE(mode->getIndex() == 0);
    REQUIRE(rate->getIndex() == 1);
    REQUIRE(static_cast<float>(*gate) == Catch::Approx(0.8f));

    auto* restoredKey = dynamic_cast<juce::AudioParameterChoice*>(
        restored.getAPVTS().getParameter(parameters::ids::key));
    REQUIRE(restoredKey != nullptr);
    REQUIRE(restoredKey->getIndex() == 11);
}

TEST_CASE("MIDI Performance parameters have stable continuous/discrete automatable contracts and persist",
          "[parameters][midi_performance]") {
    ChordSynthAudioProcessor processor;
    auto& apvts = processor.getAPVTS();

    SECTION("Performance MIDI Enabled contract and defaults") {
        auto* midiEnabled = dynamic_cast<juce::AudioParameterBool*>(
            apvts.getParameter(parameters::ids::performanceMidiEnabled));
        REQUIRE(midiEnabled != nullptr);

        const juce::AudioProcessorParameter& hostParameter = *midiEnabled;
        REQUIRE(midiEnabled->getParameterID() == parameters::ids::performanceMidiEnabled);
        REQUIRE(midiEnabled->getVersionHint() == parameters::performanceMidiEnabledParameterVersion);
        REQUIRE(midiEnabled->name == parameters::names::performanceMidiEnabled);
        REQUIRE(hostParameter.isBoolean());
        REQUIRE(hostParameter.isDiscrete());
        REQUIRE(hostParameter.getDefaultValue() == Catch::Approx(0.0f));
        REQUIRE(hostParameter.getValue() == Catch::Approx(0.0f));
        REQUIRE(*midiEnabled == false);
    }

    SECTION("Transform Palette choice contract, choices and default Lo-Fi") {
        auto* palette = dynamic_cast<juce::AudioParameterChoice*>(
            apvts.getParameter(parameters::ids::transformPalette));
        REQUIRE(palette != nullptr);

        const juce::StringArray expectedChoices{"Basic", "Lo-Fi", "Spice"};
        const juce::AudioProcessorParameter& hostParameter = *palette;
        REQUIRE(palette->choices == expectedChoices);
        REQUIRE(palette->getParameterID() == parameters::ids::transformPalette);
        REQUIRE(palette->getVersionHint() == parameters::transformPaletteParameterVersion);
        REQUIRE(palette->name == parameters::names::transformPalette);
        REQUIRE(hostParameter.getNumSteps() == 3);
        REQUIRE(hostParameter.isDiscrete());
        REQUIRE_FALSE(hostParameter.isBoolean());
        REQUIRE(palette->getIndex() == 1); // Default Lo-Fi
    }

    SECTION("Round-trip persistence preserves non-default values") {
        auto* midiEnabled = dynamic_cast<juce::AudioParameterBool*>(
            apvts.getParameter(parameters::ids::performanceMidiEnabled));
        auto* palette = dynamic_cast<juce::AudioParameterChoice*>(
            apvts.getParameter(parameters::ids::transformPalette));
        REQUIRE(midiEnabled != nullptr);
        REQUIRE(palette != nullptr);

        *midiEnabled = true;
        *palette = 2; // Spice

        juce::MemoryBlock stateBlock;
        processor.getStateInformation(stateBlock);
        REQUIRE(stateBlock.getSize() > 0);

        ChordSynthAudioProcessor restoredProcessor;
        restoredProcessor.setStateInformation(
            stateBlock.getData(), static_cast<int>(stateBlock.getSize()));

        auto* restoredMidiEnabled = dynamic_cast<juce::AudioParameterBool*>(
            restoredProcessor.getAPVTS().getParameter(parameters::ids::performanceMidiEnabled));
        auto* restoredPalette = dynamic_cast<juce::AudioParameterChoice*>(
            restoredProcessor.getAPVTS().getParameter(parameters::ids::transformPalette));
        REQUIRE(restoredMidiEnabled != nullptr);
        REQUIRE(restoredPalette != nullptr);

        REQUIRE(*restoredMidiEnabled == true);
        REQUIRE(restoredPalette->getIndex() == 2);
    }
}

TEST_CASE("Legacy state without MIDI performance parameters restores new defaults",
          "[parameters][midi_performance][legacy]") {
    chordsynth::ChordSynthAudioProcessor legacySource;
    auto* key = dynamic_cast<juce::AudioParameterChoice*>(
        legacySource.getAPVTS().getParameter(parameters::ids::key));
    if (key != nullptr) *key = 5;

    auto legacyState = legacySource.getAPVTS().copyState();
    for (int i = legacyState.getNumChildren() - 1; i >= 0; --i) {
        auto child = legacyState.getChild(i);
        const auto id = child.getProperty("id").toString();
        if (id == parameters::ids::performanceMidiEnabled || id == parameters::ids::transformPalette)
            legacyState.removeChild(i, nullptr);
    }

    std::unique_ptr<juce::XmlElement> xml(legacyState.createXml());
    juce::MemoryBlock block;
    juce::AudioProcessor::copyXmlToBinary(*xml, block);

    chordsynth::ChordSynthAudioProcessor restored;
    auto* midiEnabled = dynamic_cast<juce::AudioParameterBool*>(
        restored.getAPVTS().getParameter(parameters::ids::performanceMidiEnabled));
    auto* palette = dynamic_cast<juce::AudioParameterChoice*>(
        restored.getAPVTS().getParameter(parameters::ids::transformPalette));

    REQUIRE(midiEnabled != nullptr);
    REQUIRE(palette != nullptr);

    // Set conflicting non-defaults before loading
    *midiEnabled = true;
    *palette = 0; // Basic

    restored.setStateInformation(block.getData(), static_cast<int>(block.getSize()));

    REQUIRE(*midiEnabled == false);
    REQUIRE(palette->getIndex() == 1); // Restores default Lo-Fi

    auto* restoredKey = dynamic_cast<juce::AudioParameterChoice*>(
        restored.getAPVTS().getParameter(parameters::ids::key));
    REQUIRE(restoredKey != nullptr);
    REQUIRE(restoredKey->getIndex() == 5);
}

TEST_CASE("Processor ignores semantically invalid parameter children in an otherwise valid state",
          "[parameters][robustness][persistence]") {
    ChordSynthAudioProcessor source;
    auto* sourceKey = dynamic_cast<juce::AudioParameterChoice*>(
        source.getAPVTS().getParameter(parameters::ids::key));
    REQUIRE(sourceKey != nullptr);
    *sourceKey = 7;

    auto state = source.getAPVTS().copyState();
    juce::ValueTree invalidChild{"NOT_PARAM"};
    invalidChild.setProperty("id", parameters::ids::key, nullptr);
    invalidChild.setProperty("value", 0.0f, nullptr);
    state.appendChild(invalidChild, nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    juce::MemoryBlock block;
    juce::AudioProcessor::copyXmlToBinary(*xml, block);

    ChordSynthAudioProcessor restored;
    restored.setStateInformation(block.getData(), static_cast<int>(block.getSize()));

    auto* restoredKey = dynamic_cast<juce::AudioParameterChoice*>(
        restored.getAPVTS().getParameter(parameters::ids::key));
    REQUIRE(restoredKey != nullptr);
    REQUIRE(restoredKey->getIndex() == 7);
}

TEST_CASE("Processor handles malformed, null, or wrong-type state blobs safely",
          "[parameters][robustness]") {
    ChordSynthAudioProcessor processor;
    auto* keyParam = dynamic_cast<juce::AudioParameterChoice*>(
        processor.getAPVTS().getParameter(parameters::ids::key));
    REQUIRE(keyParam != nullptr);
    *keyParam = 3;

    // 1. Null data or 0 size does not crash or corrupt
    processor.setStateInformation(nullptr, 0);
    REQUIRE(keyParam->getIndex() == 3);

    // 2. Garbage binary data
    const char garbage[] = "NOT_AN_XML_OR_VALID_STATE_BLOB";
    processor.setStateInformation(garbage, sizeof(garbage));
    REQUIRE(keyParam->getIndex() == 3);

    // 3. XML with wrong root tag
    juce::XmlElement wrongRoot("WrongRootType");
    juce::MemoryBlock wrongBlock;
    juce::AudioProcessor::copyXmlToBinary(wrongRoot, wrongBlock);
    processor.setStateInformation(wrongBlock.getData(), static_cast<int>(wrongBlock.getSize()));
    REQUIRE(keyParam->getIndex() == 3);
}
