# ChordSynth Implementation Plan

> **For Hermes:** Use the `subagent-driven-development` skill to implement this plan task-by-task, with independent specification and code-quality reviews before each commit.

**Goal:** Construir un instrumento musical inspirado en la filosofía de HiChord, con siete pads de acordes diatónicos, un motor de síntesis reutilizable y dos formatos producidos desde la misma base de código: aplicación Standalone y plugin VST3 para FL Studio.

**Architecture:** Un único `juce::AudioProcessor` contiene el motor musical, el sintetizador y la cadena DSP. `juce_add_plugin(... FORMATS Standalone VST3)` genera los dos hosts desde el mismo código. La teoría musical se mantiene como C++ puro y determinista; la interfaz solo emite intenciones musicales y nunca implementa armonía ni DSP.

**Tech Stack:** C++20, JUCE 9.0.1, CMake 3.22+, Visual Studio 2022, VST3, Catch2 3.15.3, pluginval 1.0.4, GitHub Actions Windows.

---

## 1. Contexto y decisiones de producto

### Nombre y ubicación

- Nombre provisional: `ChordSynth`.
- Repositorio: `/home/erickesc/repos/chord-synth/`.
- El nombre se puede cambiar antes de fijar los identificadores públicos del plugin.
- Los códigos VST3 `PLUGIN_MANUFACTURER_CODE` y `PLUGIN_CODE` deben quedar estables después de la primera versión distribuida.

### Fuente de inspiración

Documento primario consultado:

- HiChord Manual & User Guide, revisión 2.8: <https://hichord.shop/pages/manual#quickstart>

Conceptos que sí se adoptan:

- Siete pads representan los grados diatónicos I, ii, iii, IV, V, vi y vii°.
- Cambio de tonalidad sin cambiar la relación entre pads.
- Transformaciones de acordes, inversiones y voice leading como evolución del producto.
- Arquitectura separada en lógica musical, voces, envolvente, filtro, efectos y salida.
- Un mismo instrumento puede ser controlado por la UI o por MIDI del DAW.
- La expresividad musical es parte del producto, no solo una colección de osciladores.

Conceptos que no entran en la primera versión:

- No se clonará la UI, marca, presets ni comportamiento exacto de HiChord.
- No habrá looper, batería, vocoder, sampler, secuenciador ni 28 voicings en V0/V1.
- No habrá versión web ni WASM inicialmente.
- No habrá AU, AAX ni CLAP inicialmente.
- No se copiarán nombres comerciales, muestras ni recursos gráficos de HiChord.

### Alcance por entregable

#### V0 — “primer acorde audible”

La aplicación Standalone debe:

1. Abrir un dispositivo de audio.
2. Mostrar siete pads y la tonalidad actual.
3. Mapear C mayor a C, Dm, Em, F, G, Am y Bdim.
4. Reproducir una tríada al mantener un pad.
5. Liberar las notas al soltarlo.
6. Usar un oscilador seno polifónico y una envolvente corta para evitar clics.
7. Pasar pruebas unitarias y un smoke test de render offline.

#### V1 — instrumento utilizable

Standalone y VST3 deben compartir:

- Entrada MIDI cromática desde teclado o Piano Roll.
- Siete pads de acordes en la UI.
- 12 tonalidades mayores.
- Sine, saw, square y triangle.
- Polifonía mínima de 16 voces para permitir acordes superpuestos.
- ADSR automatizable.
- Filtro low-pass con cutoff y resonance.
- Detune estéreo por voz.
- Ganancia master con smoothing.
- Presets versionados.
- Estado restaurable por el host.
- Validación con pluginval y prueba manual en FL Studio.

#### V1.1 — color y movimiento

- Chorus.
- Delay sincronizable al BPM del host.
- Reverb.
- Arpegiador Up, Down, Up/Down y Random.
- Rate 1/4, 1/8 y 1/16.

#### Posterior

- Escalas adicionales.
- Joystick/XY para transformaciones de acordes.
- Inversiones y voice leading inteligente.
- FM.
- Sampler.
- Sequencer.
- Looper.
- Vocoder.
- Modulation matrix, LFO, wavetables, granular, MPE y microtonalidad.

## 2. Arquitectura objetivo

```text
UI pads / teclado del host
            |
            v
       NoteCommand
            |
            v
+---------------------------+
| Music Engine              |
| key, scale, chord mapping |
| inversions, voice leading |
+---------------------------+
            |
            v MIDI note events
+---------------------------+
| Synth Engine              |
| allocator + 16 voices     |
| oscillator pair + ADSR    |
+---------------------------+
            |
            v stereo audio
+---------------------------+
| DSP Chain                 |
| filter -> chorus -> delay |
| -> reverb -> master gain  |
+---------------------------+
            |
            v
 Standalone device / VST3 host
```

### Reglas de dependencia

```text
ui -> plugin -> music
ui -> plugin -> parameters
plugin -> music
plugin -> synth
synth -> dsp
music -X-> JUCE UI
music -X-> plugin host
DSP -X-> UI
```

`music` debe poder probarse sin iniciar JUCE GUI ni un dispositivo de audio.

### Contratos principales

```cpp
// src/music/Chord.h
#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace chordsynth::music {

enum class ChordQuality : std::uint8_t { major, minor, diminished };

struct Chord {
    int degree{};                    // 0..6
    int rootMidi{};                  // 0..127
    ChordQuality quality{};
    std::array<int, 3> midiNotes{};  // root position in V0
    std::string label;
};

} // namespace chordsynth::music
```

```cpp
// src/music/MajorScaleChordMap.h
#pragma once

#include "Chord.h"

namespace chordsynth::music {

class MajorScaleChordMap {
public:
    [[nodiscard]] Chord chordForDegree(
        int tonicPitchClass,
        int baseOctave,
        int degree) const;
};

} // namespace chordsynth::music
```

```cpp
// src/dsp/UiMidiQueue.h
#pragma once

#include <array>
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

namespace chordsynth::dsp {

class UiMidiQueue {
public:
    bool push(const juce::MidiMessage& message) noexcept;
    void drainTo(juce::MidiBuffer& destination, int sampleOffset) noexcept;

private:
    static constexpr int capacity = 256;
    juce::AbstractFifo fifo{capacity};
    std::array<juce::MidiMessage, capacity> events{};
};

} // namespace chordsynth::dsp
```

La implementación de la cola debe ser acotada y no asignar memoria en el audio thread.

### Cadena de audio V1

```text
Incoming host MIDI + bounded UI event queue
-> ChordSynthesiser
-> global low-pass filter
-> chorus
-> tempo-aware delay
-> reverb
-> smoothed master gain
-> output
```

En V0 solo se habilitan `ChordSynthesiser -> master gain`. Las etapas posteriores se agregan verticalmente con pruebas antes de conectarlas.

## 3. Restricciones de tiempo real

Dentro de `prepareToPlay`, `processBlock` y los métodos de render de voz:

- No realizar `new`, crecimiento de `std::vector`, parsing JSON ni carga de archivos.
- No tomar mutexes ni esperar a otros threads.
- No hacer I/O, logging, llamadas de red ni acceso al filesystem.
- Preasignar voces, buffers, delays y colas.
- Usar `juce::SmoothedValue` para parámetros que puedan producir zipper noise.
- Consumir parámetros del `AudioProcessorValueTreeState` mediante atomics.
- Hacer el DSP dependiente de `sampleRate` y tamaño de bloque.
- Activar `juce::ScopedNoDenormals` en `processBlock`.
- Limpiar canales de salida no escritos.
- Mantener la salida finita y acotada; ningún bloque puede contener NaN/Inf.

## 4. Estructura prevista

```text
chord-synth/
├── CMakeLists.txt
├── CMakePresets.json
├── LICENSE
├── README.md
├── cmake/
│   └── Dependencies.cmake
├── external/
│   └── JUCE/                    # git submodule pinned to 9.0.1
├── src/
│   ├── music/
│   │   ├── Chord.h
│   │   ├── MajorScaleChordMap.h
│   │   ├── MajorScaleChordMap.cpp
│   │   ├── NoteMath.h
│   │   └── NoteMath.cpp
│   ├── dsp/
│   │   ├── ChordSound.h
│   │   ├── ChordVoice.h
│   │   ├── ChordVoice.cpp
│   │   ├── Oscillator.h
│   │   ├── Oscillator.cpp
│   │   ├── UiMidiQueue.h
│   │   └── UiMidiQueue.cpp
│   ├── parameters/
│   │   ├── ParameterIds.h
│   │   ├── ParameterLayout.h
│   │   └── ParameterLayout.cpp
│   ├── presets/
│   │   ├── Preset.h
│   │   ├── PresetSerializer.h
│   │   └── PresetSerializer.cpp
│   ├── plugin/
│   │   ├── PluginProcessor.h
│   │   ├── PluginProcessor.cpp
│   │   ├── PluginEditor.h
│   │   └── PluginEditor.cpp
│   └── ui/
│       ├── ChordPad.h
│       ├── ChordPad.cpp
│       ├── ChordPadGrid.h
│       └── ChordPadGrid.cpp
├── tests/
│   ├── CMakeLists.txt
│   ├── music/
│   │   ├── MajorScaleChordMapTests.cpp
│   │   └── NoteMathTests.cpp
│   ├── dsp/
│   │   ├── OscillatorTests.cpp
│   │   ├── UiMidiQueueTests.cpp
│   │   └── RenderSafetyTests.cpp
│   ├── parameters/
│   │   └── ParameterLayoutTests.cpp
│   ├── presets/
│   │   └── PresetSerializerTests.cpp
│   └── plugin/
│       └── ProcessorSmokeTests.cpp
├── tools/
│   └── validate-plugin.ps1
├── docs/
│   ├── architecture.md
│   ├── realtime-safety.md
│   ├── references.md
│   └── plans/
│       └── 2026-08-20-chord-synth-implementation-plan.md
└── .github/
    └── workflows/
        └── windows-build.yml
```

## 5. Build reproducible

### Dependencias fijadas al redactar este plan

- JUCE `9.0.1`, publicado el 10 de agosto de 2026.
- Catch2 `v3.15.3`, publicado el 26 de julio de 2026.
- pluginval `v1.0.4` para validación VST3.
- CMake mínimo `3.22`, alineado con el ejemplo CMake oficial de JUCE consultado.

No usar `master` ni descargar dependencias sin versión en CI.

### Presets de CMake

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "windows-msvc-debug",
      "generator": "Visual Studio 17 2022",
      "binaryDir": "${sourceDir}/build/windows-msvc-debug",
      "cacheVariables": {
        "CMAKE_CXX_STANDARD": "20",
        "CMAKE_CXX_STANDARD_REQUIRED": "ON",
        "BUILD_TESTING": "ON"
      }
    },
    {
      "name": "windows-msvc-release",
      "inherits": "windows-msvc-debug",
      "binaryDir": "${sourceDir}/build/windows-msvc-release"
    }
  ],
  "buildPresets": [
    {
      "name": "windows-debug",
      "configurePreset": "windows-msvc-debug",
      "configuration": "Debug"
    },
    {
      "name": "windows-release",
      "configurePreset": "windows-msvc-release",
      "configuration": "Release"
    }
  ],
  "testPresets": [
    {
      "name": "windows-debug",
      "configurePreset": "windows-msvc-debug",
      "configuration": "Debug",
      "output": { "outputOnFailure": true }
    }
  ]
}
```

### Comandos de calidad

```powershell
cmake --preset windows-msvc-debug
cmake --build --preset windows-debug --parallel
ctest --preset windows-debug
cmake --preset windows-msvc-release
cmake --build --preset windows-release --parallel
```

La prueba real en FL Studio se realiza sobre el artefacto Release, no sobre Debug.

## 6. Estrategia de pruebas

### Pirámide

1. Pruebas unitarias rápidas de teoría musical y serialización.
2. Pruebas DSP offline a varias sample rates y tamaños de bloque.
3. Smoke tests del `AudioProcessor` sin UI.
4. pluginval sobre el VST3 Release.
5. Prueba manual del Standalone.
6. Prueba manual de automatización y restauración en FL Studio.

### Matriz DSP mínima

- Sample rates: 44.1 kHz, 48 kHz y 96 kHz.
- Tamaños de bloque: 1, 16, 64, 256, 512 y 1024 samples.
- Entrada: silencio, una nota, tríada, acordes solapados y note-off.
- Aserciones: no NaN/Inf, energía audible tras note-on, decaimiento tras note-off, amplitud acotada y canales estéreo válidos.

### Disciplina TDD por tarea

Cada comportamiento sigue este ciclo:

1. Escribir una prueba pequeña que describa el contrato.
2. Ejecutar solo esa prueba y observar que falla por la ausencia del comportamiento.
3. Implementar lo mínimo para pasar.
4. Ejecutar la prueba específica.
5. Ejecutar la suite completa.
6. Refactorizar solo con todo verde.
7. Commit pequeño y verificable.

No se considera RED un error de compilación provocado por typo o configuración rota; el fallo debe demostrar la capacidad ausente.

## 7. Plan de implementación detallado

### Task 1: Confirmar identidad, licencia y plataforma de release

**Objective:** Cerrar decisiones que son costosas de cambiar después de publicar un plugin.

**Files:**
- Create: `README.md`
- Create: `LICENSE`
- Create: `docs/references.md`

**Steps:**

1. Confirmar nombre definitivo, fabricante y licencia del producto.
2. Elegir códigos VST3 de cuatro caracteres y documentar que serán inmutables.
3. Revisar por separado la licencia de JUCE y la del VST3 SDK; la licencia MIT del SDK VST3 no elimina las obligaciones de licencia de JUCE.
4. Documentar Windows x64 + Visual Studio 2022 como plataforma de release V1.
5. Registrar el manual HiChord solo como referencia funcional, no como especificación que se deba copiar.
6. Commit: `docs: define product identity and licensing constraints`.

**Exit:** No quedan placeholders de fabricante/licencia en CMake.

### Task 2: Inicializar Git y fijar JUCE

**Objective:** Crear un repositorio reproducible sin depender de ramas flotantes.

**Files:**
- Create: `.gitignore`
- Create: `.gitmodules`
- Create: `external/JUCE/`

**Steps:**

```bash
cd /home/erickesc/repos/chord-synth
git init
git submodule add https://github.com/juce-framework/JUCE.git external/JUCE
cd external/JUCE
git checkout 9.0.1
cd ../..
git add .gitmodules external/JUCE .gitignore
git commit -m "build: pin JUCE 9.0.1"
```

`.gitignore` debe excluir `build/`, `.vs/`, `out/`, artefactos VST3 y archivos de usuario de Visual Studio.

**Verification:** `git submodule status` muestra el commit correspondiente a `9.0.1` sin prefijo `+`.

**Estado (2026-08-20): completada.**
- Git inicializado con commit base de documentación e identidad (`75faa5c`).
- `.gitignore` configurado para ignorar builds, artefactos VST3, `.vs/` y temporales de IDE/CMake.
- Submódulo `external/JUCE` fijado en tag `9.0.1` (`e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8`).
- Commit realizado: `8faecae` (`build: pin JUCE 9.0.1`).

### Task 3: Crear el target común Standalone + VST3

**Objective:** Probar desde el inicio que una sola definición JUCE genera ambos formatos.

**Files:**
- Create: `CMakeLists.txt`
- Create: `CMakePresets.json`
- Create: `src/plugin/PluginProcessor.h`
- Create: `src/plugin/PluginProcessor.cpp`
- Create: `src/plugin/PluginEditor.h`
- Create: `src/plugin/PluginEditor.cpp`

**CMake mínimo:**

```cmake
cmake_minimum_required(VERSION 3.22)
project(ChordSynth VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(external/JUCE)
include(CTest)

juce_add_plugin(ChordSynth
    COMPANY_NAME "REPLACE_BEFORE_IMPLEMENTATION"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Chsy
    PLUGIN_CODE Chs1
    FORMATS Standalone VST3
    PRODUCT_NAME "ChordSynth"
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE)

target_sources(ChordSynth PRIVATE
    src/plugin/PluginProcessor.cpp
    src/plugin/PluginProcessor.h
    src/plugin/PluginEditor.cpp
    src/plugin/PluginEditor.h)

target_compile_definitions(ChordSynth PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(ChordSynth
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
```

**Steps:**

1. Configurar CMake.
2. Compilar processor/editor vacíos.
3. Verificar que existen los targets Standalone y VST3.
4. Abrir Standalone y comprobar que la ventana carga.
5. Commit: `build: create shared standalone and VST3 targets`.

**Estado (2026-08-20): completada.**
- Creado `CMakeLists.txt` y `CMakePresets.json` con soporte para C++20, Standalone y VST3 (`juce_add_plugin`).
- Creados `src/plugin/PluginProcessor.h/.cpp` y `src/plugin/PluginEditor.h/.cpp` con inicialización básica de buses estéreo y UI.
- Commit realizado: `4eed5cf` (`build: create shared standalone and VST3 targets`).

### Task 4: Integrar Catch2 y el primer test

**Objective:** Tener RED/GREEN automatizado antes de producir lógica.

**Files:**
- Create: `cmake/Dependencies.cmake`
- Create: `tests/CMakeLists.txt`
- Create: `tests/music/NoteMathTests.cpp`

**Steps:**

1. Fijar Catch2 `v3.15.3` con `FetchContent` o submodule.
2. Escribir `TEST_CASE("middle C has the expected frequency")` llamando a una API aún ausente.
3. Configurar y ejecutar `ctest`; verificar fallo por símbolo/comportamiento ausente, no por FetchContent.
4. Crear `src/music/NoteMath.h/.cpp` con conversión MIDI a Hz usando A4=440.
5. Verificar C4 aproximadamente 261.625565 Hz y A4 exactamente 440 Hz dentro de tolerancia.
6. Commit: `test: establish C++ test harness`.

**Estado (2026-08-20): completada.**
- Catch2 `v3.15.3` integrado mediante `FetchContent` en `cmake/Dependencies.cmake`.
- Ciclo TDD ejecutado:
  - RED: `NoteMathTests.cpp` con aserciones para A4 (440 Hz), C4 (~261.6256 Hz) y A3 (220 Hz); verificado fallo con stub retornando `0.0`.
  - GREEN: Implementado `chordsynth::music::midiToFrequency` en `src/music/NoteMath.cpp`.
  - Verificación: 100% tests pasaron en `ctest`.
- Commit realizado: `448f33c` (`test: establish C++ test harness`).

### Task 5: Implementar el mapa diatónico mayor

**Objective:** Convertir tonalidad + grado en la tríada correcta.

**Files:**
- Create: `src/music/Chord.h`
- Create: `src/music/MajorScaleChordMap.h`
- Create: `src/music/MajorScaleChordMap.cpp`
- Create: `tests/music/MajorScaleChordMapTests.cpp`

**RED cases, uno por ciclo:**

1. En C mayor, grados 1–7 producen roots C, D, E, F, G, A, B.
2. Las calidades son major, minor, minor, major, major, minor, diminished.
3. El grado V produce G-B-D.
4. C# mayor transpone todos los resultados un semitono.
5. Grados fuera de 0..6 se rechazan de manera explícita.
6. Las notas se mantienen dentro de MIDI 0..127.

**Implementation constants:**

```cpp
constexpr std::array scaleSemitones{0, 2, 4, 5, 7, 9, 11};
constexpr std::array qualities{
    ChordQuality::major,
    ChordQuality::minor,
    ChordQuality::minor,
    ChordQuality::major,
    ChordQuality::major,
    ChordQuality::minor,
    ChordQuality::diminished,
};
```

**Verification:** `ctest -R MajorScaleChordMap --output-on-failure` y luego suite completa.

**Commit:** `feat: map major keys to seven diatonic triads`.

### Task 6: Crear un oscilador sample-rate-independent

**Objective:** Renderizar seno estable sin NaN ni drift de fase.

**Files:**
- Create: `src/dsp/Oscillator.h`
- Create: `src/dsp/Oscillator.cpp`
- Create: `tests/dsp/OscillatorTests.cpp`

**RED cases:**

1. A4 a 48 kHz tiene cruces de cero consistentes con 440 Hz.
2. La fase queda normalizada después de muchos bloques.
3. La salida de seno permanece en [-1, 1].
4. Un reset produce secuencia determinista.
5. Cambiar sample rate conserva la frecuencia audible.

**GREEN:** Implementar acumulador de fase normalizada y seno; no agregar aún otras formas de onda.

**Commit:** `feat: add deterministic sine oscillator`.

### Task 7: Implementar voz con ADSR anti-click

**Objective:** Convertir note-on/note-off en audio con una envolvente segura.

**Files:**
- Create: `src/dsp/ChordSound.h`
- Create: `src/dsp/ChordVoice.h`
- Create: `src/dsp/ChordVoice.cpp`
- Create: `tests/dsp/ChordVoiceTests.cpp`

**RED cases:**

1. Note-on genera energía no nula.
2. Note-off decae a silencio.
3. La voz se libera después del release.
4. Cambiar velocity cambia amplitud.
5. Salida finita en sample rates y bloques de la matriz.

**GREEN:** Usar el oscilador seno y `juce::ADSR`; valores iniciales attack 5 ms, decay 80 ms, sustain 0.8 y release 120 ms.

**Commit:** `feat: render polyphonic synth voice with ADSR`.

### Task 8: Integrar 16 voces en PluginProcessor

**Objective:** Hacer que MIDI entrante produzca audio en Standalone y VST3.

**Files:**
- Modify: `src/plugin/PluginProcessor.h`
- Modify: `src/plugin/PluginProcessor.cpp`
- Create: `tests/plugin/ProcessorSmokeTests.cpp`

**RED cases:**

1. Un note-on MIDI produce energía en el bloque.
2. Una tríada MIDI produce audio sin invalidar muestras.
3. Note-off permite que el bloque vuelva a silencio.
4. Dieciséis notas simultáneas no crashean.
5. Diecisiete notas aplican una política de voice stealing determinista/aceptable.

**GREEN:** Preasignar 16 `ChordVoice` y un `ChordSound` en el constructor; preparar el synthesiser solo en `prepareToPlay`.

**Commit:** `feat: connect MIDI input to polyphonic processor`.

### Task 9: Crear cola acotada para eventos de la UI

**Objective:** Permitir que la message thread solicite notas sin bloquear el audio thread.

**Files:**
- Create: `src/dsp/UiMidiQueue.h`
- Create: `src/dsp/UiMidiQueue.cpp`
- Create: `tests/dsp/UiMidiQueueTests.cpp`
- Modify: `src/plugin/PluginProcessor.cpp`

**RED cases:**

1. Mantiene orden FIFO.
2. Drena eventos en el siguiente bloque.
3. Reporta overflow sin asignar ni bloquear.
4. Note-on y note-off sobreviven el cruce de threads.
5. El processor mezcla MIDI del host y UI en el mismo bloque.

**Verification adicional:** instrumentar una build de test con contador global de allocations y comprobar cero allocations durante una serie estable de `processBlock`.

**Commit:** `feat: add bounded UI-to-audio MIDI queue`.

### Task 10: Construir los siete pads

**Objective:** Completar la experiencia V0 de mantener un pad y oír una tríada.

**Files:**
- Create: `src/ui/ChordPad.h`
- Create: `src/ui/ChordPad.cpp`
- Create: `src/ui/ChordPadGrid.h`
- Create: `src/ui/ChordPadGrid.cpp`
- Modify: `src/plugin/PluginEditor.h`
- Modify: `src/plugin/PluginEditor.cpp`
- Create: `tests/plugin/ChordPadCommandTests.cpp`

**Behavior:**

- Mouse-down envía tres note-ons.
- Mouse-up, mouse-exit con captura perdida y destrucción del editor envían los note-offs correspondientes.
- Cada pad muestra número romano y nombre del acorde.
- C mayor muestra `I C`, `ii Dm`, `iii Em`, `IV F`, `V G`, `vi Am`, `vii° Bdim`.
- El pad muestra estado pressed visible.

**TDD:** Extraer primero un `ChordPadCommandBuilder` sin UI y probar los mensajes exactos; después conectar el componente JUCE.

**Manual verification:** Mantener pad V y oír G-B-D; soltar y comprobar release sin nota colgada.

**Commit:** `feat: add seven playable diatonic chord pads`.

### Task 11: Añadir selector de las 12 tonalidades

**Objective:** Remapear los siete pads manteniendo relaciones diatónicas.

**Files:**
- Create: `src/parameters/ParameterIds.h`
- Create: `src/parameters/ParameterLayout.h`
- Create: `src/parameters/ParameterLayout.cpp`
- Modify: `src/plugin/PluginProcessor.*`
- Modify: `src/plugin/PluginEditor.*`
- Create: `tests/parameters/ParameterLayoutTests.cpp`

**RED cases:**

1. Parámetro `key` tiene 12 opciones estables.
2. Cambiar C a D transpone C/Dm/... a D/Em/...
3. El estado del host restaura la tonalidad.
4. Los IDs y rangos no cambian entre instancias.

**GREEN:** Introducir `AudioProcessorValueTreeState` como fuente única de parámetros y estado.

**Commit:** `feat: add automatable key selection`.

### Task 12: Añadir ADSR automatizable y smoothing

**Objective:** Exponer attack, decay, sustain y release sin glitches.

**Files:**
- Modify: `src/parameters/ParameterLayout.cpp`
- Modify: `src/dsp/ChordVoice.*`
- Modify: `src/plugin/PluginProcessor.cpp`
- Modify: `src/plugin/PluginEditor.cpp`
- Extend: `tests/dsp/ChordVoiceTests.cpp`

**RED cases:** rangos válidos, valores default, restauración y cambio durante render sin NaN/clic severo.

**UI:** Cuatro controles con attachments de APVTS. Nunca escribir directamente parámetros desde el audio thread.

**Commit:** `feat: expose automatable ADSR controls`.

### Task 13: Añadir cuatro waveforms

**Objective:** Incorporar sine, saw, square y triangle con selección automatizable.

**Files:**
- Modify: `src/dsp/Oscillator.*`
- Modify: `src/parameters/ParameterLayout.cpp`
- Modify: `src/plugin/PluginEditor.cpp`
- Extend: `tests/dsp/OscillatorTests.cpp`

**RED cases:** muestra/periodicidad/rango para cada forma, cambio de waveform sin muestras inválidas y estado restaurable.

**Constraint:** En esta fase se aceptan formas naive para aprendizaje; crear un issue separado para band-limiting/aliasing antes de vender presets brillantes en registros altos.

**Commit:** `feat: add selectable classic waveforms`.

### Task 14: Añadir filtro low-pass

**Objective:** Agregar cutoff/resonance global con actualización realtime segura.

**Files:**
- Create: `src/dsp/Filter.h`
- Create: `src/dsp/Filter.cpp`
- Create: `tests/dsp/FilterTests.cpp`
- Modify: processor, parameter layout y editor.

**RED cases:**

1. Cutoff bajo atenúa una señal aguda.
2. Cutoff alto conserva más energía.
3. Valores extremos no producen NaN/Inf.
4. Funciona a 44.1/48/96 kHz.
5. Automatización rápida queda suavizada.

**GREEN:** Empezar con `juce::dsp::StateVariableTPTFilter<float>`.

**Commit:** `feat: add automatable low-pass filter`.

### Task 15: Añadir par estéreo detuned por nota

**Objective:** Crear anchura sin duplicar la lógica musical.

**Files:**
- Modify: `src/dsp/ChordVoice.*`
- Extend: `tests/dsp/ChordVoiceTests.cpp`
- Modify: parameter layout/editor.

**Behavior:**

- Oscillator A: pan left, detune negativo.
- Oscillator B: pan right, detune positivo.
- Detune 0 cents produce una señal centrada equivalente.
- Rango inicial: 0–20 cents; default 7.
- En mono, no debe desaparecer por cancelación de fase.

**RED cases:** energía en ambos canales, diferencia estéreo con detune > 0, correlación/mono razonable y salida acotada.

**Commit:** `feat: add stereo detuned oscillator pairs`.

### Task 16: Persistir estado y presets versionados

**Objective:** Restaurar sesiones del host y compartir presets sin acoplar el formato externo al ValueTree interno.

**Files:**
- Create: `src/presets/Preset.h`
- Create: `src/presets/PresetSerializer.h/.cpp`
- Create: `tests/presets/PresetSerializerTests.cpp`
- Modify: `PluginProcessor::getStateInformation/setStateInformation`.

**Schema inicial:**

```json
{
  "schema_version": 1,
  "name": "Warm C",
  "parameters": {
    "key": 0,
    "waveform": "saw",
    "attack_ms": 5.0,
    "decay_ms": 80.0,
    "sustain": 0.8,
    "release_ms": 120.0,
    "cutoff_hz": 8000.0,
    "resonance": 0.2,
    "detune_cents": 7.0,
    "master_gain_db": -12.0
  }
}
```

**RED cases:** round-trip, campos ausentes, valores fuera de rango, versión desconocida y JSON malformado. Parsing siempre fuera del audio thread.

**Commit:** `feat: add versioned preset and host-state persistence`.

### Task 17: Añadir render-safety y soak tests

**Objective:** Detectar fallos DSP que unit tests pequeños no capturan.

**Files:**
- Create: `tests/dsp/RenderSafetyTests.cpp`
- Create: `docs/realtime-safety.md`

**Cases:**

- Matriz de sample rates/block sizes.
- 60 segundos de render offline con note-ons/offs aleatorios y semilla fija.
- Automatización extrema de cada parámetro.
- All-notes-off.
- Recrear processor/editor repetidamente.
- Guardar/cargar estado durante ciclos de vida permitidos por el host.

**Assertions:** cero crash, cero NaN/Inf, pico <= límite documentado, notas terminan y test repetible.

**Commit:** `test: add deterministic DSP soak coverage`.

### Task 18: Validar Standalone V1

**Objective:** Probar el instrumento con dispositivo de audio real antes de entrar al DAW.

**Manual checklist:**

1. Seleccionar driver/dispositivo y 48 kHz.
2. Probar tamaño de buffer 64, 128, 256 y 512 si el hardware lo permite.
3. Tocar cada pad en C y en otra tonalidad.
4. Solapar al menos cuatro pads.
5. Probar teclado MIDI externo si está disponible.
6. Cambiar parámetros mientras suena un acorde.
7. Cerrar/reabrir y verificar estado/preset.
8. Confirmar ausencia de notas colgadas y audio corrupto.
9. Registrar versión de Windows, driver, interfaz y latencia observada.

**Artifact:** `docs/validation/standalone-v1.md` con resultados reales.

**Commit:** `test: document standalone V1 validation`.

### Task 19: Validar VST3 con pluginval

**Objective:** Detectar problemas de ciclo de vida, buses, estado y realtime antes de FL Studio.

**Files:**
- Create: `tools/validate-plugin.ps1`
- Create: `docs/validation/pluginval-v1.md`

**Command base:**

```powershell
pluginval.exe --strictness-level 5 --validate "C:\path\to\ChordSynth.vst3"
```

**Steps:**

1. Compilar Release limpio.
2. Ejecutar pluginval.
3. Guardar versión, comando, exit code y resumen real.
4. Si falla, reproducir con una prueba automatizada antes de corregir.
5. Repetir hasta exit code 0.

**Commit:** `test: validate release VST3 with pluginval`.

### Task 20: Validar dentro de FL Studio

**Objective:** Demostrar el flujo de usuario final en el host objetivo.

**Checklist:**

1. Instalar/copiar `ChordSynth.vst3` en una ruta escaneada por FL Studio.
2. Re-scan plugins y cargarlo como generator/instrument.
3. Abrir/cerrar la UI sin crash.
4. Tocar desde Piano Roll y controlador MIDI.
5. Tocar los siete pads de la UI.
6. Automatizar cutoff, attack, release y detune.
7. Guardar el proyecto FLP, cerrar FL Studio, reabrir y verificar estado.
8. Cambiar sample rate/buffer desde el host.
9. Renderizar/exportar audio y confirmar que coincide musicalmente con playback.
10. Probar stop/play, panic/all notes off y eliminación/reinserción del plugin.

**Artifact:** `docs/validation/fl-studio-v1.md`, incluyendo versión exacta de FL Studio, Windows, ruta VST3 y resultados reales.

**Exit V1:** los diez puntos pasan sin notas colgadas, pérdida de estado ni crash.

**Commit:** `test: document FL Studio VST3 validation`.

### Task 21: Añadir chorus verticalmente

**Objective:** Primer efecto post-synth con mix automatizable.

**Files:** `src/dsp/Chorus.*`, prueba DSP, parámetros y editor.

**TDD:** mix 0 es bypass perceptual; mix > 0 cambia estéreo; cambios no generan NaN; sample-rate reset correcto.

**GREEN:** `juce::dsp::Chorus<float>` primero; implementación propia solo si existe una razón sonora medible.

**Commit:** `feat: add stereo chorus effect`.

### Task 22: Añadir delay sincronizable

**Objective:** Echo estable y opcionalmente sincronizado al tempo del host.

**Files:** `src/dsp/TempoDelay.*`, tests, parámetros y editor.

**TDD:** tiempo libre, 1/4/1/8/1/16 con BPM conocido, feedback acotado, cambio de BPM sin invalidar memoria, bypass.

**Realtime:** preasignar delay line para el máximo soportado en `prepare`.

**Commit:** `feat: add tempo-aware delay`.

### Task 23: Añadir reverb

**Objective:** Completar la cadena V1.1 de ambiente.

**Files:** `src/dsp/Reverb.*`, tests, parámetros y editor.

**TDD:** wet 0 es bypass; impulso produce cola; cola decae; reset limpia estado; salida finita.

**Commit:** `feat: add reverb stage`.

### Task 24: Añadir clock musical y arpegiador

**Objective:** Separar scheduling musical del render de voces.

**Files:**
- Create: `src/music/MusicalClock.*`
- Create: `src/music/Arpeggiator.*`
- Create: `tests/music/MusicalClockTests.cpp`
- Create: `tests/music/ArpeggiatorTests.cpp`

**RED cases:**

1. Up ordena grave a agudo.
2. Down ordena agudo a grave.
3. Up/Down no duplica extremos accidentalmente.
4. Random es determinista con semilla inyectada en tests.
5. 1/8 a 120 BPM produce offsets de sample correctos.
6. Cambiar sample rate/BPM conserva fase musical razonable.
7. Si el host no da tempo, se usa BPM interno documentado.

**Integration:** El arpegiador emite MIDI interno; no renderiza audio ni conoce la UI.

**Commit:** `feat: add host-synchronised arpeggiator`.

### Task 25: Automatizar build y tests en Windows

**Objective:** Evitar que el proyecto dependa de una sola máquina.

**Files:**
- Create: `.github/workflows/windows-build.yml`

**Pipeline:**

1. Checkout con submodules.
2. Configure Debug.
3. Build.
4. CTest.
5. Configure/build Release.
6. Subir Standalone y VST3 como artifacts.
7. Ejecutar pluginval si su distribución/licencia y modo headless lo permiten; de lo contrario mantenerlo como gate local documentado.

**Commit:** `ci: build and test Standalone and VST3 on Windows`.

### Task 26: Preparar una release reproducible

**Objective:** Entregar binarios verificables y documentación suficiente.

**Files:**
- Modify: `README.md`
- Create: `CHANGELOG.md`
- Create: `docs/build-windows.md`
- Create: `docs/user-guide.md`

**Release checklist:**

- Working tree limpio.
- Suite verde.
- pluginval verde.
- FL Studio checklist verde.
- Versiones y IDs consistentes.
- Licencias y avisos incluidos.
- Artefactos Release, no Debug.
- SHA-256 calculado para cada artefacto.
- Tag firmado/anotado si la configuración Git lo permite.

**Commit:** `docs: prepare first validated release`.

## 8. Gates por hito

### Gate V0

- [ ] Siete acordes correctos en C mayor.
- [ ] Standalone abre y reproduce audio.
- [ ] Note-off no deja notas colgadas.
- [ ] Tests de teoría, oscilador, voz y processor verdes.
- [ ] Cero NaN/Inf en render offline.

### Gate V1

- [ ] Standalone y VST3 provienen del mismo target shared code.
- [ ] Cuatro waveforms, ADSR, filtro, detune y master gain funcionan.
- [ ] 12 tonalidades.
- [ ] Estado del host y presets hacen round-trip.
- [ ] pluginval nivel 5 pasa.
- [ ] FL Studio carga, automatiza, guarda, restaura y exporta.
- [ ] No hay I/O, locks ni allocations conocidas en audio thread estable.

### Gate V1.1

- [ ] Chorus, delay y reverb tienen bypass y tests de estabilidad.
- [ ] Arpegiador mantiene timing por sample offsets.
- [ ] Delay/arp siguen BPM del host y tienen fallback explícito.

## 9. Riesgos y mitigaciones

| Riesgo | Mitigación |
|---|---|
| Aprender C++, JUCE, DSP y VST3 simultáneamente | Entregas verticales muy pequeñas; V0 solo seno + tríada + Standalone. |
| Bugs difíciles dentro de FL Studio | Processor testeable offline + pluginval antes del DAW. |
| Audio thread bloqueado o con allocations | Cola acotada, preallocation, soak tests y documento realtime-safety. |
| Aliasing en saw/square | Aceptarlo solo en aprendizaje V1; planear PolyBLEP/band-limited antes de release sonora seria. |
| Cambios de IDs rompen sesiones | Fijar IDs VST3 y APVTS antes de distribuir; nunca reutilizar un ID con otro significado. |
| JUCE/VST3 licensing mal interpretado | Gate legal en Task 1; revisar JUCE y VST3 por separado. |
| Linux remoto no representa Windows/FL Studio | CI Windows + validación manual en Windows; no afirmar compatibilidad solo por compilar en Linux. |
| UI domina el proyecto antes de sonar bien | UI V0 deliberadamente simple; priorizar music/DSP y feedback funcional. |
| Clicks/zipper noise | ADSR desde V0 y smoothing para parámetros continuos. |
| Scope creep hacia clon completo | Looper, vocoder, sampler y drums quedan explícitamente fuera hasta cerrar V1.1. |

## 10. Aprendizaje recomendado por laboratorio

Cada laboratorio debe cerrar con una prueba, una visualización o un render verificable:

1. **Pitch:** MIDI -> Hz; calcular A4 y C4 a mano y verificar en test.
2. **Sampling:** fase discreta; renderizar 440 Hz a 48 kHz.
3. **Polifonía:** sumar tres notas y observar headroom/clipping.
4. **ADSR:** graficar o exportar la envolvente de una nota.
5. **Filtro:** comparar energía antes/después de cutoff.
6. **Estéreo:** medir L/R y mono compatibility del detune.
7. **Realtime:** provocar una allocation en un spike desechable, medirla y luego eliminarla.
8. **Plugin host:** observar lifecycle prepare/process/release y restauración de estado.
9. **Timing:** convertir BPM/división musical a samples.
10. **Voice leading:** formular distancia entre voicings y minimizarla antes de implementar UI.

Los spikes de aprendizaje no se convierten en producción directamente: se desechan y la funcionalidad real se reimplementa con RED-GREEN-REFACTOR.

## 11. Criterio de “terminado”

Una tarea no está terminada solo porque compila. Debe tener:

- Prueba que se observó fallar primero por el motivo esperado.
- Implementación mínima.
- Prueba específica verde.
- Suite completa verde.
- Verificación manual cuando cruza audio device, VST3 o FL Studio.
- Documentación actualizada.
- Commit pequeño con paths relevantes solamente.
- Revisión independiente de cumplimiento y después de calidad.

## 12. Primer corte recomendado

Ejecutar únicamente Tasks 1–10 para obtener V0. No empezar filter, presets, VST3 host validation ni efectos hasta que se pueda:

```text
abrir Standalone
-> mantener pad 5 en C
-> escuchar G + B + D
-> soltar
-> oír un release limpio
-> cerrar sin crash
```

Ese recorrido es el tracer bullet que demuestra la arquitectura completa: UI -> Music Engine -> MIDI interno -> Voice Allocator -> Oscillator -> ADSR -> audio device.

## 13. Referencias verificadas al redactar el plan

- HiChord Manual, rev. 2.8: <https://hichord.shop/pages/manual#quickstart>
- JUCE releases, versión fijada 9.0.1: <https://github.com/juce-framework/JUCE/releases/tag/9.0.1>
- Ejemplo CMake oficial JUCE AudioPlugin: <https://github.com/juce-framework/JUCE/tree/master/examples/CMake/AudioPlugin>
- Catch2 v3.15.3: <https://github.com/catchorg/Catch2/releases/tag/v3.15.3>
- pluginval v1.0.4: <https://github.com/Tracktion/pluginval/releases/tag/v1.0.4>

Antes de implementar se deben consultar además, en sus fuentes oficiales vigentes: licencia JUCE, licencia VST3 SDK, documentación de `juce_add_plugin`, `AudioProcessorValueTreeState`, `juce::Synthesiser`, `juce::dsp` y las instrucciones actuales de escaneo VST3 de FL Studio.
