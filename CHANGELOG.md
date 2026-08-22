# Changelog

Todas las notas notables de cambios para este proyecto están documentadas en este archivo.
El formato está basado en [Keep a Changelog](https://keepachangelog.com/es-ES/1.0.0/) y este proyecto sigue [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
  - Suite de 66 pruebas automatizadas con Catch2 (100% GREEN).
  - Pruebas de seguridad offline, matrix de sample rates y soak testing.
  - GitHub Actions Workflow para build y test en Windows x64.
