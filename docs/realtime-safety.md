# Seguridad en tiempo real y verificación

## Invariantes de la ruta de audio

`processBlock` y `renderNextBlock` deben terminar de forma determinista dentro del presupuesto del host. La política del proyecto prohíbe introducir en la ruta de audio:

- Asignar/liberar memoria (`new`, `delete`, `malloc`, crecimiento de `std::vector`, `juce::Array::add`, concatenación de strings).
- Locks, mutexes, esperas o llamadas bloqueantes.
- I/O, archivos, red, parsing o logging.
- Construir etiquetas o mensajes para la interfaz.

Las colecciones de voces, buffers, FIFOs y estado DSP deberían preasignarse en construcción o `prepareToPlay`. La implementación actual todavía tiene rutas a endurecer: al procesar MIDI Performance el voicer puede construir etiquetas y algunos `juce::MidiBuffer` pueden crecer al insertar eventos. `processBlock` inicia `juce::ScopedNoDenormals`, limpia canales de salida no usados y debe producir muestras finitas.

## Parámetros y estado

APVTS ofrece parámetros host-automatizables estables. La ruta de audio obtiene punteros atómicos una vez y los lee con `memory_order_relaxed`. Los parámetros continuos se suavizan o sanitizan antes de afectar al DSP. JSON/XML, migraciones y persistencia se realizan fuera de la ruta de audio.

HarmonyState actual es v2 y Preset JSON actual es schema v3. La migración acepta estados/presets legados compatibles y aplica defaults explícitos; no ocurre parsing en `processBlock`.

## Rendimiento armónico y MIDI

La generación de recetas, voicings, transformaciones, diferencias MIDI y mapeo MIDI persigue límites estrictos:

- Como máximo seis tonos armónicos más un bajo opcional.
- Arreglos fijos para candidatos, diferencias de re-voicing y lotes de eventos; una sustitución completa admite hasta catorce eventos.
- No deben añadirse `std::vector`, locks ni asignaciones a voicing, transformaciones o `MidiPerformanceMapper`.
- El mapper conserva un único acorde/grado mapeado activo y procesa note-off repetidos y CC 120/123 para prevenir notas colgadas. Las pulsaciones superpuestas no tienen todavía seguimiento de conteo independiente.

La interfaz comunica eventos por `UiMidiQueue`, una FIFO lock-free. El bajo generado se marca en canal interno 2 y se mezcla directamente; las voces armónicas generadas usan canal 1. Con el arpegiador activo el canal 2 evita el arpegiador; el MIDI externo de otros canales sigue la ruta normal del arpegiador.

La carga de presets y los controles UI se ejecutan en el hilo de mensajes. Antes de modificar tonalidad, escala, escena o HarmonyState, la interfaz libera cualquier acorde activo. Así mantiene el ciclo de vida de notas correcto sin introducir locks en la ruta de audio.

## Pruebas automatizadas

`tests/dsp/RenderSafetyTests.cpp` cubre:

1. Matriz de sample rates 44.1, 48 y 96 kHz, con bloques de 1, 16, 64, 256, 512 y 1024 muestras.
2. Soak render determinista de 60 segundos a 48 kHz / 256 muestras con automatización y eventos polifónicos de semilla fija.
3. Valores extremos, negativos, no finitos y fuera de rango para parámetros automatizados.
4. All-notes-off, decaimiento ADSR y defensa contra voces colgadas.
5. Creación, preparación, render y destrucción repetidas del procesador; guardar/restaurar estado durante el ciclo del host.

La suite completa también cubre `MidiPerformanceMapper` y el enrutamiento de procesador que mantiene el bajo generado en canal 2 fuera del arpegiador.

La prueba finita/no-crash por sí sola no es suficiente: los tests de automatización comparan procesadores deterministas alineados, los filtros se validan por energía tras transitorios y las rutas de enum inválidas deben comprobar el fallback documentado.

## Gates locales y de Windows

En Linux se deben ejecutar el build, los tests por tags pertinentes (por ejemplo `[music]`, `[interaction]`, `[state]`, `[presets]` y `[plugin]`), CTest completo, `git diff --check` y contratos de sintaxis UI. Los contratos de fuente/sintaxis no prueban el renderizado real de GUI ni la integración VST3.

Antes de declarar la versión lista para entrega, en Windows se requiere:

```powershell
cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release --config Release --parallel
ctest --test-dir out/build/windows-msvc-release -C Release --output-on-failure
```

Después se realiza smoke manual de Standalone: escenas, todas las paletas, liberación al perder foco, teclas, MIDI note/CC, arpegiador con bajo separado, presets, proyecto guardado y ausencia de notas colgadas. El VST3 se valida adicionalmente en FL Studio/pluginval mediante las checklists del repositorio.
