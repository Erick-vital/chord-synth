# ChordSynth Performance Interface Implementation Plan

> **For Hermes:** Use the `subagent-driven-development` skill to implement this plan task-by-task, with independent specification and code-quality reviews before each commit.

**Goal:** Reemplazar el editor placeholder por un instrumento Standalone/VST3 usable con siete teclas de acordes, voicing interactivo, edición por grado y controles completos del motor existente.

**Architecture:** La teoría y el voicing vivirán en clases C++ puras con estructuras de capacidad fija. Un controlador de interpretación en message thread conservará el acorde activo y enviará lotes MIDI atómicos a `UiMidiQueue`; `processBlock` seguirá limitado a drenar eventos y ejecutar DSP preasignado. El procesador será dueño del estado armónico persistente, y el editor JUCE solo presentará dicho estado y emitirá intenciones musicales.

**Tech Stack:** C++20, JUCE 9.0.1, Catch2, CMake 3.22+, APVTS/ValueTree, GitHub Actions Windows 2022, Standalone y VST3.

**Design source:** `docs/design/chord-synth-interface.md`

**Visual prototype:** `docs/design/chord-synth-interface-prototype.html`

---

## Alcance cerrado

Esta entrega incluye:

- siete teclas/grados diatónicos;
- tonalidades mayores existentes;
- escenas A–D: triadas, séptimas, abierto e inversiones;
- edición por escena y grado de calidad, extensión, inversión, distribución y registro;
- modo diatónico y modo libre para overrides como Dmaj7 dentro de C mayor;
- interacción mouse/touch y `Q–U`;
- selección de escenas con `1–4`;
- aplicación al siguiente acorde y re-voicing opcional del acorde sostenido;
- controles visuales para parámetros DSP existentes;
- persistencia y migración de estado;
- misma UI en Standalone y VST3.

No incluye secuenciador, looper, progresiones automáticas, escalas menores, 28 voicings, MIDI learn ni clonación visual de HiChord.

## Invariantes técnicos

1. La UI nunca calcula teoría directamente.
2. `processBlock` no añade allocations, locks, parsing ni acceso a componentes.
3. Todo note-off se genera a partir de las notas realmente activadas, no recalculando con el estado nuevo.
4. Un lote MIDI entra completo o no entra; no se permiten triadas parcialmente encoladas.
5. Pérdida de foco, cambio de tonalidad, cierre del editor y destrucción del controlador liberan el acorde de UI.
6. MIDI del host continúa cromático; las siete teclas son una fuente adicional de acordes.
7. IDs APVTS actuales no cambian.

## Comandos base

Baseline/headless Linux:

```bash
cmake -S . -B build -DCHORDSYNTH_BUILD_PLUGIN=OFF -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Build de producción Windows:

```powershell
cmake -B build -S . -G "Visual Studio 17 2022" -A x64 `
  -DCHORDSYNTH_BUILD_PLUGIN=ON -DBUILD_TESTING=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

---

### Task 1: Fijar contratos de notas y voicing de capacidad fija

**Objective:** Representar triadas y séptimas sin allocations ni cambiar el contrato de `Chord` existente antes de tiempo.

**Files:**
- Create: `src/music/VoicedChord.h`
- Create: `tests/music/VoicedChordTests.cpp`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

**Step 1: Write failing tests**

Definir pruebas para un tipo `NoteSet` con `std::array<int, 4>` y `count`:

```cpp
TEST_CASE("NoteSet exposes only its active MIDI notes", "[music][voicing]") {
    NoteSet notes{{60, 64, 67, 0}, 3};
    REQUIRE(notes.size() == 3);
    REQUIRE(notes[0] == 60);
    REQUIRE(notes[2] == 67);
}
```

Añadir contratos para `ChordExtension::{triad,seventh}`, `VoicingStyle::{close,open}`, `QualityRule::{diatonic,major,minor,diminished}` y `VoicingSpec{extension,inversion,style,baseOctave,qualityRule}`.

**Step 2: Verify RED**

```bash
cmake --build build --parallel
```

Expected: FAIL porque `music/VoicedChord.h` y sus tipos aún no existen.

**Step 3: Implement minimally**

Crear tipos triviales, comparables y acotados. `NoteSet::count` debe sanearse a `0..4`; no usar `std::vector`.

**Step 4: Verify GREEN**

```bash
cmake --build build --parallel
ctest --test-dir build -R VoicedChord --output-on-failure
```

Expected: PASS.

**Step 5: Commit**

```bash
git add CMakeLists.txt tests/CMakeLists.txt src/music/VoicedChord.h tests/music/VoicedChordTests.cpp
git commit -m "feat: define fixed-capacity chord voicing contracts"
```

---

### Task 2: Generar triadas, séptimas, inversiones y voicings abiertos

**Objective:** Crear un motor musical puro que produzca las notas exactas mostradas y disparadas por la UI.

**Files:**
- Create: `src/music/DiatonicChordVoicer.h`
- Create: `src/music/DiatonicChordVoicer.cpp`
- Create: `tests/music/DiatonicChordVoicerTests.cpp`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

**Step 1: RED — vertical slice de triada**

Probar `C major + grado I + triad/root/close/octave 3 -> C3 E3 G3` y `grado ii -> D3 F3 A3`.

**Step 2: GREEN — triadas diatónicas**

Reutilizar las reglas de `MajorScaleChordMap`, no duplicar tablas de nombres/calidades sin extraer una fuente común.

**Step 3: RED/GREEN — séptimas**

Añadir casos:

```text
I   Cmaj7  = C E G B
ii  Dm7    = D F A C
V   G7     = G B D F
vii Bm7b5  = B D F A
```

**Step 4: RED/GREEN — transformación de voicing**

- primera inversión de C: `E3 G3 C4`;
- segunda inversión de C: `G3 C4 E4`;
- abierto de C: `C3 G3 E4`;
- inversión inválida para el número de notas: clamp determinista;
- octavas 2–4 y límites MIDI `0..127`.

**Step 5: RED/GREEN — calidad libre**

En contexto C mayor, grado ii con `QualityRule::major` y séptima debe producir `D3 F#3 A3 C#4`, label `Dmaj7`. El modo diatónico debe seguir produciendo Dm7.

**Step 6: Full verification**

```bash
cmake --build build --parallel
ctest --test-dir build -R "DiatonicChordVoicer|MajorScaleChordMap" --output-on-failure
ctest --test-dir build --output-on-failure
```

**Step 7: Commit**

```bash
git add CMakeLists.txt tests/CMakeLists.txt src/music/DiatonicChordVoicer.* tests/music/DiatonicChordVoicerTests.cpp
git commit -m "feat: generate diatonic and custom chord voicings"
```

---

### Task 3: Modelar cuatro escenas y overrides por grado

**Objective:** Dar una fuente única de verdad a escenas A–D y al diseñador por grado.

**Files:**
- Create: `src/music/HarmonyConfiguration.h`
- Create: `src/music/HarmonyConfiguration.cpp`
- Create: `tests/music/HarmonyConfigurationTests.cpp`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

**Step 1: RED — defaults**

Probar una matriz fija `4 escenas × 7 grados`:

- A: triada/root/close;
- B: seventh/root/close;
- C: triad/root/open;
- D: triad/first-inversion/close.

**Step 2: GREEN — configuración acotada**

Usar `std::array<SceneConfiguration, 4>` y `std::array<VoicingSpec, 7>`. No mapas ni claves dinámicas.

**Step 3: RED/GREEN — edición localizada**

Cambiar escena B, grado ii a `major + seventh` debe modificar solo B/ii; A/ii y B/I permanecen intactos. `resetDegree(scene, degree)` restaura el default de esa celda.

**Step 4: RED/GREEN — validación**

Sanear escena `0..3`, grado `0..6`, octava `2..4` e inversión válida. APIs inválidas deben devolver `false` o un resultado documentado, nunca indexar fuera de rango.

**Step 5: Verify and commit**

```bash
cmake --build build --parallel
ctest --test-dir build -R HarmonyConfiguration --output-on-failure
git add CMakeLists.txt tests/CMakeLists.txt src/music/HarmonyConfiguration.* tests/music/HarmonyConfigurationTests.cpp
git commit -m "feat: add per-scene per-degree harmony configuration"
```

---

### Task 4: Hacer atómicos los lotes de MIDI de UI

**Objective:** Evitar acordes incompletos y note-offs parciales cuando la cola está cerca de su capacidad.

**Files:**
- Modify: `src/dsp/UiMidiQueue.h`
- Modify: `src/dsp/UiMidiQueue.cpp`
- Modify: `tests/dsp/UiMidiQueueTests.cpp`

**Step 1: RED**

Añadir `tryPushBatch(std::span<const juce::MidiMessage>) noexcept` y probar:

1. un lote de cuatro conserva orden;
2. con espacio insuficiente no escribe ningún evento;
3. lote vacío es éxito sin cambio;
4. lote mayor que capacidad falla;
5. no se rompe el productor único/consumidor único existente.

**Step 2: GREEN**

Reservar todo el lote con una sola llamada `prepareToWrite(count, ...)`; escribir en los dos segmentos y llamar `finishedWrite(count)` solo si cabe completo.

**Step 3: Verify**

```bash
cmake --build build --parallel
ctest --test-dir build -R UiMidiQueue --output-on-failure
ctest --test-dir build --output-on-failure
```

**Step 4: Commit**

```bash
git add src/dsp/UiMidiQueue.* tests/dsp/UiMidiQueueTests.cpp
git commit -m "feat: enqueue UI MIDI chord events atomically"
```

---

### Task 5: Crear el controlador de interpretación y liberación exacta

**Objective:** Convertir press/release/re-voice en comandos MIDI sin acoplar componentes JUCE a teoría musical.

**Files:**
- Create: `src/interaction/ChordPerformanceController.h`
- Create: `src/interaction/ChordPerformanceController.cpp`
- Create: `tests/interaction/ChordPerformanceControllerTests.cpp`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

**Step 1: RED — press/release**

Construir con `HarmonyConfiguration`, `DiatonicChordVoicer` y una salida de comandos inyectable. Probar:

- `pressDegree(0)` en C/A produce note-on 60, 64, 67;
- `releaseActiveChord()` produce los note-off de esas mismas notas;
- cambiar tonalidad o escena antes de soltar no cambia los note-off;
- press sobre otro grado libera primero el acorde activo y luego activa el nuevo;
- press repetido del mismo grado no duplica note-ons.

**Step 2: GREEN — acorde activo fijo**

Conservar `std::optional<ActiveChord>{degree, NoteSet}` en message thread. El modo inicial es una sola tecla de acorde activa, apropiado para progresiones y compatible con 16 voces.

**Step 3: RED/GREEN — cambio de escena**

- por defecto `setScene()` solo afecta al siguiente press;
- con `liveRevoice=false`, un acorde sostenido queda intacto;
- con `liveRevoice=true`, calcular diferencia ordenada entre old/new, conservar notas comunes y encolar off antes que on;
- C triad -> Cmaj7 solo añade B;
- C root -> primera inversión apaga C3 y añade C4 sin retrigger E/G.

**Step 4: RED/GREEN — cleanup**

`allNotesOff()`, destructor y fallo de enqueue deben dejar estado interno coherente. Si un batch no cabe, no actualizar `ActiveChord` como si hubiera sonado.

**Step 5: Verify and commit**

```bash
cmake --build build --parallel
ctest --test-dir build -R ChordPerformanceController --output-on-failure
ctest --test-dir build --output-on-failure
git add CMakeLists.txt tests/CMakeLists.txt src/interaction tests/interaction
git commit -m "feat: add safe real-time chord performance controller"
```

---

### Task 6: Persistir configuración armónica con migración legacy

**Objective:** Mantener escenas, overrides y modo libre aunque el editor se cierre o el host restaure una sesión.

**Files:**
- Create: `src/state/HarmonyState.h`
- Create: `src/state/HarmonyState.cpp`
- Create: `tests/state/HarmonyStateTests.cpp`
- Modify: `src/plugin/PluginProcessor.h`
- Modify: `src/plugin/PluginProcessor.cpp`
- Modify: `tests/plugin/ProcessorSmokeTests.cpp`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

**Step 1: RED — ValueTree round-trip**

Probar schema `HarmonyState` versión 1 con selected scene, live-revoice, rule y 28 specs. Un round-trip debe preservar un override no default.

**Step 2: RED — migración**

Cargar blobs APVTS existentes sin hijo `HarmonyState` sobre una instancia configurada de forma conflictiva. Debe restaurar defaults armónicos explícitos y conservar parámetros APVTS legacy.

**Step 3: GREEN — ownership**

El processor será dueño de `HarmonyState` y expondrá acceso limitado para el editor. En `getStateInformation`, anexar el hijo al copy de APVTS; en `setStateInformation`, validar/migrar fuera de `processBlock` antes de reemplazar estado.

**Step 4: RED/GREEN — malformed state**

Versión desconocida, índices fuera de rango y tipos inválidos no deben contaminar el estado. Mantener los guards actuales para blob nulo, tamaño cero y root incorrecto.

**Step 5: Verify and commit**

```bash
cmake --build build --parallel
ctest --test-dir build -R "HarmonyState|Processor" --output-on-failure
ctest --test-dir build --output-on-failure
git add CMakeLists.txt tests/CMakeLists.txt src/state tests/state src/plugin/PluginProcessor.* tests/plugin/ProcessorSmokeTests.cpp
git commit -m "feat: persist versioned harmony performance state"
```

---

### Task 7: Construir la tecla JUCE de acorde con semántica press/release

**Objective:** Crear el control táctil base sin depender de `Button::onClick` para eventos sostenidos.

**Files:**
- Create: `src/ui/ChordKeyComponent.h`
- Create: `src/ui/ChordKeyComponent.cpp`
- Create: `src/ui/ChordSynthLookAndFeel.h`
- Create: `src/ui/ChordSynthLookAndFeel.cpp`
- Modify: `CMakeLists.txt`

**Step 1: Implement component contract**

`ChordKeyComponent` recibe label de grado, nombre, notas y atajo; expone callbacks `onPress`, `onRelease`, `onSelect`. Sobrescribir `mouseDown`, `mouseUp`, `mouseExit` solo cuando se pierde captura, `focusLost` y destructor. El callback de release debe ser idempotente.

**Step 2: Implement visual states**

Estados normal, hover, focus y pressed; pressed debe desplazarse visualmente y usar el acento. No dibujar teoría dentro del componente.

**Step 3: Add production-only sources**

Añadir los archivos UI a `target_sources(ChordSynth PRIVATE ...)`, no a las bibliotecas headless que compilan contra el APVTS sustituto.

**Step 4: Windows compile gate**

Ejecutar workflow `Windows Build & Test`; expected: compilan `ChordSynth_Standalone` y `ChordSynth_VST3` sin warnings nuevos.

**Step 5: Commit**

```bash
git add CMakeLists.txt src/ui/ChordKeyComponent.* src/ui/ChordSynthLookAndFeel.*
git commit -m "feat: add tactile JUCE chord key component"
```

---

### Task 8: Construir el panel de performance de siete teclas y escenas

**Objective:** Hacer visible y tocable el flujo principal del prototipo.

**Files:**
- Create: `src/ui/PerformancePanel.h`
- Create: `src/ui/PerformancePanel.cpp`
- Modify: `src/plugin/PluginEditor.h`
- Modify: `src/plugin/PluginEditor.cpp`
- Modify: `CMakeLists.txt`

**Step 1: Layout**

Implementar header “Ahora”, toggle live re-voice, cuatro botones de escena y siete `ChordKeyComponent`. Usar `FlexBox`/`Grid` o layout determinista en `resized()`; mínimo 900×620, objetivo 1180×760.

**Step 2: Wire controller**

El editor crea `ChordPerformanceController` con referencias propiedad del processor. `PerformancePanel` solo llama press/release/select/scene. Refrescar labels desde el voicer cuando cambian tonalidad, escena u override.

**Step 3: Keyboard shortcuts**

- `Q–U`: press/release de grados;
- `1–4`: escenas;
- ignorar auto-repeat;
- no capturar cuando un ComboBox/editor tiene foco;
- `focusLost`, editor hidden y destructor llaman `allNotesOff()`.

Usar seguimiento explícito de teclas abajo; no depender de un `keyPressed` sin evento de release. Si JUCE no entrega key-up directamente al componente, usar un `Timer` de message thread que compara `KeyPress::isKeyCurrentlyDown` y emite transiciones.

**Step 4: Manual acceptance on Windows**

- mantener I: C–E–G;
- soltar: release audible;
- Q seguido rápidamente por W: no queda C activo;
- Alt-Tab mientras se sostiene Q: no quedan notas;
- escena B + V: G7;
- live re-voice off/on responde según diseño.

**Step 5: Commit**

```bash
git add CMakeLists.txt src/ui/PerformancePanel.* src/plugin/PluginEditor.*
git commit -m "feat: add seven-key chord performance interface"
```

---

### Task 9: Añadir toolbar armónico y diseñador por grado

**Objective:** Permitir cambiar contexto y modificar el voicing de cada tecla sin abandonar la interpretación.

**Files:**
- Create: `src/ui/HarmonyToolbar.h`
- Create: `src/ui/HarmonyToolbar.cpp`
- Create: `src/ui/ChordDesignerPanel.h`
- Create: `src/ui/ChordDesignerPanel.cpp`
- Modify: `src/plugin/PluginEditor.h`
- Modify: `src/plugin/PluginEditor.cpp`
- Modify: `CMakeLists.txt`

**Step 1: Toolbar**

Conectar tonalidad a `parameters::ids::key` mediante `ComboBoxAttachment`. Antes de una edición de tonalidad originada en UI, liberar el acorde activo. Detectar automatización externa sin tocar componentes desde audio thread: usar listener + `AsyncUpdater` o polling de `Timer` en message thread.

**Step 2: Rule mode**

Diatónico deshabilita calidad manual; Libre habilita mayor/menor/disminuido. No mostrar una opción que aún no tenga efecto.

**Step 3: Designer**

Mostrar grado seleccionado, extensión, calidad, inversión, abierto/cerrado y octava. Cambios son preview local; `Guardar en este grado` escribe una sola celda scene/degree y muestra confirmación visible; `Restaurar` recupera su default.

**Step 4: Held chord semantics**

Guardar un cambio solo revoicea el acorde actual si live re-voice está activo. En caso contrario, se escucha al siguiente press.

**Step 5: Verify**

En Windows, crear Dmaj7 en B/ii dentro de C major, guardar, tocar ii, cerrar/reabrir editor y comprobar que sigue configurado.

**Step 6: Commit**

```bash
git add CMakeLists.txt src/ui/HarmonyToolbar.* src/ui/ChordDesignerPanel.* src/plugin/PluginEditor.*
git commit -m "feat: add interactive per-degree chord designer"
```

---

### Task 10: Exponer los parámetros de sonido existentes

**Objective:** Sustituir el backend invisible por controles APVTS utilizables y automatizables.

**Files:**
- Create: `src/ui/SoundPanel.h`
- Create: `src/ui/SoundPanel.cpp`
- Modify: `src/plugin/PluginEditor.h`
- Modify: `src/plugin/PluginEditor.cpp`
- Modify: `CMakeLists.txt`

**Step 1: Oscillator/filter**

Añadir attachments para waveform, detune, cutoff y resonance. Usar skew/rangos del APVTS; los labels presentan unidades, no duplican límites como lógica independiente.

**Step 2: Arpeggiator**

Añadir enabled, mode, rate y gate. `Off` visual equivale a `arp_enabled=false`; los otros modos seleccionan choice y activan el bool.

**Step 3: Effects**

Mostrar controles principales siempre: chorus mix, delay mix y reverb mix. Los parámetros avanzados existentes aparecen en panel expandible, no compiten con las siete teclas.

**Step 4: Attachment lifetime**

Declarar attachments después de sus controles y antes de cualquier objeto que necesite destruirse primero; documentar orden de miembros. Nunca escribir raw atomics desde componentes.

**Step 5: Windows verification**

Mover cada control y comprobar cambio audible, automatización VST3 y restauración de sesión. Ejecutar pluginval después de cambios de layout/estado.

**Step 6: Commit**

```bash
git add CMakeLists.txt src/ui/SoundPanel.* src/plugin/PluginEditor.*
git commit -m "feat: expose synth effects and arpeggiator controls"
```

---

### Task 11: Integrar preset, estado visible y layout adaptable

**Objective:** Completar el chrome de producto sin convertir menús genéricos de JUCE en la navegación principal.

**Files:**
- Create: `src/ui/HeaderBar.h`
- Create: `src/ui/HeaderBar.cpp`
- Modify: `src/plugin/PluginEditor.h`
- Modify: `src/plugin/PluginEditor.cpp`
- Modify: `src/presets/Preset.h`
- Modify: `src/presets/PresetSerializer.cpp`
- Modify: `tests/presets/PresetSerializerTests.cpp`
- Modify: `CMakeLists.txt`

**Step 1: RED — preset schema migration**

Subir schema solo si el JSON externo incluye armonía. Probar lectura de schema 1 con defaults armónicos y round-trip del nuevo schema sin perder parámetros existentes.

**Step 2: GREEN — serialization**

Serializar escenas/overrides en sección `harmony`, con arrays de longitud fija y validación. No incluir estado efímero como acorde actualmente sostenido.

**Step 3: Header**

Añadir marca, preset actual, estado Audio activo/inactivo y acción Audio/MIDI. En Standalone, abrir el selector de dispositivo JUCE; en VST3, mostrar que el dispositivo lo controla el host en vez de ofrecer una acción inválida.

**Step 4: Resize**

Conservar jerarquía a tamaños intermedios: primero reducir panel avanzado, nunca ocultar las siete teclas. Guardar tamaño del editor si JUCE/host lo permite sin añadirlo al contrato de preset musical.

**Step 5: Commit**

```bash
git add CMakeLists.txt src/ui/HeaderBar.* src/plugin/PluginEditor.* src/presets tests/presets
git commit -m "feat: complete ChordSynth product interface and presets"
```

---

### Task 12: Añadir contratos de build GUI y smoke test de editor

**Objective:** Evitar volver a publicar un ejecutable que solo muestra “ChordSynth”.

**Files:**
- Create: `tests/cmake/ProductionUiSourcesContract.cmake`
- Create: `tests/gui/EditorSmokeTests.cpp` (solo target de producción)
- Modify: `tests/CMakeLists.txt`
- Modify: `.github/workflows/windows-build.yml`

**Step 1: RED — source contract**

El test CMake debe fallar si production target no incluye `PluginEditor.cpp`, `PerformancePanel.cpp`, `ChordKeyComponent.cpp`, `HarmonyToolbar.cpp`, `ChordDesignerPanel.cpp` y `SoundPanel.cpp`.

**Step 2: GREEN — source contract**

Registrar `ProductionUiSourcesContract` junto a los contratos headless existentes.

**Step 3: GUI smoke**

En Windows/full JUCE, construir un test que inicialice `ScopedJuceInitialiser_GUI`, cree processor/editor y compruebe:

- editor no nulo;
- tamaño mínimo;
- siete componentes con IDs accesibles `degree-0..degree-6`;
- cuatro escenas;
- controles key/waveform/cutoff presentes.

El test no afirma audio por píxeles; la lógica musical ya está cubierta headless.

**Step 4: CI artifacts**

Hacer `if-no-files-found: error`; ejecutar GUI smoke antes de archivar. Mantener Standalone y VST3 como artefactos separados o dentro del mismo ZIP con nombres claros.

**Step 5: Commit**

```bash
git add tests .github/workflows/windows-build.yml
git commit -m "test: guard production chord interface in Windows builds"
```

---

### Task 13: Validación musical, realtime y documentación final

**Objective:** Demostrar que la interfaz funciona de extremo a extremo en Standalone y VST3.

**Files:**
- Modify: `docs/user-guide.md`
- Create: `docs/validation/interface-v1.md`
- Modify: `docs/validation/standalone-v1.md`
- Modify: `docs/validation/fl-studio-v1.md`
- Modify: `docs/realtime-safety.md`

**Step 1: Full Linux gates**

```bash
cmake -S . -B build -DCHORDSYNTH_BUILD_PLUGIN=OFF -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Expected: todos los tests headless y contratos CMake pasan.

**Step 2: Windows gates**

Ejecutar build Release, CTest, GUI smoke y pluginval. Verificar que el artefacto publicado contiene un editor funcional, no el placeholder.

**Step 3: Manual Standalone matrix**

Documentar resultado real para:

- Audio/MIDI configurado;
- mouse down/up;
- `Q–U` y `1–4`;
- foco perdido;
- C major triad/seventh/open/inversion;
- override libre Dmaj7;
- re-voice off/on;
- arpegiador;
- guardar/cargar preset;
- cierre sin notas colgadas.

**Step 4: FL Studio VST3 matrix**

Validar misma UI, input cromático del Piano Roll, automation APVTS, guardado/reapertura de proyecto y ausencia de controles Audio/MIDI inválidos dentro del plugin.

**Step 5: Realtime audit**

Confirmar por diff/revisión que `processBlock` solo drena cola y ejecuta DSP preasignado. Probar render repetido con interacción UI simulada y muestras finitas. Revisar `UiMidiQueue` como SPSC: una sola fuente productora desde message thread.

**Step 6: Independent reviews**

Solicitar en orden:

1. revisión de cumplimiento contra `docs/design/chord-synth-interface.md`;
2. revisión de calidad, realtime safety y causalidad de tests;
3. re-ejecución del controlador de tests, `git diff --check` y `git status --short`.

**Step 7: Commit**

```bash
git add docs
git commit -m "docs: validate the ChordSynth performance interface"
```

---

## Orden de entrega recomendado

- **Primer corte usable:** Tasks 1–8. Ya permite tocar siete acordes y cambiar escenas.
- **Corte expresivo:** Tasks 9–10. Añade edición por grado y controles de timbre.
- **Corte distribuible:** Tasks 11–13. Presets, CI GUI, validación Windows/VST3 y documentación.

No comenzar una tarea si la anterior no tiene RED/GREEN, suite completa y revisión del árbol real. El primer artefacto que se entregue a usuarios debe incluir al menos Tasks 1–8 y el smoke test de Task 12; nunca volver a publicar el editor placeholder como aplicación terminada.
