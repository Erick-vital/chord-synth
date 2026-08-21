#include "NoteMath.h"
#include <cmath>

namespace chordsynth::music {

double midiToFrequency(int midiNoteNumber) noexcept
{
    return 440.0 * std::pow(2.0, (static_cast<double>(midiNoteNumber) - 69.0) / 12.0);
}

} // namespace chordsynth::music
