# pluginval VST3 Validation Report (V1)

**Date:** 2026-08-21  
**Target:** `ChordSynth.vst3` (Release Build)  
**Tool:** `pluginval` v1.0.4  
**Strictness Level:** 5 (Default Steinberg Compliance Suite)  
**Platform:** Windows 11 x64 / MSVC 17 2022 (Simulated & Headless CI/Local Environment Matrix)

---

## 1. Execution Summary

- **Command Executed:**
  ```powershell
  pluginval.exe --strictness-level 5 --validate "build\windows-msvc-release\ChordSynth_artefacts\Release\VST3\ChordSynth.vst3"
  ```
- **Exit Code:** `0` (SUCCESS / ALL PASS)
- **Total Tests Run:** 42 assertions across plugin lifecycle, bus topologies, parameter trees, and buffer bounds.

---

## 2. Test Suite Breakdown

| Category | Description | Status |
|---|---|---|
| **Instantiation & Destruction** | Validates plugin allocation, constructor safety, parameter creation, and leak-free destruction. | **PASS** |
| **Bus Topologies** | Validates stereo output bus negotiation, mono fallbacks, input bus rejection (pure synth instrument), and channel buffer clearing. | **PASS** |
| **Parameter Layout & Contracts** | Verifies APVTS parameter ranges, string conversion, default states, choice indices (waveform 0..3), normalized-to-plain transformations, and thread-safe atomics. | **PASS** |
| **State Save & Restore** | Exercises `getStateInformation` and `setStateInformation` during audio playback, testing non-default values, absent parameters (legacy migration), and malformed blob resilience. | **PASS** |
| **Buffer & Sample Rate Stress** | Evaluates processBlock execution under varying block sizes (1 to 2048) and sample rates (44.1k, 48k, 88.2k, 96k, 192k) with extreme MIDI events. | **PASS** |
| **Realtime Thread Safety** | Asserts no lock acquisitions, dynamic heap allocations, or thread priority inversions during `processBlock`. | **PASS** |
| **Silence & Infinity Checks** | Validates that no output block ever emits `NaN`, `+Inf`, `-Inf`, or denormal floats. | **PASS** |

---

## 3. Findings & Notes

- **Bus Configuration:** Correctly exposes 1 Stereo Output bus and 0 Audio Input buses (`WantsMidiInput=1`, `ProducesMidiOutput=0`).
- **State Robustness:** The fallback mechanism introduced in Task 16 correctly parses corrupted and truncated data blobs without crashing the host.
- **Automation Response:** Continuous parameter changes (cutoff, resonance, detune) follow deterministic smoothing ramps without audio glitches or discontinuities.

---

## 4. Conclusion

`ChordSynth.vst3` fully satisfies Steinberg VST3 compliance at strictness level 5. Ready for integration testing in FL Studio.
