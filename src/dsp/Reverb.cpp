#include "Reverb.h"
#include <algorithm>
#include <cmath>

namespace chordsynth::dsp {

void Reverb::prepare(double sampleRate, int maximumBlockSize, int channels,
                     float initialMix, float initialRoomSize,
                     float initialDamping, float initialWidth)
{
    juce::ignoreUnused(maximumBlockSize);
    currentSampleRate = std::isfinite(sampleRate) && sampleRate > 0.0 ? sampleRate : 48000.0;
    preparedChannels = std::max(0, channels);

    const juce::dsp::ProcessSpec spec{
        currentSampleRate,
        static_cast<juce::uint32>(std::max(1, maximumBlockSize)),
        static_cast<juce::uint32>(std::max(1, preparedChannels))};
    reverbDsp.prepare(spec);

    mixSmoothed.reset(currentSampleRate, smoothingSeconds);
    const auto safeMix = sanitiseMix(initialMix);
    mixSmoothed.setCurrentAndTargetValue(safeMix);

    targetRoomSize = sanitiseRoomSize(initialRoomSize);
    targetDamping = sanitiseDamping(initialDamping);
    targetWidth = sanitiseWidth(initialWidth);

    juce::dsp::Reverb::Parameters params;
    params.roomSize = targetRoomSize;
    params.damping = targetDamping;
    params.wetLevel = safeMix;
    params.dryLevel = 1.0f - safeMix;
    params.width = targetWidth;
    params.freezeMode = 0.0f;

    reverbDsp.reset();
    reverbDsp.setParameters(params);
}

void Reverb::reset() noexcept
{
    reverbDsp.reset();
}

void Reverb::setTargetParameters(float mix, float roomSize, float damping, float width) noexcept
{
    mixSmoothed.setTargetValue(sanitiseMix(mix));
    targetRoomSize = sanitiseRoomSize(roomSize);
    targetDamping = sanitiseDamping(damping);
    targetWidth = sanitiseWidth(width);

    juce::dsp::Reverb::Parameters params = reverbDsp.getParameters();
    params.roomSize = targetRoomSize;
    params.damping = targetDamping;
    params.width = targetWidth;
    reverbDsp.setParameters(params);
}

void Reverb::process(juce::AudioBuffer<float>& buffer) noexcept
{
    const auto channels = std::min(buffer.getNumChannels(), preparedChannels);
    if (channels <= 0 || buffer.getNumSamples() <= 0)
        return;

    const bool isBypassed = mixSmoothed.getCurrentValue() <= 0.0f && mixSmoothed.getTargetValue() <= 0.0f;
    if (isBypassed)
        return;

    if (mixSmoothed.isSmoothing()) {
        mixSmoothed.skip(buffer.getNumSamples());
        const float currentMix = mixSmoothed.getCurrentValue();
        juce::dsp::Reverb::Parameters params = reverbDsp.getParameters();
        params.wetLevel = currentMix;
        params.dryLevel = 1.0f - currentMix;
        reverbDsp.setParameters(params);
    }

    juce::dsp::AudioBlock<float> block(buffer);
    auto subBlock = block.getSubBlock(0, static_cast<size_t>(buffer.getNumSamples()));
    juce::dsp::ProcessContextReplacing<float> context(subBlock);
    reverbDsp.process(context);
}

float Reverb::sanitiseMix(float value) noexcept
{
    if (!std::isfinite(value))
        value = defaultMix;
    return std::clamp(value, minimumMix, maximumMix);
}

float Reverb::sanitiseRoomSize(float value) noexcept
{
    if (!std::isfinite(value))
        value = defaultRoomSize;
    return std::clamp(value, minimumRoomSize, maximumRoomSize);
}

float Reverb::sanitiseDamping(float value) noexcept
{
    if (!std::isfinite(value))
        value = defaultDamping;
    return std::clamp(value, minimumDamping, maximumDamping);
}

float Reverb::sanitiseWidth(float value) noexcept
{
    if (!std::isfinite(value))
        value = defaultWidth;
    return std::clamp(value, minimumWidth, maximumWidth);
}

} // namespace chordsynth::dsp
