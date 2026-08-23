# ChordSynth

ChordSynth es un instrumento de acordes polifónico para explorar, tocar y producir progresiones con rapidez. Está construido en C++20 con JUCE y comparte el mismo motor musical entre la aplicación Standalone y el plugin VST3.

En lugar de construir cada acorde nota por nota, ChordSynth presenta siete grados diatónicos para la tonalidad y escala elegidas. Puedes tocar una progresión de inmediato, seleccionar una escena musical o diseñar el voicing de cada grado.

## Funciones principales

- **Interpretación por grados:** siete pads I–vii°; en C mayor: C, Dm, Em, F, G, Am y Bdim.
- **Tonalidad y escala:** doce tonalidades, Mayor y Menor natural.
- **Recetas de acordes:** tríada, 7, 9, 11, 13, add9, 6/9, sus2 y sus4; `add9`, `maj9` y dominante 9 tienen identidades diferentes.
- **Voicings musicales:** compacto, abierto y rootless, con política de quinta, bajo raíz/slash, registro seguro y nearest voice leading determinista.
- **Cuatro escenas:** A Diatónica, B Séptimas, C Lo‑Fi Warm y D Jazz Tension; sus defaults son datos por grado y se adaptan a la escala.
- **Chord Color:** tres paletas de transformaciones temporales (Básica, Lo‑Fi y Spice), que pueden fijarse explícitamente en el grado activo.
- **MIDI Performance opcional:** notas MIDI 36–42 disparan grados; CC 20–27 controlan colores. El MIDI no mapeado conserva sus eventos originales.
- **Bajo y arpegiador separados:** armonía en canal interno 1 y bajo en canal 2; el arpegiador no arpegia el bajo.
- **Síntesis polifónica:** 16 voces estéreo, envolventes anti-click, osciladores Sine/Saw/Square/Triangle, detune y filtro low-pass resonante.
- **Arpegiador y efectos:** Up/Down/Up-Down/Random, chorus, delay libre o sincronizado y reverb.
- **Presets y estado persistente:** HarmonyState v2 y presets JSON schema 4 con migración compatible de versiones anteriores.
- **Standalone y VST3:** un mismo motor en ambos formatos.

## Uso rápido

1. Abre ChordSynth Standalone o como instrumento VST3 en tu DAW.
2. Selecciona una **Tonalidad** y una **Escala**; por ejemplo `C` y `Mayor`.
3. Toca los pads o el teclado:

   ```text
   Q   W   E   R   T   Y   U
   I   ii  iii IV  V   vi  vii°
   ```

   Mantén la tecla para sostener el acorde; suéltala para liberar las mismas notas.
4. Cambia de escena con `1`–`4`:

   - A: Diatónica, tríadas compactas.
   - B: Séptimas diatónicas compactas.
   - C: Lo‑Fi Warm, extensiones abiertas con bajo raíz.
   - D: Jazz Tension, extensiones rootless con bajo raíz.

5. Mantén `A S D F G H J K` para aplicar los ocho colores de la paleta seleccionada. **Fijar en grado** persiste la transformación; soltar el color restaura la base si no la fijaste.
6. Selecciona un grado para abrir **Diseñar acorde** y editar forma, calidad, voicing, quinta, bajo, voz guía, inversión y registro.
7. En **Sonido y movimiento**, ajusta forma de onda, detune, filtro, arpegiador y efectos.
8. Selecciona un preset. La carga libera el acorde activo y sincroniza la escena, los pads y el diseñador.

### Re-voicing de acordes sostenidos

Por defecto una escena afecta al próximo acorde. Activa **Re-voicing del acorde sostenido** para reemplazar diferencialmente las voces de un acorde que ya mantienes pulsado, conservando las notas comunes cuando sea posible.

### MIDI Performance

Activa **MIDI Perf** solo si quieres el mapeo semántico:

- Notas 36–42: I–VII.
- CC 20–27: colores 1–8 (`>=64` pulsa, `<64` libera).
- Eventos MIDI no mapeados: pasan sin cambios.

Los tonos armónicos usan canal interno 1; el bajo usa canal 2 y evita el arpegiador. Esto permite arpegiar el acorde sin convertir el bajo en arpegio.

## Presets incluidos

- **Default (Init):** escena A Diatónica.
- **Warm Saw Chords:** escena C Lo‑Fi Warm.
- **Ambient Open Keys:** escena C con re-voicing activo.
- **Arp Plucks:** escena B con arpegiador activo.
- **Jazz Tension:** escena D con re-voicing activo.

## Instalación y compilación en Windows

El objetivo de release es Windows 10/11 x64 y la validación principal se realiza en FL Studio. Necesitas Visual Studio 2022 con herramientas C++20, CMake 3.22+ y los submódulos.

```powershell
git clone --recurse-submodules https://github.com/Erick-vital/chord-synth.git
cd chord-synth
cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release --config Release --parallel
ctest --test-dir out/build/windows-msvc-release -C Release --output-on-failure
```

Artefactos esperados:

- Standalone: `out/build/windows-msvc-release/ChordSynth_artefacts/Release/Standalone/ChordSynth.exe`
- VST3: `out/build/windows-msvc-release/ChordSynth_artefacts/Release/VST3/ChordSynth.vst3`

Copia el VST3 a `C:\Program Files\Common Files\VST3\` y vuelve a escanear plugins en el DAW. Consulta [la guía Windows](docs/build-windows.md) para más detalle.

## Validación GUI en Linux

Linux se usa como gate de compilación y smoke del artefacto, no como sustituto de la validación manual en Windows/FL Studio. Para compilar los formatos GUI se requieren las cabeceras X11/GTK además de ALSA, OpenGL y fuentes:

```bash
sudo apt-get update && sudo apt-get install -y \
  pkg-config libasound2-dev libcurl4-openssl-dev libfontconfig1-dev \
  libfreetype6-dev libgl1-mesa-dev libjack-jackd2-dev libx11-dev \
  libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev \
  libxi-dev libgtk-3-dev
cmake --preset linux-ninja-plugin-debug
cmake --build --preset linux-plugin-debug --target ChordSynth_Standalone ChordSynth_VST3 ChordSynthGuiSmokeTests --parallel
ctest --test-dir build/linux-ninja-plugin-debug --output-on-failure
```

Los artefactos Debug quedan bajo `build/linux-ninja-plugin-debug/ChordSynth_artefacts/Debug/`. Si no hay una sesión gráfica, ejecuta el smoke GUI con `xvfb-run -a`; este smoke no valida audio/MIDI físico.

## Arquitectura

```text
UI: pads, diseñador, colores y entrada MIDI
                  |
                  v
       cola MIDI lock-free / parámetros APVTS
                  |
                  v
motor musical: receta -> voicing -> bajo/voice leading
                  |
                  v
arpegiador (canal 1) + bajo directo (canal 2)
                  |
                  v
motor de síntesis: 16 voces estéreo
                  |
                  v
DSP: filtro -> chorus -> delay -> reverb -> ganancia
                  |
                  v
     audio Standalone o host VST3
```

La interfaz no genera audio. Las rutas de receta, voicing, diferencias MIDI y mapeo están diseñadas para usar capacidad fija y evitar locks, I/O y parsing en `processBlock`. La ausencia total de asignaciones en cada ruta de rendimiento sigue siendo un objetivo de endurecimiento: el voicer aún construye etiquetas y algunos búferes MIDI pueden crecer en la implementación actual.

## Calidad y validación

La suite cubre teoría musical, recetas, voicings, bajo, voice leading, transformaciones, mapeo MIDI, estado, presets, automatización y render-safety. Incluye render prolongado, sample rates y bloques distintos, parámetros extremos, restauración de estado y contratos de sintaxis UI.

La validación Linux no sustituye la ejecución real de Standalone/VST3 en Windows. Antes de una entrega, compila y prueba ambos formatos y realiza smoke manual de escenas, colores, pérdida de foco, MIDI, bajo con arpegiador, presets y restauración de proyecto.

## Documentación

- [Guía de uso detallada](docs/user-guide.md)
- [Diseño Lo‑Fi/Jazz](docs/design/lofi-jazz-harmony.md)
- [Guía de compilación en Windows](docs/build-windows.md)
- [Seguridad en tiempo real](docs/realtime-safety.md)
- [Diseño de la interfaz](docs/design/chord-synth-interface.md)
- [Validación en FL Studio](docs/validation/fl-studio-v1.md)
- [Validación con pluginval](docs/validation/pluginval-v1.md)
- [Notas de versiones](CHANGELOG.md)

## Licencia

El código fuente de ChordSynth está bajo licencia MIT. JUCE se distribuye bajo sus propios términos de licencia (GPLv3 para distribución abierta o licencia comercial para distribución cerrada). El VST3 SDK de Steinberg se distribuye bajo licencia MIT.
