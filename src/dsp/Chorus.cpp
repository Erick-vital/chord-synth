#include "Chorus.h"
#include <algorithm>
#include <cmath>

namespace chordsynth::dsp {

void Chorus::prepare(double sampleRate, int maximumBlockSize, int channels,
                     float initialMix, float initialRateHz, float initialDepth)
{
    currentSampleRate = std::isfinite(sampleRate) && sampleRate > 0.0 ? sampleRate : 48000.0;
    preparedChannels = std::max(0, channels);

    const juce::dsp::ProcessSpec spec{
        currentSampleRate,
        static_cast<juce::uint32>(std::max(1, maximumBlockSize)),
        static_cast<juce::uint32>(std::max(1, preparedChannels))};
    chorusDsp.prepare(spec);

    mixSmoothed.reset(currentSampleRate, smoothingSeconds);
    const auto safeMix = sanitiseMix(initialMix);
    mixSmoothed.setCurrentAndTargetValue(safeMix);

    targetRateHz = sanitiseRate(initialRateHz);
    targetDepth = sanitiseDepth(initialDepth);

    chorusDsp.setRate(targetRateHz);
    chorusDsp.setDepth(targetDepth);
    chorusDsp.setCentreDelay(7.0f);
    chorusDsp.setFeedback(0.0f);
    chorusDsp.setMix(safeMix);
    chorusDsp.reset();
}

void Chorus::reset() noexcept
{
    chorusDsp.reset();
}

void Chorus::setTargetParameters(float mix, float rateHz, float depth) noexcept
{
    mixSmoothed.setTargetValue(sanitiseMix(mix));
    targetRateHz = sanitiseRate(rateHz);
    targetDepth = sanitiseDepth(depth);

    chorusDsp.setRate(targetRateHz);
    chorusDsp.setDepth(targetDepth);
}

void Chorus::process(juce::AudioBuffer<float>& buffer) noexcept
{
    const auto channels = std::min(buffer.getNumChannels(), preparedChannels);
    if (channels <= 0 || buffer.getNumSamples() <= 0)
        return;

    // If both current and target mix are 0, completely bypass DSP processing
    if (mixSmoothed.getCurrentValue() <= 0.0f && mixSmoothed.getTargetValue() <= 0.0f)
        return;

    // Advance smoothed mix per block (or per sample)
    if (mixSmoothed.isSmoothing()) {
        mixSmoothed.skip(buffer.getNumSamples());
        chorusDsp.setMix(mixSmoothed.getCurrentValue());
    }

    juce::dsp::AudioBlock<float> block(buffer);
    auto subBlock = block.getSubBlock(0, static_cast<size_t>(buffer.getNumSamples()));
    juce::dsp::ProcessContextReplacing<float> context(subBlock);
    chorusDsp.process(context);
}

float Chorus::sanitiseMix(float value) noexcept
{
    if (!std::isfinite(value))
        value = defaultMix;
    return std::clamp(value, minimumMix, maximumMix);
}

float Chorus::sanitiseRate(float value) noexcept
{
    if (!std::isfinite(value))
        value = defaultRateHz;
    return std::clamp(value, minimumRateHz, maximumRateHz);
}

float Chorus::sanitiseDepth(float value) noexcept
{
    if (!std::isfinite(value))
        value = defaultDepth;
    return std::clamp(value, minimumDepth, maximumDepth);
}

} // namespace chordsynth::dsp
