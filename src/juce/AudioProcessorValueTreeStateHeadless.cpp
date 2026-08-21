#include "AudioProcessorValueTreeStateHeadless.h"

// Project-unique stand-ins satisfy unreachable GUI constructors emitted by the
// upstream APVTS implementation without defining JUCE's public attachment types.
namespace juce {
class ChordSynthHeadlessSliderParameterAttachment {
public:
    ChordSynthHeadlessSliderParameterAttachment(RangedAudioParameter&, Slider&, UndoManager*) { jassertfalse; }
};

class ChordSynthHeadlessComboBoxParameterAttachment {
public:
    ChordSynthHeadlessComboBoxParameterAttachment(RangedAudioParameter&, ComboBox&, UndoManager*) { jassertfalse; }
};

class ChordSynthHeadlessButtonParameterAttachment {
public:
    ChordSynthHeadlessButtonParameterAttachment(RangedAudioParameter&, Button&, UndoManager*) { jassertfalse; }
};
} // namespace juce

// Keep the state-management implementation sourced directly from the vendored
// JUCE version rather than maintaining a divergent copy.
#define AudioProcessorValueTreeState ChordSynthHeadlessAudioProcessorValueTreeState
#define SliderParameterAttachment ChordSynthHeadlessSliderParameterAttachment
#define ComboBoxParameterAttachment ChordSynthHeadlessComboBoxParameterAttachment
#define ButtonParameterAttachment ChordSynthHeadlessButtonParameterAttachment
#include <juce_audio_processors/utilities/juce_AudioProcessorValueTreeState.cpp>
#undef ButtonParameterAttachment
#undef ComboBoxParameterAttachment
#undef SliderParameterAttachment
#undef AudioProcessorValueTreeState
