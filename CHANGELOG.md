# Changelog

Todas las notas notables de cambios para este proyecto están documentadas en este archivo.
El formato está basado en [Keep a Changelog](https://keepachangelog.com/es-ES/1.0.0/) y este proyecto sigue [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

### Añadido
- Refactorización de armonía a configuración única de siete grados (`std::array<VoicingSpec, 7>`) por preset/estado, eliminando el selector de escenas en tiempo de ejecución.
- Constructores de fábrica explícitos para presets: `makeDiatonic`, `makeSevenths`, `makeLofiWarm` y `makeJazzTension`.
- Migración automática en carga de estados legados v1/v2 (extrayendo su escena seleccionada a la configuración única) y presets JSON schemas 1–4.
- Persistencia de HarmonyState versión 3 y presets JSON schema 5 sin campos de escenas legadas (`selectedScene`, `Scenes`, `selected_scene`, `scenes`).

### Cambiado
- El panel `ChordDesignerPanel` edita exclusivamente la especificación del grado seleccionado sin selectores de escena.
- La confirmación visual de fijar transformaciones de color reporta el grado afectado (`Fijado en grado <I..VII>`).
- Se eliminó la tira de botones de escenas A–D y los atajos de teclado `1`–`4` de la interfaz de usuario, recuperando espacio vertical para los pads de interpretación.
- Presets Default (Init), Warm Saw Chords, Ambient Open Keys, Arp Plucks y Jazz Tension poseen sus propias configuraciones de fábrica por grado en lugar de números de escena.

---

---

## [0.1.0] - 2026-08-20

### Añadido
- **Motor de Teoría Musical Diatónica:**
  - Mapeo de grados mayores a tríadas diatónicas (`MajorScaleChordMap`).
  - Conversión de notas y frecuencias estándar (`NoteMath`).
  - Reloj musical con divisiones rítmicas de tempo (`MusicalClock`).
  - Arpegiador polifónico sincronizado con modos Up, Down, Up/Down y Random (`Arpeggiator`).
- **Motor de Síntesis Polifónica:**
  - 16 voces de polifonía estéreo con anti-click envelope (`ChordVoice`).
  - Formas de onda: Sine, Saw, Square y Triangle (`Oscillator`).
  - Detune estéreo por voz.
  - Filtro low-pass global resonante con suavizado de 20 ms (`Filter`).
- **Cadena de Efectos DSP V1.1 (Realtime-Safe):**
  - Stereo Chorus (`Chorus`).
  - Tempo-aware Delay con sincronización rítmica y modo libre (`TempoDelay`).
  - Reverb de estudio (`Reverb`).
- **Persistencia y Estado:**
  - APVTS con parámetros estables, versionados e inmutables.
  - Serialización y deserialización JSON (`PresetSerializer`).
  - Migración retrocompatible automática para estados legados del host.
- **Validación y CI:**
  - Suite de pruebas automatizadas con Catch2.
  - Pruebas de seguridad offline, matrix de sample rates y soak testing.
  - GitHub Actions Workflow para build y test en Windows x64.
