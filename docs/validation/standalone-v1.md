# Standalone V1 Validation Report

**Date:** 2026-08-21  
**Target:** Standalone Instrument Engine V1 (`ChordSynth`)  
**Host Environment:** Linux x86_64 (Kernel 6.8.0-111-generic) / Headless Build Pipeline + Offline Device Rendering

---

## 1. Environment & Architecture Overview

- **Engine Core:** `ChordSynthAudioProcessor` (C++20, JUCE 9.0.1).
- **Audio Output:** Stereo Bus (Channels 0 & 1), 32-bit float buffer format.
- **Synthesiser Structure:** 16-voice polyphonic allocator with `ChordVoice` dual detuned oscillator pairs (Saw, Square, Triangle, Sine), ADSR envelope, and global post-synth TPT Low-Pass Filter with parameter smoothing.
- **Control Paths:** Host MIDI note/CC input + wait-free, bounded `UiMidiQueue` (256-event capacity).

---

## 2. Checklist Execution & Findings

| # | Verification Item | Status | Details / Measurement |
|---|---|---|---|
| 1 | **Driver / Sample Rate Compatibility** | PASS | Validated across standard audio rates: `44.1 kHz`, `48.0 kHz`, and `96.0 kHz`. Filter coefficients and oscillator phase increments adapt without artifacts. |
| 2 | **Buffer Size Flexibility** | PASS | Verified under block sizes `1`, `16`, `64`, `128`, `256`, `512`, and `1024` samples without buffer underruns, NaN/Inf or signal discontinuities. |
| 3 | **Diatonic Pad & Key Mappings** | PASS | Validated with `MajorScaleChordMap` in C Major (C, Dm, Em, F, G, Am, Bdim) and transposed tonalities across 12 chromatic root keys. |
| 4 | **Polyphonic Voice Overlap** | PASS | Triggered up to 16 simultaneous voices across multiple overlapping chords without voice allocation exhaustion, clipping or stuck notes. |
| 5 | **MIDI Input & Queue Concurrency** | PASS | Processed high-rate MIDI event streams through both direct host MIDI buffers and the multi-threaded `UiMidiQueue`. |
| 6 | **Dynamic Parameter Automation** | PASS | Modulated `cutoff`, `resonance`, `detune`, and `waveform` choice during active polyphonic audio rendering. Smooth 20ms parameter ramps eliminate audible zipper noise. |
| 7 | **Lifecycle & Preset State Persistence** | PASS | Verified APVTS state serialization and `PresetSerializer` JSON schema v1 export/import. State restored cleanly mid-playback without voice drops or memory corruption. |
| 8 | **All-Notes-Off & Anti-Click Decay** | PASS | Clean release envelope decay upon note-off / `allNotesOff` CC, returning buffer magnitude to `< 1e-4` within release window. |
| 9 | **60-Second Soak Stability** | PASS | Completed deterministic 60-second offline soak render (11,250 blocks @ 48kHz). Peak output bounded <= 8.0 across 16 voices; bit-exact determinism across identical seeds. |

---

## 3. Conclusion

The Standalone / Core engine meets all V1 functional and realtime safety requirements. GUI-specific native device windowing remains decoupled and ready for platform-specific builds (Windows MSVC / macOS / Linux GUI).
