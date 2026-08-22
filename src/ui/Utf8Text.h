#pragma once

#include <juce_core/juce_core.h>

namespace chordsynth::ui {

inline juce::String utf8(const char* text)
{
    return juce::String::fromUTF8(text);
}

} // namespace chordsynth::ui
