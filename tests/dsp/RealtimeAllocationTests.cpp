#include <catch2/catch_test_macros.hpp>
#include "parameters/ParameterIds.h"
#include "plugin/PluginProcessor.h"
#include <atomic>
#include <cstdlib>
#include <execinfo.h>
#include <new>

namespace {
std::atomic<bool> countRealtimeAllocations{false};
std::atomic<std::size_t> realtimeAllocationCount{0};

void recordAllocation() noexcept
{
    if (countRealtimeAllocations.load(std::memory_order_relaxed)) {
        const auto previous = realtimeAllocationCount.fetch_add(1, std::memory_order_relaxed);
        if (previous == 0) {
            countRealtimeAllocations.store(false, std::memory_order_relaxed);
            void* frames[32]{};
            const int frameCount = backtrace(frames, 32);
            backtrace_symbols_fd(frames, frameCount, 2);
            countRealtimeAllocations.store(true, std::memory_order_relaxed);
        }
    }
}

void* allocate(std::size_t size)
{
    recordAllocation();
    if (void* memory = std::malloc(size == 0 ? 1 : size)) {
        return memory;
    }
    throw std::bad_alloc{};
}

void* allocateAligned(std::size_t size, std::size_t alignment)
{
    recordAllocation();
    void* memory = nullptr;
    if (posix_memalign(&memory, alignment, size == 0 ? 1 : size) == 0) {
        return memory;
    }
    throw std::bad_alloc{};
}

void beginAllocationCount() noexcept
{
    realtimeAllocationCount.store(0, std::memory_order_relaxed);
    countRealtimeAllocations.store(true, std::memory_order_release);
}

std::size_t endAllocationCount() noexcept
{
    countRealtimeAllocations.store(false, std::memory_order_release);
    return realtimeAllocationCount.load(std::memory_order_relaxed);
}
} // namespace

extern "C" {
void* __real_malloc(std::size_t size);
void* __real_calloc(std::size_t count, std::size_t size);
void* __real_realloc(void* memory, std::size_t size);
void __real_free(void* memory);
int __real_posix_memalign(void** memory, std::size_t alignment, std::size_t size);

void* __wrap_malloc(std::size_t size)
{
    recordAllocation();
    return __real_malloc(size);
}

void* __wrap_calloc(std::size_t count, std::size_t size)
{
    recordAllocation();
    return __real_calloc(count, size);
}

void* __wrap_realloc(void* memory, std::size_t size)
{
    recordAllocation();
    return __real_realloc(memory, size);
}

void __wrap_free(void* memory) { __real_free(memory); }

int __wrap_posix_memalign(void** memory, std::size_t alignment, std::size_t size)
{
    recordAllocation();
    return __real_posix_memalign(memory, alignment, size);
}
}

void* operator new(std::size_t size) { return allocate(size); }
void* operator new[](std::size_t size) { return allocate(size); }
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }
void* operator new(std::size_t size, std::align_val_t alignment)
{
    return allocateAligned(size, static_cast<std::size_t>(alignment));
}
void* operator new[](std::size_t size, std::align_val_t alignment)
{
    return allocateAligned(size, static_cast<std::size_t>(alignment));
}
void operator delete(void* memory, std::align_val_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::align_val_t) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t, std::align_val_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t, std::align_val_t) noexcept { std::free(memory); }

TEST_CASE("MIDI Performance processBlock performs no dynamic allocations after prepare", "[dsp][realtime][allocation]")
{
    constexpr int blockSize = 256;
    chordsynth::ChordSynthAudioProcessor processor;
    processor.prepareToPlay(48000.0, blockSize);

    auto* performanceMidi = processor.getAPVTS().getRawParameterValue(
        chordsynth::parameters::ids::performanceMidiEnabled);
    REQUIRE(performanceMidi != nullptr);
    performanceMidi->store(1.0f, std::memory_order_relaxed);

    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer midi;
    midi.ensureSize(64 * 1024);

    // Exercise every mapped degree and transform once before measuring so all
    // lazy one-time setup is excluded from the realtime contract.
    for (int degree = 0; degree < 7; ++degree) {
        midi.clear();
        midi.addEvent(juce::MidiMessage::noteOn(1, 36 + degree, 0.8f), 0);
        midi.addEvent(juce::MidiMessage::controllerEvent(1, 20 + degree, 127), 32);
        midi.addEvent(juce::MidiMessage::controllerEvent(1, 20 + degree, 0), 96);
        midi.addEvent(juce::MidiMessage::noteOff(1, 36 + degree), 192);
        buffer.clear();
        processor.processBlock(buffer, midi);
    }

    std::size_t allocations = 0;
    for (int iteration = 0; iteration < 64; ++iteration) {
        const int degree = iteration % 7;
        const int slot = iteration % 8;
        midi.clear();
        midi.addEvent(juce::MidiMessage::noteOn(1, 36 + degree, 0.75f), 0);
        midi.addEvent(juce::MidiMessage::controllerEvent(1, 20 + slot, 127), 48);
        midi.addEvent(juce::MidiMessage::controllerEvent(1, 20 + slot, 0), 128);
        midi.addEvent(juce::MidiMessage::noteOff(1, 36 + degree), 224);
        buffer.clear();

        beginAllocationCount();
        processor.processBlock(buffer, midi);
        allocations += endAllocationCount();
    }

    REQUIRE(allocations == 0);
}

TEST_CASE("processBlock bounds oversized MIDI input before realtime scratch buffers", "[dsp][realtime][allocation]")
{
    constexpr int blockSize = 256;
    constexpr int oversizedEventCount = 16384;
    chordsynth::ChordSynthAudioProcessor processor;
    processor.prepareToPlay(48000.0, blockSize);

    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer midi;
    midi.ensureSize(256 * 1024);
    for (int index = 0; index < oversizedEventCount; ++index) {
        midi.addEvent(juce::MidiMessage::controllerEvent(1, 1, index % 128), 0);
    }

    beginAllocationCount();
    processor.processBlock(buffer, midi);
    const auto allocations = endAllocationCount();

    REQUIRE(allocations == 0);
}

TEST_CASE("processBlock does not inspect host MIDI beyond the realtime scan budget", "[dsp][realtime][midi-bound]")
{
    constexpr int blockSize = 256;
    constexpr int rejectedEventCount = 256;
    chordsynth::ChordSynthAudioProcessor processor;
    processor.prepareToPlay(48000.0, blockSize);

    const std::uint8_t sysExData[]{0xf0, 0x01, 0x02, 0xf7};
    const auto sysEx = juce::MidiMessage::createSysExMessage(sysExData, static_cast<int>(std::size(sysExData)));
    juce::MidiBuffer midi;
    midi.ensureSize(16 * 1024);
    for (int index = 0; index < rejectedEventCount; ++index) {
        midi.addEvent(sysEx, 0);
    }
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);

    juce::AudioBuffer<float> buffer(2, blockSize);
    processor.processBlock(buffer, midi);

    float absoluteSum = 0.0f;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            absoluteSum += std::abs(buffer.getSample(channel, sample));
        }
    }
    REQUIRE(absoluteSum == 0.0f);
}

TEST_CASE("processBlock does not copy rejected UI SysEx on the audio thread", "[dsp][realtime][allocation]")
{
    constexpr int blockSize = 256;
    chordsynth::ChordSynthAudioProcessor processor;
    processor.prepareToPlay(48000.0, blockSize);

    const std::uint8_t sysExData[]{0xf0, 0x01, 0x02, 0xf7};
    const auto sysEx = juce::MidiMessage::createSysExMessage(sysExData, static_cast<int>(std::size(sysExData)));
    REQUIRE_FALSE(processor.getUiMidiQueue().push(sysEx));

    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer midi;
    beginAllocationCount();
    processor.processBlock(buffer, midi);
    const auto allocations = endAllocationCount();

    REQUIRE(allocations == 0);
}
