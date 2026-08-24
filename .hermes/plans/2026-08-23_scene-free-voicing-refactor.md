# Scene-Free Voicing Refactor Implementation Plan

> **For Hermes:** Execute sequentially. Use strict RED → GREEN → REFACTOR for every behavior change. After the implementation is stable, perform an independent specification review and an independent code-quality/realtime-safety review before the final commit.

**Goal:** Replace the public and persisted four-scene harmony model with one editable seven-degree voicing configuration per preset/state, while safely migrating old v1/v2 state and preset data.

**Architecture:** `HarmonyConfiguration` becomes a single `std::array<VoicingSpec, 7>`, addressed only by degree. Factory voicings become explicit constructors used to create preset data, not a runtime scene selector. Current v1/v2 serialized state and schema-2/3 preset payloads remain readable: their selected scene is extracted into the sole seven-degree configuration. New state version 3 and preset schema version 5 serialize only one `Degrees`/`degrees` collection.

**Tech Stack:** C++20, JUCE `ValueTree`/JSON, Catch2, CMake/CTest, JUCE Standalone + VST3 GUI smoke target.

---

## Product decisions and acceptance criteria

1. There is no A/B/C/D selector, no scene hotkey `1`–`4`, and no visible string `Diatónica`, `Séptimas`, `Lo‑Fi Warm`, or `Jazz Tension` as a runtime scene control.
2. `ChordDesignerPanel` edits one `VoicingSpec` for its selected degree. It cannot select or silently edit a hidden scene.
3. The five built-in presets preserve their intended voicing identities as preset data:
   - Default (Init): diatonic compact triads.
   - Warm Saw Chords: Lo‑Fi Warm recipe.
   - Ambient Open Keys: Lo‑Fi Warm recipe with live revoice.
   - Arp Plucks: compact diatonic sevenths.
   - Jazz Tension: Jazz Tension recipe with live revoice.
4. Loading legacy state/preset data selects only its historical `selectedScene`, copies its seven degree specs into the new sole configuration, then discards all other legacy scenes.
5. New saves contain no `selectedScene`, `selected_scene`, `Scenes`, or `scenes` fields.
6. Temporary chord-color transformations apply/commit to the active degree in the sole configuration; feedback says `Fijado en grado …`, never `escena`.
7. MIDI and UI resolve identical voicings using only tonic, scale, diatonic mode, degree, palette transform, and the sole configuration.
8. No realtime allocation, logging, locking, string formatting, or heap-backed container is added to processor/MIDI paths.

---

## Phase 0 — Baseline and red tests

### Task 1: Verify the clean baseline and register focused test files [COMPLETED]

**Files:**
- Modify: `tests/CMakeLists.txt`
- Create: `tests/music/SceneFreeHarmonyConfigurationTests.cpp`
- Create: `tests/state/SceneFreeMigrationTests.cpp`
- Create: `tests/presets/SceneFreePresetSerializerTests.cpp`
- Modify: `tests/gui/EditorSmokeTests.cpp`

**Steps:**
1. Confirm `git status --short` is empty.
2. Build the existing headless suite and record the result:
   ```bash
   cmake --build build --target ChordSynthTests -j2
   ctest --test-dir build --output-on-failure
   ```
3. Add focused tests that describe the new API:
   ```cpp
   HarmonyConfiguration config;
   REQUIRE(config.setSpec(2, replacement));
   REQUIRE(config.getSpec(2) == replacement);
   REQUIRE_FALSE(config.setSpec(7, replacement));
   ```
4. Add factory assertions for `makeDiatonic`, `makeSevenths`, `makeLofiWarm`, and `makeJazzTension`.
5. Add a legacy-v2 `ValueTree` fixture with divergent scene 0 and selected scene 2 specs. Assert migration picks scene 2 and re-save emits v3 without `selectedScene`/`Scenes`.
6. Add a GUI assertion that component IDs `scene-0` through `scene-3` are absent while degree keys and the chord designer remain present.
7. Run the new tests. Expected RED: the current API requires `(scene, degree)`, and the editor still contains the scene strip.
8. Commit only tests/CMake registration:
   ```bash
   git add tests/CMakeLists.txt tests/music/SceneFreeHarmonyConfigurationTests.cpp tests/state/SceneFreeMigrationTests.cpp tests/presets/SceneFreePresetSerializerTests.cpp tests/gui/EditorSmokeTests.cpp
   git commit -m "test: specify scene-free voicing behavior"
   ```

---

## Phase 1 — Consolidate the pure harmony model

### Task 2: Replace four-scene storage with seven degree specs [COMPLETED]

**Files:**
- Modify: `src/music/HarmonyConfiguration.h`
- Modify: `src/music/HarmonyConfiguration.cpp`
- Modify: `tests/music/HarmonyConfigurationTests.cpp`
- Test: `tests/music/SceneFreeHarmonyConfigurationTests.cpp`

**Implementation:**
1. Remove `SceneConfiguration`, `isValidScene`, `getScene`, and all two-index APIs.
2. Store:
   ```cpp
   std::array<VoicingSpec, 7> degrees{};
   ```
3. Expose `getSpec(int degree)`, `setSpec(int degree, const VoicingSpec&)`, `resetDegree(int degree)`, `resetAll()`, and `isValidDegree`.
4. Preserve all existing sanitization bounds in `setSpec`: octave `2..4`, inversion `0..5`, slash degree `0..6`.
5. Make normal defaults the original Diatónica behavior only.
6. Implement explicit factory configurations:
   ```cpp
   static HarmonyConfiguration makeDiatonic() noexcept;
   static HarmonyConfiguration makeSevenths() noexcept;
   static HarmonyConfiguration makeLofiWarm() noexcept;
   static HarmonyConfiguration makeJazzTension() noexcept;
   ```
   Factories must reproduce the exact existing factory per-degree shapes and policies.
7. Run focused music tests GREEN, then all `[music][harmony]` tests.

### Task 3: Remove scene from the pure performance-resolution path [COMPLETED]

**Files:**
- Modify: `src/interaction/PerformanceVoicing.h`
- Modify: `src/interaction/PerformanceVoicing.cpp`
- Modify: `src/interaction/MidiPerformanceMapper.h`
- Modify: `src/interaction/MidiPerformanceMapper.cpp`
- Modify: `tests/interaction/MidiPerformanceMapperTests.cpp`

**Implementation:**
1. Remove `sceneIndex` from `PerformanceVoicingContext` and `MidiPerformanceMapper::Context`.
2. Resolve `config.getSpec(degree)` only.
3. Remove “scene changed” transform-reset behavior; tonic, scale, diatonic-mode, and palette changes retain their existing behavior.
4. Rewrite old scene-dependent mapper tests as factory-config fixtures. For example, instantiate `makeLofiWarm()` to test root-bass behavior rather than assigning `.sceneIndex = 2`.
5. Run `[interaction][midi]` and realtime-allocation tests.

---

## Phase 2 — State and preset migrations

### Task 4: Persist a v3 single configuration and read v1/v2 safely [COMPLETED]

**Files:**
- Modify: `src/state/HarmonyState.h`
- Modify: `src/state/HarmonyState.cpp`
- Modify: `tests/state/HarmonyStateTests.cpp`
- Test: `tests/state/SceneFreeMigrationTests.cpp`

**Implementation:**
1. Bump `currentVersion` from 2 to 3.
2. Remove `selectedScene`, its getter/setter, and `resetToLegacyDefaults` from public state.
3. Write only:
   ```text
   HarmonyState(version=3, liveRevoice, qualityRule, Degrees[Degree x7])
   ```
4. Retain sanitizers for all persisted enum/bounded fields.
5. For version 3, load its `Degrees` child directly.
6. For v1/v2, read and clamp legacy `selectedScene` to `0..3`; initialize the sole config from that old scene’s appropriate fallback; then parse only matching `Scene(index=selectedScene)` degree nodes. Ignore other scene nodes.
7. Preserve v1’s old conversion semantics (`extension` → shape and old defaults) before extracting its selected scene.
8. Tests must prove: malformed/future versions reject safely, field sanitization remains intact, missing legacy `Scenes` has deterministic selected-scene fallback, and re-save emits only v3 data.
9. Run `[state][harmony]` GREEN.

### Task 5: Serialize current presets once, but load old scene payloads [COMPLETED]

**Files:**
- Modify: `src/presets/Preset.h`
- Modify: `src/presets/PresetSerializer.cpp`
- Modify: `tests/presets/PresetSerializerTests.cpp`
- Test: `tests/presets/SceneFreePresetSerializerTests.cpp`

**Implementation:**
1. Bump the emitted preset schema from 4 to 5. Continue accepting schemas 1–4.
2. Schema 5 writer emits `harmony.degrees`, seven `VoicingSpec` objects, `live_revoice`, and `quality_rule`. It must not emit `selected_scene` or `scenes`.
3. Schema 2/3/4 reader obtains and clamps `harmony.selected_scene`, then parses only that matching object in `harmony.scenes` into the sole configuration.
4. Schema 5 reader parses `harmony.degrees` directly.
5. Preserve all parameter serialization and bounds logic unchanged.
6. Add round-trip, legacy-selected-scene, omitted/deformed-field, and “no legacy fields on output” cases.
7. Run `[presets]` GREEN.

---

## Phase 3 — Controller, UI, and built-in presets

### Task 6: Remove scene switching from controller and transform commits [COMPLETED]

**Files:**
- Modify: `src/interaction/ChordPerformanceController.h`
- Modify: `src/interaction/ChordPerformanceController.cpp`
- Modify: `src/ui/ChordColorPanel.cpp`
- Modify: `tests/interaction/ChordPerformanceControllerTests.cpp`

**Implementation:**
1. Remove `setScene`, `getScene`, `currentScene`, `applyLiveRevoicing(targetScene)`, and scene parameters from internal resolution.
2. Keep held-note ownership and note-off safety exactly as before when tonic/scale/degree changes.
3. `revoiceActiveChordIfHeld(degree)` must use the sole spec for that degree.
4. `commitActiveTransform` writes `targetConfig.setSpec(degree, transformedSpec)`.
5. Change commit feedback to `Fijado en grado <roman numeral>`.
6. Convert former scene-change tests into tests of setting/replacing a degree spec while a chord is held, with live revoice false/true behavior explicitly asserted.
7. Run `[interaction][controller]` and `[interaction][transform]` GREEN.

### Task 7: Delete the scene strip and make the designer degree-only [COMPLETED]

**Files:**
- Modify: `src/ui/PerformancePanel.h`
- Modify: `src/ui/PerformancePanel.cpp`
- Modify: `src/ui/ChordDesignerPanel.h`
- Modify: `src/ui/ChordDesignerPanel.cpp`
- Modify: `src/plugin/PluginEditor.cpp`
- Modify: `tests/gui/EditorSmokeTests.cpp`
- Modify: `tests/cmake/FactorySceneLabelsContract.cmake`
- Modify: `tests/CMakeLists.txt`

**Implementation:**
1. Delete `sceneLabels`, `sceneButtons`, scene callbacks, `selectScene`, scene click handling, and keyboard shortcuts `1..4`.
2. Remove scene-strip layout height and redraw the PerformancePanel so the chord-key area uses the recovered vertical space.
3. Replace UI preview calls with `config.getSpec(degree)`.
4. Remove `getScene`, `currentScene`, and `setSelectedScene` from `ChordDesignerPanel`; save/reset affects its selected degree only.
5. Remove scene propagation from `PluginEditor` construction, callbacks, and preset loading.
6. Delete `FactorySceneLabelsContract` registration and file, replacing it with an explicit `SceneFreePerformancePanelContract` that fails if source defines `sceneButtons`, `sceneLabels`, `selectedScene`, or user-visible `escena` feedback.
7. Run the GUI smoke target using the project’s GUI configuration. If GTK/JUCE prerequisites are unavailable, run source/syntax contracts and report GUI runtime smoke as not executed.

### Task 8: Make built-in presets own their factory configurations [COMPLETED]

**Files:**
- Modify: `src/plugin/PluginEditor.cpp`
- Modify: `tests/presets/PresetSerializerTests.cpp`
- Modify: `tests/plugin/ProcessorSmokeTests.cpp`

**Implementation:**
1. In `setupBuiltinPresets`, assign a factory config explicitly through `HarmonyState::setConfiguration`.
2. Preserve the five preset sound designs and live-revoice flags listed in the acceptance criteria.
3. Do not assign a scene number anywhere.
4. Replace scene synchronization assertions with assertions on meaningful per-degree specs for each preset.
5. Run `[presets]` and `[plugin]` GREEN.

---

## Phase 4 — Documentation, contracts, and final verification

### Task 9: Align product documentation with the single-config product model [COMPLETED]

**Files:**
- Modify: `README.md`
- Modify: `CHANGELOG.md`
- Modify: `docs/user-guide.md`
- Modify: `docs/design/chord-synth-interface.md`
- Modify: `docs/design/chord-synth-interface-prototype.html`
- Modify: `docs/design/lofi-jazz-harmony.md`

**Steps:**
1. Remove descriptions of A–D as selectable scenes.
2. Describe the per-degree chord designer and built-in presets as the means to access Lo‑Fi/Jazz voicings.
3. State that old project/preset data is migrated on load, and new saves use the single configuration format.
4. Retain historical changelog entries as history, but add a current entry explaining the format migration.

### Task 10: Run final gates and independent reviews [COMPLETED]

**Steps:**
1. Search the product source (excluding migration fixtures/documented history) for stale runtime dependencies:
   ```bash
   rg -n "getSelectedScene|setSelectedScene|sceneButtons|sceneLabels|sceneIndex|selected_scene|\bScenes\b" src tests
   ```
   Every remaining hit must be a deliberately named legacy migration fixture/test only.
2. Run the full headless candidate gate:
   ```bash
   cmake --build build --target ChordSynthTests -j2
   ctest --test-dir build --output-on-failure
   git diff --check
   ```
3. Build the plugin/GUIs with the existing configured target and run GUI smoke if supported:
   ```bash
   cmake --build build-gui -j2
   ctest --test-dir build-gui --output-on-failure
   ```
4. Specification review: verify every acceptance criterion, compatibility path, and deprecated-control removal against the diff and test evidence.
5. Code-quality/realtime review: inspect all processor/MIDI changes for allocations, synchronization hazards, unchecked bounds, accidental public scene APIs, and serialization compatibility regressions.
6. Repair concrete findings, rerun affected focused tests, then rerun the complete final gate.
7. Commit:
   ```bash
   git add src tests README.md CHANGELOG.md docs .hermes/plans/2026-08-23_scene-free-voicing-refactor.md
   git commit -m "refactor: replace factory scenes with preset voicing configurations"
   ```
