# Realtime Safety Guidelines and Verification

## Core Realtime Safety Invariants

In audio plugin development (VST3, Standalone, AU, CLAP), the audio rendering thread must execute deterministically within a strict time budget. Failure to do so leads to audio dropouts, glitches, buffer underruns, or host crashes.

### 1. Bounded Audio Callback (`processBlock`, `renderNextBlock`)
- **No Dynamic Memory Allocation:** No calls to `malloc`, `free`, `new`, `delete`, or operations that resize containers (`std::vector::push_back`, `std::string` concatenation, `juce::Array::add`, etc.). All voice collections, buffers, FIFOs, and DSP state must be preallocated during `prepareToPlay` or object construction.
- **No Blocking / Mutex Locks:** Never acquire standard mutexes (`std::mutex`, `juce::CriticalSection`) or wait on condition variables in the audio path. Shared data between UI and Audio threads must use wait-free, lock-free primitives (e.g., `std::atomic<float>` with `std::memory_order_relaxed`, lock-free FIFOs like `juce::AbstractFifo`).
- **No I/O Operations:** No file reads/writes, logging, console printing, or network socket calls within the audio render path.
- **Denormal Protection:** Always instantiate `juce::ScopedNoDenormals` at the beginning of `processBlock` to disable denormal floating point operations on x86/ARM hardware.
- **Finite Output Guarantees:** All rendered output samples must be checked and guarded against `NaN`, `+Inf`, and `-Inf`. Parameter updates must sanitize non-finite or out-of-range floats before applying them to DSP filters or oscillators.
- **Unused Channel Clearing:** Clear any excess or unconnected output channels (`buffer.clear(i, 0, buffer.getNumSamples())`).

### 2. Parameter Integration and State Migration
- Host automation parameters are managed via `AudioProcessorValueTreeState` (APVTS).
- Realtime threads access parameter values via atomic pointers obtained once during initialization (`getRawParameterValue`).
- Smooth transitions for continuous parameters (e.g., filter cutoff, resonance, master gain) use `juce::SmoothedValue` or deterministic linear ramps to avoid clicks/zipper noise.
- Complex state serialization/deserialization (JSON, XML) must happen strictly on the message/host thread (`getStateInformation`, `setStateInformation`), never inside `processBlock`.

---

## Verification and Soak Testing Architecture

ChordSynth includes an automated suite in `tests/dsp/RenderSafetyTests.cpp` validating:

1. **Matrix of Supported Sample Rates and Block Sizes:**
   - Sample rates: `44.1 kHz`, `48.0 kHz`, `96.0 kHz`.
   - Block sizes: `1`, `16`, `64`, `256`, `512`, `1024` samples.
   - Asserts finite output and bounded amplitudes across all combinations during silence, note-on, sustain, and note-off.

2. **Deterministic Soak Test (60 Seconds Offline Render):**
   - Renders 11,250 consecutive blocks (at 48 kHz / 256 samples per block).
   - Simulates randomized polyphonic Note-On, Note-Off, and runtime parameter automation driven by a deterministic fixed seed (`0xC001D00Du`).
   - Asserts bit-exact sample determinism across independent processor instances with identical seeds.
   - Asserts finite output (`isfinite`) and bounded polyphonic peak amplitude (`maxPeak <= 8.0f`).

3. **Extreme and Malformed Parameter Injection:**
   - Injects boundary, negative, excessive, `+Inf`, `-Inf`, and `NaN` values directly into raw atomic parameter pointers during active polyphonic rendering.
   - Verifies defensive clamping and fallback sanitization prevent filter blowup or oscillator hangs.

4. **All-Notes-Off and Envelope Decay:**
   - Verifies clean ADSR release decay after MIDI all-notes-off CC / message without hung voices or DC offsets.

5. **Host Lifecycle Stress:**
   - Repeated processor creation, preparation, rendering, and teardown across 50 consecutive cycles.
   - State capture and restoration mid-playback without audio corruption or memory leaks.
