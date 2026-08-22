# ChordSynth

ChordSynth es un instrumento de acordes polifónico para explorar, tocar y producir progresiones armónicas con rapidez. Está construido en C++20 con JUCE y comparte el mismo motor entre la aplicación Standalone y el plugin VST3.

En lugar de obligarte a construir cada acorde nota por nota, ChordSynth presenta siete grados diatónicos para la tonalidad y escala seleccionadas. Puedes empezar tocando una progresión inmediatamente y, cuando lo necesites, modificar el voicing de cada grado.

## Funciones principales

- **Interpretación por grados diatónicos:** siete teclas para los grados `I` a `vii°`. En C mayor corresponden a `C`, `Dm`, `Em`, `F`, `G`, `Am` y `Bdim`.
- **Tonalidad y escala:** selecciona cualquiera de las 12 tonalidades y trabaja en escala mayor o menor natural.
- **Cuatro escenas de voicing:** alterna entre tríadas, séptimas, voicings abiertos e inversiones sin cambiar el contexto armónico.
- **Diseñador de acordes por grado:** ajusta extensión (tríada o séptima), inversión, distribución cerrada o abierta y registro base. El modo Diatónico respeta la escala; el modo Libre permite personalizar la calidad del grado.
- **Síntesis polifónica:** 16 voces estéreo con envolventes anti-click, osciladores Sine, Saw, Square y Triangle, detune estéreo y filtro low-pass resonante.
- **Arpegiador sincronizado:** modos Up, Down, Up/Down y Random, con rate musical y control de gate.
- **Efectos integrados:** chorus estéreo, delay libre o sincronizado al tempo del host y reverb.
- **Presets y estado persistente:** los parámetros y la configuración armónica se guardan en un estado versionado y compatible con proyectos anteriores.
- **Dos formatos:** aplicación Standalone para tocar directamente y VST3 para usar dentro de un DAW compatible, como FL Studio.

## Uso rápido

1. Abre ChordSynth como aplicación Standalone o cárgalo como instrumento VST3 en tu DAW.
2. En la barra de armonía, elige una **Tonalidad** y una **Escala**. Por ejemplo, `C` y `Mayor`.
3. Toca los siete pads de acordes con el ratón o el teclado del ordenador:

   ```text
   Q   W   E   R   T   Y   U
   I   ii  iii IV  V   vi  vii°
   ```

   Mantén pulsada una tecla para sostener el acorde y suéltala para liberar las notas.
4. Cambia de escena con las teclas `1` a `4` o con los botones `A` a `D`:

   - `A`: tríadas en posición raíz.
   - `B`: acordes de séptima diatónicos.
   - `C`: voicings abiertos.
   - `D`: inversiones para facilitar el voice leading.

5. Selecciona un grado para abrir **Diseñar acorde**. Ahí puedes cambiar extensión, inversión, distribución y registro. Guarda el cambio para aplicarlo a ese grado.
6. En **Sonido y movimiento**, elige la forma de onda, ajusta detune y filtro, activa el arpegiador y añade chorus, delay o reverb.
7. Usa la sección **Preset** para seleccionar una configuración disponible. En Standalone, abre **Audio / MIDI** para elegir el dispositivo de audio y las entradas MIDI.

### Re-voicing de acordes sostenidos

De forma predeterminada, cambiar una escena modifica solo el siguiente acorde para evitar cortes inesperados. Activa **Re-voicing del acorde sostenido** si quieres que el acorde que ya mantienes pulsado se actualice al cambiar de escena; ChordSynth conserva las notas comunes y actualiza únicamente las necesarias.

## Ejemplo: progresión rápida

Para tocar `I–V–vi–IV` en C mayor:

1. Selecciona `C` y `Mayor`.
2. Pulsa `Q`, `T`, `Y`, `R`.
3. Prueba la escena `B` para convertir la progresión en séptimas, o `C` para abrir los acordes.
4. Activa el arpegiador, selecciona `1/8` y ajusta el gate para convertir los acordes sostenidos en un patrón rítmico.

## Instalación y compilación en Windows

El objetivo de release de la primera versión es Windows 10/11 x64, con validación principal en FL Studio. Para compilar necesitas Visual Studio 2022 con herramientas C++20, CMake 3.22+ y los submódulos del repositorio.

```powershell
git clone --recurse-submodules https://github.com/Erick-vital/chord-synth.git
cd chord-synth
cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release --config Release --parallel
```

Tras compilar, los artefactos se generan en:

- Standalone: `out/build/windows-msvc-release/ChordSynth_artefacts/Release/Standalone/ChordSynth.exe`
- VST3: `out/build/windows-msvc-release/ChordSynth_artefacts/Release/VST3/ChordSynth.vst3`

Copia el VST3 en `C:\Program Files\Common Files\VST3\` y ejecuta el escaneo de plugins de tu DAW. Consulta la [guía de compilación en Windows](docs/build-windows.md) para los pasos completos.

## Arquitectura

```text
UI: pads diatónicos, controles y entrada MIDI
                  |
                  v
       cola de comandos MIDI sin locks
                  |
                  v
motor musical: tonalidad, escala, grados y voicings
                  |
                  v
motor de síntesis: 16 voces estéreo y polifonía
                  |
                  v
 DSP: filtro -> chorus -> delay -> reverb -> ganancia
                  |
                  v
     audio Standalone o host VST3
```

La UI no genera audio directamente. El motor musical, la síntesis y el DSP se comparten entre Standalone y VST3 para mantener el comportamiento consistente en ambos formatos.

## Calidad y validación

El proyecto incluye pruebas automatizadas para teoría musical, voicings, arpegiador, síntesis, automatización de parámetros, serialización de estado y seguridad de render. También cubre render prolongado, distintos sample rates, tamaños de bloque, valores extremos y restauración de estado.

```powershell
ctest --test-dir out/build/windows-msvc-release -C Release --output-on-failure
```

## Documentación

- [Guía de uso detallada](docs/user-guide.md)
- [Guía de compilación en Windows](docs/build-windows.md)
- [Diseño de la interfaz de interpretación](docs/design/chord-synth-interface.md)
- [Seguridad en tiempo real](docs/realtime-safety.md)
- [Validación en FL Studio](docs/validation/fl-studio-v1.md)
- [Validación con pluginval](docs/validation/pluginval-v1.md)
- [Notas de versiones](CHANGELOG.md)

## Licencia

El código fuente de ChordSynth está bajo licencia MIT. JUCE se distribuye bajo sus propios términos de licencia (GPLv3 para distribución abierta o licencia comercial para distribución cerrada). El VST3 SDK de Steinberg se distribuye bajo licencia MIT.
