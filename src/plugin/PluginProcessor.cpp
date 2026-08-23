#include "PluginProcessor.h"
#include "parameters/ParameterIds.h"
#if !JUCE_HEADLESS_PLUGIN_CLIENT
#include "PluginEditor.h"
#endif

#include <cerrno>
#include <cmath>
#include <cstdlib>

namespace {

bool isFiniteNumericValue(const juce::var& value)
{
    if (value.isInt() || value.isInt64() || value.isDouble() || value.isBool())
        return std::isfinite(static_cast<double>(value));

    if (!value.isString())
        return false;

    const auto text = value.toString();
    const auto* begin = text.toRawUTF8();
    char* end = nullptr;
    errno = 0;
    const auto parsed = std::strtod(begin, &end);
    return end != begin && *end == '\0' && errno != ERANGE && std::isfinite(parsed);
}

void removeUnsafeParameterChildren(juce::ValueTree& state,
                                   chordsynth::parameters::AudioProcessorValueTreeState& apvts)
{
    juce::StringArray restoredIds;

    for (int index = 0; index < state.getNumChildren();) {
        const auto child = state.getChild(index);
        const auto hasParameterId = child.hasProperty("id");
        const auto isParameter = child.hasType("PARAM");

        if (!isParameter) {
            if (hasParameterId)
                state.removeChild(index, nullptr);
            else
                ++index;
            continue;
        }

        const auto id = child.getProperty("id").toString();
        const auto value = child.getProperty("value");
        const auto isKnownParameter = apvts.getParameter(id) != nullptr;
        const auto isDuplicate = restoredIds.contains(id);

        if (!isKnownParameter || isDuplicate || !isFiniteNumericValue(value)) {
            state.removeChild(index, nullptr);
            continue;
        }

        restoredIds.add(id);
        ++index;
    }
}

constexpr int maxRealtimeMidiEvents = 256;

bool isRealtimeSafeMidiMessage(const juce::MidiMessage& message) noexcept
{
    return !message.isSysEx() && message.getRawDataSize() <= 3;
}

int appendRealtimeSafeEvents(const juce::MidiBuffer& source,
                             juce::MidiBuffer& destination,
                             int maxEvents,
                             int numSamples) noexcept
{
    int copied = 0;
    int inspected = 0;
    for (const auto metadata : source) {
        // Bound total iterator work, including malformed/unsupported events.
        // Bounding accepted events alone still permits an unbounded scan on the
        // audio thread when a host supplies only rejected MIDI.
        if (inspected == maxEvents) {
            break;
        }
        ++inspected;
        if (metadata.samplePosition < 0 || metadata.samplePosition >= numSamples) {
            continue;
        }
        const auto message = metadata.getMessage();
        if (!isRealtimeSafeMidiMessage(message)) {
            continue;
        }
        destination.addEvent(message, metadata.samplePosition);
        ++copied;
    }
    return copied;
}

} // namespace

namespace chordsynth {

ChordSynthAudioProcessor::ChordSynthAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      midiPerformanceMapper(harmonyState.getConfiguration(), chordVoicer),
      apvts(*this, nullptr, parameters::stateRootType, parameters::createParameterLayout())
{
    synth.addSound(new dsp::ChordSound());
    for (int i = 0; i < numVoices; ++i)
        synth.addVoice(new dsp::ChordVoice());
    waveformParameter = apvts.getRawParameterValue(parameters::ids::waveform);
    cutoffParameter = apvts.getRawParameterValue(parameters::ids::cutoff);
    resonanceParameter = apvts.getRawParameterValue(parameters::ids::resonance);
    detuneParameter = apvts.getRawParameterValue(parameters::ids::detune);
    chorusMixParameter = apvts.getRawParameterValue(parameters::ids::chorusMix);
    chorusRateParameter = apvts.getRawParameterValue(parameters::ids::chorusRate);
    chorusDepthParameter = apvts.getRawParameterValue(parameters::ids::chorusDepth);
    delayMixParameter = apvts.getRawParameterValue(parameters::ids::delayMix);
    delayFeedbackParameter = apvts.getRawParameterValue(parameters::ids::delayFeedback);
    delayTimeMsParameter = apvts.getRawParameterValue(parameters::ids::delayTimeMs);
    delaySyncParameter = apvts.getRawParameterValue(parameters::ids::delaySync);
    delaySyncRateParameter = apvts.getRawParameterValue(parameters::ids::delaySyncRate);
    reverbMixParameter = apvts.getRawParameterValue(parameters::ids::reverbMix);
    reverbRoomSizeParameter = apvts.getRawParameterValue(parameters::ids::reverbRoomSize);
    reverbDampingParameter = apvts.getRawParameterValue(parameters::ids::reverbDamping);
    reverbWidthParameter = apvts.getRawParameterValue(parameters::ids::reverbWidth);
    arpEnabledParameter = apvts.getRawParameterValue(parameters::ids::arpEnabled);
    arpModeParameter = apvts.getRawParameterValue(parameters::ids::arpMode);
    arpRateParameter = apvts.getRawParameterValue(parameters::ids::arpRate);
    arpGateParameter = apvts.getRawParameterValue(parameters::ids::arpGate);
    keyParameter = apvts.getRawParameterValue(parameters::ids::key);
    scaleParameter = apvts.getRawParameterValue(parameters::ids::scale);
    performanceMidiEnabledParameter = apvts.getRawParameterValue(parameters::ids::performanceMidiEnabled);
    transformPaletteParameter = apvts.getRawParameterValue(parameters::ids::transformPalette);
    jassert(waveformParameter != nullptr);
    jassert(cutoffParameter != nullptr);
    jassert(resonanceParameter != nullptr);
    jassert(detuneParameter != nullptr);
    jassert(chorusMixParameter != nullptr);
    jassert(chorusRateParameter != nullptr);
    jassert(chorusDepthParameter != nullptr);
    jassert(delayMixParameter != nullptr);
    jassert(delayFeedbackParameter != nullptr);
    jassert(delayTimeMsParameter != nullptr);
    jassert(delaySyncParameter != nullptr);
    jassert(delaySyncRateParameter != nullptr);
    jassert(reverbMixParameter != nullptr);
    jassert(reverbRoomSizeParameter != nullptr);
    jassert(reverbDampingParameter != nullptr);
    jassert(reverbWidthParameter != nullptr);
    jassert(arpEnabledParameter != nullptr);
    jassert(arpModeParameter != nullptr);
    jassert(arpRateParameter != nullptr);
    jassert(arpGateParameter != nullptr);
    jassert(keyParameter != nullptr);
    jassert(scaleParameter != nullptr);
    jassert(performanceMidiEnabledParameter != nullptr);
    jassert(transformPaletteParameter != nullptr);
    workingMidi.ensureSize(interaction::MidiPerformanceMapper::realtimeMidiBufferBytes);
    synthMidiScratch.ensureSize(interaction::MidiPerformanceMapper::realtimeMidiBufferBytes);
}

ChordSynthAudioProcessor::~ChordSynthAudioProcessor()
{
}

const juce::String ChordSynthAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ChordSynthAudioProcessor::acceptsMidi() const
{
    return true;
}

bool ChordSynthAudioProcessor::producesMidi() const
{
    return false;
}

bool ChordSynthAudioProcessor::isMidiEffect() const
{
    return false;
}

double ChordSynthAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ChordSynthAudioProcessor::getNumPrograms()
{
    return 1;
}

int ChordSynthAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ChordSynthAudioProcessor::setCurrentProgram(int)
{
}

const juce::String ChordSynthAudioProcessor::getProgramName(int)
{
    return {};
}

void ChordSynthAudioProcessor::changeProgramName(int, const juce::String&)
{
}

void ChordSynthAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);
    const auto cutoff = cutoffParameter != nullptr
        ? cutoffParameter->load(std::memory_order_relaxed) : dsp::Filter::defaultCutoffHz;
    const auto resonance = resonanceParameter != nullptr
        ? resonanceParameter->load(std::memory_order_relaxed) : dsp::Filter::defaultResonance;
    globalFilter.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels(), cutoff, resonance);

    const auto chorusMix = chorusMixParameter != nullptr
        ? chorusMixParameter->load(std::memory_order_relaxed) : dsp::Chorus::defaultMix;
    const auto chorusRate = chorusRateParameter != nullptr
        ? chorusRateParameter->load(std::memory_order_relaxed) : dsp::Chorus::defaultRateHz;
    const auto chorusDepth = chorusDepthParameter != nullptr
        ? chorusDepthParameter->load(std::memory_order_relaxed) : dsp::Chorus::defaultDepth;
    chorus.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels(), chorusMix, chorusRate, chorusDepth);

    const auto delayMix = delayMixParameter != nullptr
        ? delayMixParameter->load(std::memory_order_relaxed) : dsp::TempoDelay::defaultMix;
    const auto delayFeedback = delayFeedbackParameter != nullptr
        ? delayFeedbackParameter->load(std::memory_order_relaxed) : dsp::TempoDelay::defaultFeedback;
    const auto delayTimeMs = delayTimeMsParameter != nullptr
        ? delayTimeMsParameter->load(std::memory_order_relaxed) : dsp::TempoDelay::defaultTimeMs;
    const auto rawDelaySync = delaySyncParameter != nullptr
        ? delaySyncParameter->load(std::memory_order_relaxed) : 1.0f;
    const auto rawDelaySyncRate = delaySyncRateParameter != nullptr
        ? delaySyncRateParameter->load(std::memory_order_relaxed) : 0.0f;
    delay.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels(),
                  delayMix, delayFeedback, delayTimeMs,
                  rawDelaySync > 0.5f, dsp::delaySyncRateFromRawChoice(rawDelaySyncRate));

    const auto reverbMix = reverbMixParameter != nullptr
        ? reverbMixParameter->load(std::memory_order_relaxed) : dsp::Reverb::defaultMix;
    const auto reverbRoomSize = reverbRoomSizeParameter != nullptr
        ? reverbRoomSizeParameter->load(std::memory_order_relaxed) : dsp::Reverb::defaultRoomSize;
    const auto reverbDamping = reverbDampingParameter != nullptr
        ? reverbDampingParameter->load(std::memory_order_relaxed) : dsp::Reverb::defaultDamping;
    const auto reverbWidth = reverbWidthParameter != nullptr
        ? reverbWidthParameter->load(std::memory_order_relaxed) : dsp::Reverb::defaultWidth;
    reverb.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels(),
                   reverbMix, reverbRoomSize, reverbDamping, reverbWidth);

    arpeggiator.prepare(sampleRate);
    const auto rawArpEnabled = arpEnabledParameter != nullptr
        ? arpEnabledParameter->load(std::memory_order_relaxed) : 0.0f;
    const auto rawArpMode = arpModeParameter != nullptr
        ? arpModeParameter->load(std::memory_order_relaxed) : 0.0f;
    const auto rawArpRate = arpRateParameter != nullptr
        ? arpRateParameter->load(std::memory_order_relaxed) : 1.0f;
    const auto rawArpGate = arpGateParameter != nullptr
        ? arpGateParameter->load(std::memory_order_relaxed) : 0.8f;
    arpeggiator.setEnabled(rawArpEnabled > 0.5f);
    arpeggiator.setMode(music::arpModeFromRawChoice(rawArpMode));
    arpeggiator.setRate(music::arpRateFromRawChoice(rawArpRate));
    arpeggiator.setGate(rawArpGate);
}

void ChordSynthAudioProcessor::releaseResources()
{
    midiPerformanceMapper.reset();
}

bool ChordSynthAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void ChordSynthAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    workingMidi.clear();
    auto realtimeEventCount = appendRealtimeSafeEvents(
        midiMessages, workingMidi, maxRealtimeMidiEvents, buffer.getNumSamples());
    juce::MidiMessage uiMessage;
    while (realtimeEventCount < maxRealtimeMidiEvents && uiMidiQueue.tryPop(uiMessage)) {
        if (isRealtimeSafeMidiMessage(uiMessage)) {
            workingMidi.addEvent(uiMessage, 0);
            ++realtimeEventCount;
        }
    }

    // Update and process MIDI Performance Mapper
    const auto rawMidiPerfEnabled = performanceMidiEnabledParameter != nullptr
        ? performanceMidiEnabledParameter->load(std::memory_order_relaxed) : 0.0f;
    const bool isMidiPerfActive = rawMidiPerfEnabled > 0.5f;
    midiPerformanceMapper.setEnabled(isMidiPerfActive);

    if (isMidiPerfActive) {
        const auto rawKey = keyParameter != nullptr
            ? keyParameter->load(std::memory_order_relaxed) : 0.0f;
        const auto rawScale = scaleParameter != nullptr
            ? scaleParameter->load(std::memory_order_relaxed) : 0.0f;
        const auto rawPalette = transformPaletteParameter != nullptr
            ? transformPaletteParameter->load(std::memory_order_relaxed) : 1.0f;

        const int tonic = std::clamp(static_cast<int>(rawKey), 0, 11);
        const auto scale = rawScale >= 0.5f ? music::Scale::naturalMinor : music::Scale::major;
        const bool isDiatonic = (harmonyState.getQualityRule() == music::QualityRule::diatonic);
        const int scene = harmonyState.getSelectedScene();
        const int palIdx = std::clamp(static_cast<int>(rawPalette), 0, 2);
        auto pal = interaction::TransformPalette::loFi;
        if (palIdx == 0) pal = interaction::TransformPalette::basic;
        else if (palIdx == 2) pal = interaction::TransformPalette::spice;

        midiPerformanceMapper.setContext({
            .tonic = tonic,
            .scale = scale,
            .diatonicMode = isDiatonic,
            .sceneIndex = scene,
            .palette = pal
        });

        midiPerformanceMapper.processBlock(workingMidi, buffer.getNumSamples());
    }

    double hostBpm = music::MusicalClock::defaultBpm;
    if (auto* ph = getPlayHead()) {
        if (auto positionOpt = ph->getPosition()) {
            if (auto bpmOpt = positionOpt->getBpm()) {
                hostBpm = *bpmOpt;
            }
        }
    }

    const auto rawArpEnabled = arpEnabledParameter != nullptr
        ? arpEnabledParameter->load(std::memory_order_relaxed) : 0.0f;
    const bool isArpActive = rawArpEnabled > 0.5f;
    arpeggiator.setEnabled(isArpActive);

    synthMidiScratch.clear();
    auto& synthMidi = synthMidiScratch;
    if (isArpActive) {
        const auto rawArpMode = arpModeParameter != nullptr
            ? arpModeParameter->load(std::memory_order_relaxed) : 0.0f;
        const auto rawArpRate = arpRateParameter != nullptr
            ? arpRateParameter->load(std::memory_order_relaxed) : 1.0f;
        const auto rawArpGate = arpGateParameter != nullptr
            ? arpGateParameter->load(std::memory_order_relaxed) : 0.8f;
        arpeggiator.setMode(music::arpModeFromRawChoice(rawArpMode));
        arpeggiator.setRate(music::arpRateFromRawChoice(rawArpRate));
        arpeggiator.setGate(rawArpGate);

        // Separate channel 2 (bass / non-arpeggiated) from channel 1 and other MIDI
        for (const auto meta : workingMidi) {
            const auto msg = meta.getMessage();
            if (msg.getChannel() == 2) {
                // Pass bass directly to synthMidi without arpeggiating
                synthMidi.addEvent(msg, meta.samplePosition);
            } else {
                if (msg.isNoteOn()) {
                    arpeggiator.noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
                } else if (msg.isNoteOff()) {
                    arpeggiator.noteOff(msg.getNoteNumber());
                } else if (msg.isAllNotesOff() || msg.isAllSoundOff()) {
                    arpeggiator.allNotesOff();
                } else {
                    // Non-note MIDI messages (e.g. CC, pitch bend) pass through directly
                    synthMidi.addEvent(msg, meta.samplePosition);
                }
            }
        }

        arpeggiator.processBlock(synthMidi, buffer.getNumSamples(), hostBpm);
    } else {
        synthMidi.addEvents(workingMidi, 0, buffer.getNumSamples(), 0);
    }

    const auto rawWaveform = waveformParameter != nullptr
        ? waveformParameter->load(std::memory_order_relaxed) : 0.0f;
    synth.setWaveformForAllVoices(dsp::waveformFromRawChoice(rawWaveform));

    const auto rawDetune = detuneParameter != nullptr
        ? detuneParameter->load(std::memory_order_relaxed) : 7.0f;
    synth.setDetuneCentsForAllVoices(rawDetune);

    synth.renderNextBlock(buffer, synthMidi, 0, buffer.getNumSamples());

    const auto rawCutoff = cutoffParameter != nullptr
        ? cutoffParameter->load(std::memory_order_relaxed) : dsp::Filter::defaultCutoffHz;
    const auto rawResonance = resonanceParameter != nullptr
        ? resonanceParameter->load(std::memory_order_relaxed) : dsp::Filter::defaultResonance;
    globalFilter.setTargetParameters(rawCutoff, rawResonance);
    globalFilter.process(buffer);

    const auto rawChorusMix = chorusMixParameter != nullptr
        ? chorusMixParameter->load(std::memory_order_relaxed) : dsp::Chorus::defaultMix;
    const auto rawChorusRate = chorusRateParameter != nullptr
        ? chorusRateParameter->load(std::memory_order_relaxed) : dsp::Chorus::defaultRateHz;
    const auto rawChorusDepth = chorusDepthParameter != nullptr
        ? chorusDepthParameter->load(std::memory_order_relaxed) : dsp::Chorus::defaultDepth;
    chorus.setTargetParameters(rawChorusMix, rawChorusRate, rawChorusDepth);
    chorus.process(buffer);

    const auto rawDelayMix = delayMixParameter != nullptr
        ? delayMixParameter->load(std::memory_order_relaxed) : dsp::TempoDelay::defaultMix;
    const auto rawDelayFeedback = delayFeedbackParameter != nullptr
        ? delayFeedbackParameter->load(std::memory_order_relaxed) : dsp::TempoDelay::defaultFeedback;
    const auto rawDelayTimeMs = delayTimeMsParameter != nullptr
        ? delayTimeMsParameter->load(std::memory_order_relaxed) : dsp::TempoDelay::defaultTimeMs;
    const auto rawDelaySync = delaySyncParameter != nullptr
        ? delaySyncParameter->load(std::memory_order_relaxed) : 1.0f;
    const auto rawDelaySyncRate = delaySyncRateParameter != nullptr
        ? delaySyncRateParameter->load(std::memory_order_relaxed) : 0.0f;

    delay.setTargetParameters(rawDelayMix, rawDelayFeedback, rawDelayTimeMs,
                              rawDelaySync > 0.5f, dsp::delaySyncRateFromRawChoice(rawDelaySyncRate));
    delay.process(buffer, hostBpm);

    const auto rawReverbMix = reverbMixParameter != nullptr
        ? reverbMixParameter->load(std::memory_order_relaxed) : dsp::Reverb::defaultMix;
    const auto rawReverbRoomSize = reverbRoomSizeParameter != nullptr
        ? reverbRoomSizeParameter->load(std::memory_order_relaxed) : dsp::Reverb::defaultRoomSize;
    const auto rawReverbDamping = reverbDampingParameter != nullptr
        ? reverbDampingParameter->load(std::memory_order_relaxed) : dsp::Reverb::defaultDamping;
    const auto rawReverbWidth = reverbWidthParameter != nullptr
        ? reverbWidthParameter->load(std::memory_order_relaxed) : dsp::Reverb::defaultWidth;
    reverb.setTargetParameters(rawReverbMix, rawReverbRoomSize, rawReverbDamping, rawReverbWidth);
    reverb.process(buffer);
}

bool ChordSynthAudioProcessor::hasEditor() const
{
#if JUCE_HEADLESS_PLUGIN_CLIENT
    return false;
#else
    return true;
#endif
}

juce::AudioProcessorEditor* ChordSynthAudioProcessor::createEditor()
{
#if JUCE_HEADLESS_PLUGIN_CLIENT
    return nullptr;
#else
    return new ChordSynthAudioProcessorEditor(*this);
#endif
}

void ChordSynthAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.appendChild(harmonyState.toValueTree(), nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ChordSynthAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0)
        return;

    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType())) {
        auto incomingState = juce::ValueTree::fromXml(*xmlState);
        removeUnsafeParameterChildren(incomingState, apvts);

        const auto addDefaultIfMissing = [&incomingState](const char* id, float value) {
            for (const auto& child : incomingState)
                if (child.getProperty("id").toString() == id)
                    return;
            juce::ValueTree parameterState{"PARAM"};
            parameterState.setProperty("id", id, nullptr);
            parameterState.setProperty("value", value, nullptr);
            incomingState.appendChild(parameterState, nullptr);
        };
        addDefaultIfMissing(parameters::ids::waveform, 0.0f);
        addDefaultIfMissing(parameters::ids::scale, 0.0f);
        addDefaultIfMissing(parameters::ids::cutoff, 8000.0f);
        addDefaultIfMissing(parameters::ids::resonance, 0.2f);
        addDefaultIfMissing(parameters::ids::detune, 7.0f);
        addDefaultIfMissing(parameters::ids::chorusMix, 0.0f);
        addDefaultIfMissing(parameters::ids::chorusRate, 1.0f);
        addDefaultIfMissing(parameters::ids::chorusDepth, 0.25f);
        addDefaultIfMissing(parameters::ids::delayMix, 0.0f);
        addDefaultIfMissing(parameters::ids::delayFeedback, 0.3f);
        addDefaultIfMissing(parameters::ids::delayTimeMs, 250.0f);
        addDefaultIfMissing(parameters::ids::delaySync, 1.0f);
        addDefaultIfMissing(parameters::ids::delaySyncRate, 0.0f);
        addDefaultIfMissing(parameters::ids::reverbMix, 0.0f);
        addDefaultIfMissing(parameters::ids::reverbRoomSize, 0.5f);
        addDefaultIfMissing(parameters::ids::reverbDamping, 0.5f);
        addDefaultIfMissing(parameters::ids::reverbWidth, 1.0f);
        addDefaultIfMissing(parameters::ids::arpEnabled, 0.0f);
        addDefaultIfMissing(parameters::ids::arpMode, 0.0f);
        addDefaultIfMissing(parameters::ids::arpRate, 1.0f);
        addDefaultIfMissing(parameters::ids::arpGate, 0.8f);

        auto harmonyChild = incomingState.getChildWithName(state::stateTag);
        if (harmonyChild.isValid()) {
            if (!harmonyState.loadFromValueTree(harmonyChild)) {
                harmonyState.resetToDefaults();
            }
        } else {
            harmonyState.resetToDefaults();
        }

        apvts.replaceState(incomingState);
    }
}

} // namespace chordsynth

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new chordsynth::ChordSynthAudioProcessor();
}
