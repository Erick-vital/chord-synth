# Changelog

Todas las notas notables de cambios para este proyecto están documentadas en este archivo.
El formato está basado en [Keep a Changelog](https://keepachangelog.com/es-ES/1.0.0/) y este proyecto sigue [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

### Añadido
- Sistema de armonía Lo‑Fi/Jazz con recetas de tríada, 7, 9, 11, 13, add9, 6/9, sus2 y sus4.
- Políticas de voicing compacto, abierto y rootless; omisión de quinta, bajo raíz/slash y voice leading nearest determinista.
- Escenas de fábrica A Diatónica, B Séptimas, C Lo‑Fi Warm y D Jazz Tension.
- Transformaciones temporales de color en las paletas Básica, Lo‑Fi y Spice, con persistencia explícita por grado.
- Mapeo MIDI opcional: notas 36–42 para grados y CC 20–27 para colores; bajo separado en canal interno 2.
- Parámetros APVTS para MIDI Performance y paleta de transformaciones.
- Presets Default (Init), Warm Saw Chords, Ambient Open Keys, Arp Plucks y Jazz Tension alineados con las escenas musicales.

### Cambiado
- HarmonyState se serializa como versión 2 y migra datos v1 conservando los defaults legados.
- Los presets JSON escriben schema 3 y mantienen carga compatible con schemas 1 y 2.
- La carga de presets libera el acorde activo antes de sincronizar estado, interfaz y pads para evitar notas colgadas.

### No incluido todavía
- Tensiones alteradas `b9`, `#9`, `#11`, `b13` y `alt`.
- MIDI Learn o base de nota MIDI configurable para el mapeo de grados.

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
