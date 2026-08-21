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
