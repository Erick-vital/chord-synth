#include "Filter.h"
#include <algorithm>
#include <cmath>

namespace chordsynth::dsp {

void Filter::prepare(double sampleRate, int maximumBlockSize, int channels,
                     float initialCutoffHz, float initialResonance)
{
    currentSampleRate = std::isfinite(sampleRate) && sampleRate > 0.0 ? sampleRate : 48000.0;
    preparedChannels = std::max(0, channels);
    const juce::dsp::ProcessSpec spec{
        currentSampleRate,
        static_cast<juce::uint32>(std::max(1, maximumBlockSize)),
        static_cast<juce::uint32>(std::max(1, preparedChannels))};
    stateVariableFilter.prepare(spec);
    stateVariableFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);

    cutoff.reset(currentSampleRate, smoothingSeconds);
    resonance.reset(currentSampleRate, smoothingSeconds);
    cutoff.setCurrentAndTargetValue(sanitiseCutoff(initialCutoffHz));
    resonance.setCurrentAndTargetValue(sanitiseResonance(initialResonance));
    stateVariableFilter.setCutoffFrequency(cutoff.getCurrentValue());
    stateVariableFilter.setResonance(resonance.getCurrentValue());
    stateVariableFilter.reset();
}

void Filter::reset() noexcept
{
    stateVariableFilter.reset();
}

void Filter::setTargetParameters(float cutoffHz, float newResonance) noexcept
{
    cutoff.setTargetValue(sanitiseCutoff(cutoffHz));
    resonance.setTargetValue(sanitiseResonance(newResonance));
}

void Filter::process(juce::AudioBuffer<float>& buffer) noexcept
{
    const auto channels = std::min(buffer.getNumChannels(), preparedChannels);
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        stateVariableFilter.setCutoffFrequency(cutoff.getNextValue());
        stateVariableFilter.setResonance(resonance.getNextValue());
        for (int channel = 0; channel < channels; ++channel) {
            const auto filtered = stateVariableFilter.processSample(
                channel, buffer.getSample(channel, sample));
            buffer.setSample(channel, sample, filtered);
        }
    }
}

float Filter::sanitiseCutoff(float value) const noexcept
{
    if (!std::isfinite(value))
        value = defaultCutoffHz;
    // TPT coefficients require a value strictly below Nyquist. 49% of the
    // sample rate leaves a small numerical margin while retaining the range.
    const auto sampleRateLimit = static_cast<float>(currentSampleRate * 0.49);
    const auto upper = std::max(0.001f, std::min(maximumCutoffHz, sampleRateLimit));
    const auto lower = std::min(minimumCutoffHz, upper);
    return std::clamp(value, lower, upper);
}

float Filter::sanitiseResonance(float value) noexcept
{
    if (!std::isfinite(value))
        value = defaultResonance;
    return std::clamp(value, minimumResonance, maximumResonance);
}

} // namespace chordsynth::dsp