# Lo‑Fi/Jazz Harmony and Performance Implementation Plan
<!-- completed -->

> **For Hermes:** Use `subagent-driven-development` to implement this plan task-by-task. Execute sequentially. After every implementation task, run a specification-compliance review and then an independent code-quality/realtime-safety review before committing.

**Goal:** Expand ChordSynth from triads/sevenths into a reproducible Lo‑Fi/Jazz harmony system with six-tone recipes, musical voicing policies, temporary performance transformations, keyboard/MIDI control, and four useful factory scenes.

**Architecture:** Keep harmony declarative and persistent: a `ChordShape` selects a supported musical recipe, a resolver derives the actual quality/seventh/tensions from tonic, scale and degree, and a voicing engine selects/registers at most six chord tones plus an optional independent bass note. Add a pure transformation layer over `VoicingSpec`; both the UI controller and processor-side MIDI mapper reuse it. Temporary transformations never mutate saved state; “Fijar” explicitly writes the transformed spec into the selected scene/degree.

**Tech Stack:** C++20, JUCE, Catch2, CMake/Ninja, APVTS/ValueTree state, Standalone + VST3.

---

## Product decisions fixed by this plan

1. **Six chord tones, bass separate:** `NoteSet` stores at most six harmonic voices. `VoicedChord` additionally carries `std::optional<int> bassMidi`; controller bookkeeping therefore handles at most seven sounding notes and fourteen events during a complete replacement.
2. **Bounded recipes, not arbitrary interval soup:** v1 supports triad, seventh, ninth, eleventh, thirteenth, add9, 6/9, sus2 and sus4. Altered tensions (`b9`, `#9`, `#11`, `b13`, `alt`) are deliberately deferred.
3. **Actual chord identity is resolved:** free-mode quality adds `dominant`; diatonic mode derives major/minor/diminished and the correct seventh from scale degree. Labels must distinguish `Cadd9`, `Cmaj9`, `C9`, `Cm9`, `C6/9`, etc.
4. **Voicing and recipe stay separate:** `ChordShape` says what the chord is; `VoicingStyle` (`compact`, `open`, `rootless`) plus `FifthPolicy`, register and bass settings say what is played.
5. **Nearest voice leading is stateful but deterministic:** a pure `VoiceLeadingResolver` accepts the previous `NoteSet` and candidate recipe; it does not hide history in `DiatonicChordVoicer`.
6. **Temporary palette semantics:** eight slots per palette. Press/hold applies a transformation to the currently held degree; release restores the saved spec using differential note-off/note-on changes. “Fijar” persists the transformed spec.
7. **Input mapping:** computer keys `A S D F G H J K` select/hold the eight color slots while `Q…U` continue to select degrees. MIDI mapping is opt-in: notes 36–42 trigger degrees I–VII and CC 20–27 hold/release palette slots (`>=64` pressed, `<64` released). Other MIDI notes/events pass through unchanged.
8. **Bass and arpeggiator:** UI/MIDI-generated bass uses internal MIDI channel 2 and bypasses the arpeggiator; chord tones use channel 1 and follow the current arpeggiator. The synth still renders both channels. This avoids arpeggiating the dedicated bass voice.
9. **Factory scenes replace the current semantic defaults:** A Diatónica, B Séptimas, C Lo‑Fi Warm, D Jazz Tension. Legacy v1 harmony state must retain its old scene contents after migration rather than silently becoming the new defaults.
10. **No realtime allocation:** all recipe, voicing, transform, MIDI mapping and diff operations use fixed-capacity arrays and value types. No `std::vector`, heap allocation, locks, logging or string construction in `processBlock`.

## Acceptance examples

In C major, root register 3:

| Request | Identity/label | Required pitch classes before voicing omissions |
|---|---|---|
| I ninth | `Cmaj9` | C E G B D |
| ii ninth | `Dm9` | D F A C E |
| V ninth | `G9` | G B D F A |
| V thirteenth | `G13` | G B D F A E |
| vi eleventh | `Am11` | A C E G B D |
| I add9 | `Cadd9` | C E G D |
| I 6/9 | `C6/9` | C E G A D |
| I sus2 | `Csus2` | C D G |
| I sus4 | `Csus4` | C F G |

Regression examples that must remain unchanged: C major triads/sevenths, C natural-minor triads/sevenths, free `Dmaj7`, existing oscillator/DSP behavior, old preset loading and old note-off safety.

---

## Phase 0 — Baseline and contracts

### Task 1: Record the clean baseline and add test targets

**Objective:** Establish a reproducible starting point and register the new focused test files before production changes.

**Files:**
- Modify: `tests/CMakeLists.txt`
- Create: `tests/music/ChordRecipeTests.cpp`
- Create: `tests/music/VoiceLeadingResolverTests.cpp`
- Create: `tests/interaction/ChordTransformTests.cpp`
- Create: `tests/interaction/MidiPerformanceMapperTests.cpp`

**Steps:**
1. Confirm `git status --short` is empty; if not, record dirty files and do not overwrite them.
2. Run baseline:
   ```bash
   cmake --build build/linux-ninja-debug --parallel
   ctest --test-dir build/linux-ninja-debug --output-on-failure
   ```
   Expected: build and current suite pass.
3. Add empty Catch2 test translation units with one placeholder contract each and register them in `ChordSynthTests`.
4. Build and run the four named test files/tags to prove discovery works.
5. Commit:
   ```bash
   git add tests/CMakeLists.txt tests/music/ChordRecipeTests.cpp tests/music/VoiceLeadingResolverTests.cpp tests/interaction/ChordTransformTests.cpp tests/interaction/MidiPerformanceMapperTests.cpp
   git commit -m "test: add lo-fi harmony test targets"
   ```

---

## Phase 1 — Expand the musical core

### Task 2: Increase `NoteSet` to six tones and centralize capacities

**Objective:** Remove all hard-coded four-note assumptions before adding extensions.

**Files:**
- Modify: `src/music/VoicedChord.h`
- Modify: `src/interaction/ChordPerformanceController.cpp`
- Modify: `tests/music/VoicedChordTests.cpp`
- Modify: `tests/interaction/ChordPerformanceControllerTests.cpp`

**Contract:**
```cpp
inline constexpr std::size_t maxChordTones = 6;
using NoteStorage = std::array<int, maxChordTones>;

class NoteSet {
public:
    constexpr NoteSet(const NoteStorage& rawNotes, int noteCount) noexcept;
    static constexpr int capacity() noexcept { return static_cast<int>(maxChordTones); }
};
```

**TDD steps:**
1. RED: change `VoicedChordTests` to require count clamping at six, six-note iteration/equality and no visibility of inactive storage.
2. Run:
   ```bash
   ./build/linux-ninja-debug/tests/ChordSynthTests "[music][voicing]"
   ```
   Expected: compile/test failure against the four-note type.
3. GREEN: implement `NoteStorage`, `maxChordTones`, and six-tone `NoteSet`.
4. Replace controller capacities with named constants:
   ```cpp
   constexpr std::size_t maxSoundingNotes = maxChordTones + 1; // optional bass
   constexpr std::size_t maxReplacementEvents = maxSoundingNotes * 2;
   ```
   Do not add bass behavior yet; preserve current output.
5. Update existing aggregate constructions from 4 to 6 storage values or add a safe `initializer_list` constructor usable outside realtime rendering.
6. Run `[music][voicing]`, `[music][voicer]`, and `[interaction][controller]`.
7. Commit `refactor: expand chord note capacity to six`.

### Task 3: Introduce chord recipe and voicing contracts

**Objective:** Represent supported chord identities without encoding every identity directly in the voicer.

**Files:**
- Create: `src/music/ChordRecipe.h`
- Create: `src/music/ChordRecipe.cpp`
- Modify: `src/music/VoicedChord.h`
- Modify: `CMakeLists.txt`
- Test: `tests/music/ChordRecipeTests.cpp`

**Contracts:**
```cpp
enum class ChordShape : std::uint8_t {
    triad, seventh, ninth, eleventh, thirteenth, add9, sixNine, sus2, sus4
};

enum class ResolvedQuality : std::uint8_t {
    major, minor, diminished, dominant
};

enum class SeventhKind : std::uint8_t {
    none, major, minor, diminished, halfDiminished
};

enum class VoicingStyle : std::uint8_t { compact, open, rootless };
enum class FifthPolicy : std::uint8_t { automatic, include, omit };
enum class BassMode : std::uint8_t { none, root, slashDegree };
enum class VoiceLeadingMode : std::uint8_t { manual, nearest };

struct ChordRecipe {
    ResolvedQuality quality{ResolvedQuality::major};
    SeventhKind seventh{SeventhKind::none};
    bool includeSixth{false};
    bool includeNinth{false};
    bool includeEleventh{false};
    bool includeThirteenth{false};
    bool sus2{false};
    bool sus4{false};
};
```

Extend `VoicingSpec` with `shape`, `style`, `fifthPolicy`, `bassMode`, `slashDegree`, `voiceLeading`, `inversion`, `baseOctave`, and existing `qualityRule`. Add `QualityRule::dominant`. Preserve stable enum numeric values for existing values.

**TDD steps:**
1. RED: table-test recipe resolution for each `ChordShape`; assert sus2/sus4 are mutually exclusive and 6/9 contains 6+9 without a seventh.
2. GREEN: add pure recipe factories/sanitizers; no MIDI register logic yet.
3. Add `ChordRecipe.cpp` to `chordsynth_music` and the full JUCE target transitively through that library.
4. Run `[music][recipe]` and existing harmony tests.
5. Commit `feat: define extended chord recipe contracts`.

### Task 4: Resolve diatonic and free chord identities

**Objective:** Correctly derive maj7/m7/dom7 and 9/11/13 identities for major and natural minor.

**Files:**
- Modify: `src/music/ChordRecipe.h`
- Modify: `src/music/ChordRecipe.cpp`
- Modify: `src/music/DiatonicChordVoicer.cpp`
- Test: `tests/music/ChordRecipeTests.cpp`
- Test: `tests/music/DiatonicChordVoicerTests.cpp`

**TDD cases:**
- C major: `Cmaj9`, `Dm9`, `G9`, `G13`, `Am11`, `Bm7b5`.
- C natural minor: `Cm9`, `Dm7b5`, `D#maj9`, `Fm9`, `Gm9`, `G#maj9`, `A#9` using the project’s sharp naming convention.
- Free rules: major+ninth → maj9, minor+ninth → m9, dominant+ninth → 9, diminished+seventh → dim7.
- add9 never silently becomes maj9; 6/9 has no seventh.

**Steps:**
1. RED: write label and pitch-class expectations.
2. Move scale interval/quality/seventh tables from the anonymous voicer namespace into the recipe resolver where appropriate.
3. GREEN: expose a pure function:
   ```cpp
   ChordRecipe resolveChordRecipe(Scale, int degree, ChordShape, QualityRule) noexcept;
   ```
4. Keep existing triad and seventh note outputs byte-for-byte compatible.
5. Run `[music][recipe]`, `[music][voicer]`, and natural-minor regressions.
6. Commit `feat: resolve diatonic jazz chord identities`.

### Task 5: Generate 9th, 11th, 13th, add9, 6/9 and suspended tones

**Objective:** Render deterministic root-position tone sets and labels up to six notes.

**Files:**
- Modify: `src/music/DiatonicChordVoicer.cpp`
- Modify: `src/music/VoicedChord.h`
- Test: `tests/music/DiatonicChordVoicerTests.cpp`

**Implementation rule:** Build recipe tones by role in a fixed array (`root`, `third/suspension`, `fifth`, `seventh`, `sixth/13th`, `ninth`, `eleventh`) and then select at most six according to explicit omission priority. Do not append intervals ad hoc in multiple switch branches.

**Default omission priority when the theoretical recipe exceeds six:** omit perfect fifth first; never omit the third or seventh of a dominant chord; retain the named highest tension.

**TDD cases:** exact MIDI notes and labels for all acceptance examples, upper MIDI bound rejection, no duplicates (6 and 13 must not be emitted twice), finite 0..127 output.

**Verification:**
```bash
./build/linux-ninja-debug/tests/ChordSynthTests "[music][voicer][extensions]"
./build/linux-ninja-debug/tests/ChordSynthTests "[music][voicer]"
```

Commit `feat: render extended chord recipes`.

### Task 6: Implement compact, open and rootless policies with fifth omission

**Objective:** Separate chord identity from practical Lo‑Fi register/distribution.

**Files:**
- Create: `src/music/ChordVoicingEngine.h`
- Create: `src/music/ChordVoicingEngine.cpp`
- Modify: `src/music/DiatonicChordVoicer.h`
- Modify: `src/music/DiatonicChordVoicer.cpp`
- Modify: `CMakeLists.txt`
- Test: `tests/music/DiatonicChordVoicerTests.cpp`

**Policy definitions:**
- `compact`: ascending tones in the smallest legal span from `baseOctave`.
- `open`: deterministic drop-2/spread; preserve current open triad/seventh outputs as regressions.
- `rootless`: omit root for seventh-or-higher recipes; retain 3rd, 7th and named tension; triads fall back to compact.
- `FifthPolicy::automatic`: include for triads/sus; omit first for 9/11/13 when capacity/register requires it.
- `include` and `omit`: explicit overrides, sanitized when musically impossible.

**TDD:** RED tests for Cmaj9, Dm9, G13 and Am11 under each style and fifth policy; GREEN implementation using fixed arrays only.

Run `[music][voicer][style]` plus all voicer tests. Commit `feat: add lo-fi chord voicing policies`.

### Task 7: Add safe register constraints · [COMPLETED]

**Objective:** Prevent dense chord tones from being placed in muddy low registers while preserving explicit bass below them.

**Files:**
- Modify: `src/music/ChordVoicingEngine.h`
- Modify: `src/music/ChordVoicingEngine.cpp`
- Test: `tests/music/DiatonicChordVoicerTests.cpp`

**Rules for v1:**
- Harmonic chord floor: MIDI 48 (C3) for recipes with five/six tones.
- Rootless/open floor: MIDI 52 (E3) for non-bass tones.
- Harmonic ceiling: MIDI 96 (C7).
- Dedicated bass range: MIDI 24..47; transpose by octaves into range.
- Preserve pitch classes and ascending uniqueness.
- If a legal arrangement cannot be found, return a deterministic safe compact voicing rather than throwing from the realtime call path.

Add table-driven boundary tests across tonics, base octaves 2–4 and all shapes. Commit `feat: constrain extended voicings to safe registers`.

### Task 8: Implement nearest voice-leading resolver · [COMPLETED]

**Objective:** Choose octave placements minimizing movement from the previously sounding chord.

**Files:**
- Create: `src/music/VoiceLeadingResolver.h`
- Create: `src/music/VoiceLeadingResolver.cpp`
- Modify: `CMakeLists.txt`
- Test: `tests/music/VoiceLeadingResolverTests.cpp`

**Algorithm:** Generate a bounded set of octave-displaced candidates inside the safe register, sort each ascending, and minimize deterministic cost:

```text
cost = sum(abs(matchedVoiceDelta))
     + 12 * abs(newVoiceCount - oldVoiceCount)
     + 2 * crossingPenalty
```

Tie-break by lower total span, then lexicographic MIDI order. Bass is excluded from harmonic voice-leading and resolved separately.

**TDD:** IV→V→I and ii→V→I sequences; same inputs produce identical outputs; common tones stay where possible; zero previous notes falls back to requested manual style; no note crosses bounds.

Commit `feat: add deterministic nearest voice leading`.

### Task 9: Add separate root/slash bass to voiced output and controller · [COMPLETED]

**Objective:** Produce, display and safely release an optional bass note independently from the six harmonic tones.

**Files:**
- Modify: `src/music/VoicedChord.h`
- Modify: `src/music/DiatonicChordVoicer.cpp`
- Modify: `src/interaction/ChordPerformanceController.h`
- Modify: `src/interaction/ChordPerformanceController.cpp`
- Test: `tests/music/DiatonicChordVoicerTests.cpp`
- Test: `tests/interaction/ChordPerformanceControllerTests.cpp`

**Contract:**
```cpp
struct VoicedChord {
    // existing identity fields
    NoteSet notes{};
    std::optional<int> bassMidi{};
};

struct ActiveChord {
    int degree{};
    NoteSet notes{};
    std::optional<int> bassMidi{};
    // velocity/channel fields
};
```

**TDD:** root bass lies below harmonic voices; slash-degree bass uses scale degree pitch class; switching root→slash emits only required differential events; release always sends bass note-off; queue failure leaves active state unchanged; maximum replacement fits fixed batch.

Use MIDI channel 2 for bass and channel 1 for harmonic notes. Commit `feat: route independent chord bass voices`.

---

## Phase 2 — Scenes and durable state

### Task 10: Define the four musical factory scenes · [COMPLETED]

**Objective:** Make scenes product-level musical presets rather than generic technical examples.

**Files:**
- Modify: `src/music/HarmonyConfiguration.h`
- Modify: `src/music/HarmonyConfiguration.cpp`
- Modify: `tests/music/HarmonyConfigurationTests.cpp`

**Scene contracts:**

- **A · Diatónica:** triad, compact, fifth included, no separate bass, manual leading.
- **B · Séptimas:** seventh, compact, automatic fifth, no separate bass, nearest leading.
- **C · Lo‑Fi Warm:** degree shapes `[9, 9, 7, 9, 13, 9, 7]`, open, automatic fifth omission, root bass, nearest leading.
- **D · Jazz Tension:** degree shapes `[6/9, 11, 9, 9, 13, 11, 7]`, rootless, automatic fifth omission, root bass, nearest leading.

The recipe resolver adapts labels/qualities to major or natural minor; scene defaults do not hardcode C pitches.

**TDD:** assert all 28 defaults field-by-field, reset-degree/reset-all behavior, and representative C-major labels/notes. Commit `feat: add musical harmony factory scenes`.

### Task 11: Version HarmonyState and migrate legacy scene data · [COMPLETED]

**Objective:** Persist every new field while loading version-1 sessions exactly as they sounded before.

**Files:**
- Modify: `src/state/HarmonyState.h` (`currentVersion = 2`)
- Modify: `src/state/HarmonyState.cpp`
- Modify: `tests/state/HarmonyStateTests.cpp`

**State v2 fields per degree:**
```text
shape, inversion, style, baseOctave, qualityRule,
fifthPolicy, bassMode, slashDegree, voiceLeading
```

**Migration:**
- Accept versions 1 and 2; reject missing, malformed and future versions.
- v1 `extension=0/1` maps to `shape=triad/seventh`.
- v1 `style=0/1` maps to `compact/open`.
- Missing new fields use compatibility defaults: fifth automatic, no bass, manual voice-leading.
- A v1 state with no explicit scene nodes gets the *legacy* A/B/C/D defaults, not the new factory scene matrix.
- Serialize only v2 after loading.

Run `[state][harmony]`, processor state tests and malformed-state tests. Commit `feat: migrate harmony state to extended recipes`.

### Task 12: Version JSON presets and preserve schema 1/2 loading · [COMPLETED]

**Objective:** Export all harmony fields without breaking existing presets.

**Files:**
- Modify: `src/presets/Preset.h` (`schemaVersion = 3`)
- Modify: `src/presets/PresetSerializer.cpp`
- Modify: `tests/presets/PresetSerializerTests.cpp`

**TDD:** v3 full round trip; v2 maps extension/style and legacy defaults; v1 still defaults harmony; malformed enum/bass/degree values sanitize; unsupported v4 rejects. Ensure `fromProcessorState()` emits v3.

Run `[presets]`, `[state][harmony]`, processor state round trip. Commit `feat: serialize extended harmony presets`.

---

## Phase 3 — Lo‑Fi voicing UI

### Task 13: Extend the chord designer controls and preview · [COMPLETED]

**Objective:** Let users edit every persistent recipe/voicing field with accurate labels and visible save feedback.

**Files:**
- Modify: `src/ui/ChordDesignerPanel.h`
- Modify: `src/ui/ChordDesignerPanel.cpp`
- Modify: `src/plugin/PluginEditor.cpp`
- Modify: `tests/gui/EditorSmokeTests.cpp`
- Modify if needed: `tests/cmake/UiSyntaxVerificationContract.cmake`

**Controls and stable component IDs:**
```text
chord-shape-select: Triada, 7, 9, 11, 13, add9, 6/9, sus2, sus4
quality-select: Según escala, Mayor, Menor, Dominante, Disminuido
voicing-style-select: Compacto, Abierto, Rootless
fifth-policy-select: Auto, Incluir, Omitir
bass-mode-select: Sin bajo, Raíz, Slash
slash-degree-select: I..VII (enabled only for Slash)
voice-leading-select: Manual, Automático/nearest
inversion-select and register-select remain
```

**Behavior:** preview shows harmonic notes and a separately labeled `Bajo: C2`; changing controls previews only; save button writes once, shows `Guardado ✓`, refreshes pads and revoices a held chord only when live revoice is enabled. Unsupported combinations are disabled or sanitized visibly, not silently.

**Verification:** headless/source UI contracts, GUI smoke compilation when available; manual running GUI remains Windows validation. Commit `feat: expose jazz chord recipe editor`.

---

## Phase 4 — Temporary performance palettes

### Task 14: Implement pure chord transformations and palette definitions · [COMPLETED]

**Objective:** Define reusable, testable transformations independent of JUCE UI and MIDI transport.

**Files:**
- Create: `src/interaction/ChordTransform.h`
- Create: `src/interaction/ChordTransform.cpp`
- Modify: `CMakeLists.txt`
- Test: `tests/interaction/ChordTransformTests.cpp`

**Contracts:**
```cpp
enum class TransformPalette : std::uint8_t { basic, loFi, spice };
enum class TransformSlot : std::uint8_t { one, two, three, four, five, six, seven, eight };

struct TransformResult {
    music::VoicingSpec spec;
    std::string_view label;
};

TransformResult applyChordTransform(
    TransformPalette,
    TransformSlot,
    const music::VoicingSpec& base,
    music::Scale,
    int degree) noexcept;
```

**Palettes:**
- Basic: major/minor flip, dominant7, seventh color, add9, sus4, sus2, 6, diminished.
- Lo‑Fi: maj/min9, add9, 6/9, min11/add11, open9, rootless7, warm13, nearest-open.
- Spice (bounded to v1 recipes): dominant7, diminished7/half-dim, sus4+7, dominant9, dominant13, minor-major color fallback, rootless tension, octave/open tension. Altered b9/#9/#11 are not exposed until their recipe contract exists.

If `6` or minor-major cannot be represented by the accepted v1 recipe set, either add the minimal explicit shape in Task 3 before proceeding or replace the slot; do not encode a misleading label.

**TDD:** all 24 palette slots return deterministic, valid specs; source spec remains unchanged; diatonic/free semantics are explicit; transformed labels match resulting chord identities. Commit `feat: add performance chord color palettes`.

### Task 15: Add temporary transform lifecycle to the controller (Completed)

**Objective:** Apply/release transformations to a held chord without mutating saved scene configuration.

**Files:**
- Modify: `src/interaction/ChordPerformanceController.h`
- Modify: `src/interaction/ChordPerformanceController.cpp`
- Test: `tests/interaction/ChordPerformanceControllerTests.cpp`

**API:**
```cpp
bool beginTransform(TransformPalette, TransformSlot) noexcept;
void endTransform() noexcept;
std::optional<music::VoicingSpec> transformedSpecForActiveDegree() const noexcept;
bool commitActiveTransform(music::HarmonyConfiguration&) noexcept;
```

**Rules:**
- Transform requires an active degree; otherwise it updates visual selection only and emits no notes.
- Begin computes from the saved base spec, not from the previous temporary result; changing slots replaces the transform.
- End restores the exact saved base using differential events.
- Commit writes transformed spec to current scene/degree, clears temporary state without reverting audio, and invokes normal refresh/revoice flow.
- Scene/key/scale change and focus loss end transforms and release notes safely.
- Queue failure preserves the last successfully sounding state.

Add RED/GREEN tests for every lifecycle edge and max seven-note batches. Commit `feat: support temporary and committed chord transforms`.

### Task 16: Add the Chord Color performance panel and keyboard mappings (Completed)

**Objective:** Make palettes playable and state changes visible in-page.

**Files:**
- Create: `src/ui/ChordColorPanel.h`
- Create: `src/ui/ChordColorPanel.cpp`
- Modify: `src/ui/PerformancePanel.h`
- Modify: `src/ui/PerformancePanel.cpp`
- Modify: `src/plugin/PluginEditor.h`
- Modify: `src/plugin/PluginEditor.cpp`
- Modify: `CMakeLists.txt`
- Modify: `tests/gui/EditorSmokeTests.cpp`
- Modify UI source/syntax contracts as needed.

**UI:** palette selector `Basic / Lo‑Fi / Spice`; eight press-and-hold color buttons with stable IDs `chord-color-0..7`; visible active color and transformed chord/notes; `Fijar en grado` button disabled until a valid active transform exists; success text `Fijado en escena X · grado Y`; explicit error if persistence fails.

**Keyboard:** `A S D F G H J K` mirror button press/release. Do not capture shortcuts while a `TextEditor` or `ComboBox` has focus. Focus loss, editor close and visibility loss must release the transform and active chord.

Re-layout the editor without clipping at the existing 900×620 minimum; increase the minimum only if a GUI source/smoke test proves it necessary. Commit `feat: add playable chord color palette UI`.

---

## Phase 5 — MIDI performance mapping and processor integration

### Task 17: Build a pure fixed-capacity MIDI performance mapper · [COMPLETED]

**Objective:** Convert opt-in semantic note/CC input into degree chords and temporary colors without duplicating harmony logic.

**Files:**
- Create: `src/interaction/MidiPerformanceMapper.h`
- Create: `src/interaction/MidiPerformanceMapper.cpp`
- Modify: `CMakeLists.txt`
- Test: `tests/interaction/MidiPerformanceMapperTests.cpp`

**Rules:**
- Disabled: input buffer is unchanged.
- Enabled: notes 36–42 map to degrees 0–6; note-off releases the matching generated notes; velocity is preserved.
- CC 20–27 `>=64` begins a slot and `<64` ends it.
- Palette selection is a stable choice supplied by processor state.
- Other MIDI passes through with original sample offsets.
- Bass events are marked channel 2; harmonic notes channel 1.
- Mapping and note ownership use fixed arrays; overlapping/repeated events and all-notes-off cannot leave stuck notes.

Test sample-offset preservation, six tones+bass, transform begin/end, repeated note-on, scene/scale changes, malformed CC and all-notes-off. Commit `feat: add realtime-safe MIDI chord mapping`.

### Task 18: Add automatable MIDI-performance parameters and processor routing

**Objective:** Expose stable host state and correctly route bass around the arpeggiator.

**Files:**
- Modify: `src/parameters/ParameterIds.h`
- Modify: `src/parameters/ParameterLayout.cpp`
- Modify: `src/plugin/PluginProcessor.h`
- Modify: `src/plugin/PluginProcessor.cpp`
- Modify: `src/ui/HarmonyToolbar.h` or `src/ui/ChordColorPanel.h`
- Modify corresponding `.cpp`
- Test: `tests/parameters/ParameterLayoutTests.cpp`
- Test: `tests/plugin/ProcessorSmokeTests.cpp`
- Test: `tests/dsp/RenderSafetyTests.cpp`

**Stable APVTS parameters:**
```text
performanceMidiEnabled: bool, default false
transformPalette: choice Basic/Lo‑Fi/Spice, default Lo‑Fi
```

**Processor flow:**
```text
incoming MIDI
→ optional semantic mapper
→ split channel-2 generated bass from channel-1 chord events
→ arpeggiator handles chord events only
→ merge held bass events after arp generation
→ synth.renderNextBlock
```

Do not change behavior for default-disabled mapping. Poll/synchronize palette changes in the editor following existing key/scale automation patterns. Add component IDs and visible enabled state.

Run parameter, processor, arp and long render-safety tests. Commit `feat: integrate MIDI chord colors with audio processor`.

---

## Phase 6 — Presets, documentation and release validation

### Task 19: Update built-in sound presets to use the musical scenes · [COMPLETED]

**Objective:** Ensure shipped presets demonstrate the new harmony system rather than only new UI controls.

**Files:**
- Modify: `src/plugin/PluginEditor.cpp`
- Modify: `tests/gui/EditorSmokeTests.cpp`
- Modify: `tests/presets/PresetSerializerTests.cpp`

**Expected built-ins:**
- Default (Init) → Scene A Diatónica.
- Warm Saw Chords → Scene C Lo‑Fi Warm.
- Ambient Open Keys → Scene C, live revoice enabled.
- Arp Plucks → Scene B or an explicit lightweight extension configuration so six-tone chords do not overload the arpeggio.
- Add `Jazz Tension` preset only if the header has room and tests cover its selection.

Test that loading each preset synchronizes harmony state, controller, palette, pad labels and designer preview. Commit `feat: ship lo-fi and jazz harmony presets`.

### Task 20: Update user and architecture documentation · [COMPLETED]

**Objective:** Explain musical semantics, mappings and compatibility accurately.

**Files:**
- Modify: `README.md`
- Modify: `docs/user-guide.md`
- Modify: `docs/realtime-safety.md`
- Modify: `CHANGELOG.md`
- Create: `docs/design/lofi-jazz-harmony.md`

**Document:** recipe vs voicing; add9 vs maj9 vs dominant9; rootless/bass behavior; scene tables; Basic/Lo‑Fi/Spice mappings; keyboard and MIDI mappings; state/preset migration; altered-tension non-goals; bass/arp routing; Windows manual validation steps.

Commit `docs: explain lo-fi jazz harmony workflow`.

### Task 21: Full quality gates and independent reviews

**Objective:** Prove domain correctness, persistence compatibility, realtime safety and UI integration before release.

**Commands, in order:**
```bash
cmake --build build/linux-ninja-debug --parallel
./build/linux-ninja-debug/tests/ChordSynthTests "[music]"
./build/linux-ninja-debug/tests/ChordSynthTests "[interaction]"
./build/linux-ninja-debug/tests/ChordSynthTests "[state]"
./build/linux-ninja-debug/tests/ChordSynthTests "[presets]"
./build/linux-ninja-debug/tests/ChordSynthTests "[plugin]"
ctest --test-dir build/linux-ninja-debug --output-on-failure
git diff --check
git status --short
```

**Reviews:**
1. Specification reviewer checks every requirement and acceptance example.
2. Music-domain reviewer checks labels, intervals, omissions and major/natural-minor behavior.
3. Realtime reviewer checks `processBlock`, MIDI mapping, fixed capacities, stuck-note paths and queue failure semantics.
4. Code-quality reviewer checks duplication, enum sanitization, migrations and UI/controller separation.
5. Fix findings sequentially, rerun focused tests, then rerun the full suite.

**Windows validation (required before claiming GUI/VST3 release-ready):**
```powershell
cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release --config Release --parallel
ctest --test-dir out/build/windows-msvc-release -C Release --output-on-failure
```
Then manually smoke Standalone: all scenes, every palette slot press/release, keyboard focus-loss cleanup, MIDI note/CC mapping, arp with separate bass, preset restore, saved project restore, and no stuck notes. Validate VST3 in FL Studio/pluginval using existing project checklists. Linux syntax/source contracts do not substitute for this running Windows validation.

Final commit only after all findings are resolved: `feat: complete lo-fi jazz harmony performance system`.

---

## Files likely to change

```text
CMakeLists.txt
tests/CMakeLists.txt
src/music/ChordRecipe.{h,cpp}                    (new)
src/music/ChordVoicingEngine.{h,cpp}             (new)
src/music/VoiceLeadingResolver.{h,cpp}           (new)
src/music/VoicedChord.h
src/music/DiatonicChordVoicer.{h,cpp}
src/music/HarmonyConfiguration.{h,cpp}
src/interaction/ChordTransform.{h,cpp}           (new)
src/interaction/MidiPerformanceMapper.{h,cpp}    (new)
src/interaction/ChordPerformanceController.{h,cpp}
src/state/HarmonyState.{h,cpp}
src/presets/Preset.h
src/presets/PresetSerializer.cpp
src/parameters/ParameterIds.h
src/parameters/ParameterLayout.cpp
src/plugin/PluginProcessor.{h,cpp}
src/plugin/PluginEditor.{h,cpp}
src/ui/ChordDesignerPanel.{h,cpp}
src/ui/ChordColorPanel.{h,cpp}                   (new)
src/ui/PerformancePanel.{h,cpp}
tests/music/*Tests.cpp
tests/interaction/*Tests.cpp
tests/state/HarmonyStateTests.cpp
tests/presets/PresetSerializerTests.cpp
tests/parameters/ParameterLayoutTests.cpp
tests/plugin/ProcessorSmokeTests.cpp
tests/dsp/RenderSafetyTests.cpp
tests/gui/EditorSmokeTests.cpp
README.md
docs/user-guide.md
docs/realtime-safety.md
docs/design/lofi-jazz-harmony.md                 (new)
CHANGELOG.md
```

## Risks and mitigations

- **Stuck notes after temporary transforms:** active state must represent the last successfully enqueued chord; every replacement sends offs before ons; focus loss/all-notes-off has explicit tests.
- **State migration changes old songs:** v1 state receives legacy scene defaults and legacy per-cell mappings. Golden migration fixtures are mandatory.
- **Dense chords consume polyphony:** six tones+bass uses seven of sixteen voices; scene/preset design omits fifths and rootless voicings deliberately. Render tests cover voice stealing.
- **Arpeggiated bass sounds wrong:** channel-2 generated bass bypasses arp and is independently held/released.
- **UI and playback disagree:** all labels/previews call the same recipe+voicing pipeline as the controller; no duplicate UI chord formulas.
- **Nearest voice leading becomes nondeterministic:** bounded candidate set and explicit tie-break order; no mutable hidden voicer state.
- **Scope creep toward full jazz grammar:** altered tensions and MIDI Learn remain out of scope; fixed recipes and fixed MIDI mapping only.
- **Host automation race:** new APVTS values are read atomically; persisted harmony mutation remains message-thread/editor-owned.

## Open questions to resolve before Task 14

1. Should the Spice palette wait for altered tensions (`b9/#9/#11`) or ship as a non-altered “Tension” palette? This plan defaults to the latter to keep scope bounded.
2. Should MIDI degree triggers remain notes 36–42 or use a configurable base note? This plan defaults to fixed 36–42; configurable mapping/MIDI Learn is deferred.
3. Should root bass be enabled in all Lo‑Fi/Jazz scene cells? This plan enables it in C/D; listening tests may justify disabling it on selected degrees, but any change must be encoded as deterministic scene data and tests.
4. For natural minor V, this plan remains scale-correct (`v`, minor). Harmonic-minor dominant V is a separate future scale/mode feature, not an implicit alteration.

## Definition of done

- Every acceptance chord has the exact expected label and pitch content.
- No chord has more than six harmonic tones; optional bass is tracked separately.
- Compact/open/rootless and nearest leading are deterministic and range-safe.
- Temporary transforms restore cleanly; commit persists only the selected scene/degree.
- Keyboard and opt-in MIDI mapping work without swallowing unrelated events.
- Factory scenes sound and display consistently in major and natural minor.
- Harmony state v1/v2 and preset schema v1/v2 continue loading; new writes use HarmonyState v2 and preset schema v3.
- Full Linux headless/source/syntax suite passes.
- Windows Standalone/VST3 running validation is completed before release-ready claims.
