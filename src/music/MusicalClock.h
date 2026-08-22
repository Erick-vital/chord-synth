#pragma once

#include <algorithm>
#include <cmath>

namespace chordsynth::music {

enum class ArpRate {
    quarter = 0,   // 1/4 (1 beat)
    eighth = 1,    // 1/8 (0.5 beats)
    sixteenth = 2  // 1/16 (0.25 beats)
};

[[nodiscard]] inline ArpRate arpRateFromRawChoice(float rawChoice) noexcept {
    int choice = 0;
    if (std::isfinite(rawChoice)) {
        if (rawChoice >= 2.0f)
            choice = 2;
        else if (rawChoice > 0.0f)
            choice = static_cast<int>(rawChoice + 0.5f);
    }
    switch (choice) {
        case 1: return ArpRate::eighth;
        case 2: return ArpRate::sixteenth;
        case 0:
        default: return ArpRate::quarter;
    }
}

class MusicalClock final {
public:
    static constexpr double defaultBpm = 120.0;

    void prepare(double sampleRate) noexcept {
        currentSampleRate = std::isfinite(sampleRate) && sampleRate > 0.0 ? sampleRate : 48000.0;
    }

    void setRate(ArpRate rate) noexcept {
        currentRate = rate;
    }

    [[nodiscard]] ArpRate getRate() const noexcept {
        return currentRate;
    }

    [[nodiscard]] int getSamplesPerStep(double bpm) const noexcept {
        const double safeBpm = sanitiseBpm(bpm);
        const double secondsPerBeat = 60.0 / safeBpm;
        double stepSeconds = secondsPerBeat;

        switch (currentRate) {
            case ArpRate::quarter:
                stepSeconds = secondsPerBeat;
                break;
            case ArpRate::eighth:
                stepSeconds = secondsPerBeat * 0.5;
                break;
            case ArpRate::sixteenth:
                stepSeconds = secondsPerBeat * 0.25;
                break;
        }

        const int samples = static_cast<int>(std::round(stepSeconds * currentSampleRate));
        return std::max(1, samples);
    }

private:
    [[nodiscard]] static double sanitiseBpm(double value) noexcept {
        if (!std::isfinite(value) || value <= 1.0)
            return defaultBpm;
        return std::clamp(value, 20.0, 400.0);
    }

    double currentSampleRate{48000.0};
    ArpRate currentRate{ArpRate::eighth};
};

} // namespace chordsynth::music
