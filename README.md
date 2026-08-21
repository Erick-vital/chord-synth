# ChordSynth

Sintetizador e instrumento de acordes polifónico construido en C++20 con JUCE, diseñado como aplicación Standalone y plugin VST3 para FL Studio a partir de una única base de código y motor DSP compartido.

Inspirado en la filosofía armónica diatónica y flujo de interpretación de HiChord (7 pads diatónicos, transformaciones modales y flujo expresivo sin notas erróneas), implementando una arquitectura modular e independiente.

---

## Identidad del Producto

- **Nombre del Producto:** ChordSynth
- **Target Formats:** Standalone (App de escritorio) & VST3 (Plugin de audio)
- **Fabricante / Company Name:** ErickEsc Audio
- **Manufacturer Code (JUCE / VST3):** `Eesc` (4 caracteres, al menos una mayúscula)
- **Plugin Code (JUCE / VST3):** `Chs1` (4 caracteres únicos)
- **Identificadores VST3:** Inmutables tras la primera release pública para preservar compatibilidad con proyectos de DAWs (FL Studio, etc.).

---

## Licenciamiento

- **Código fuente del proyecto:** Licencia MIT (ver archivo `LICENSE`).
- **Framework JUCE:** Sujeto a los términos de licencia de JUCE (GPLv3 para distribución abierta / Comercial para distribución propietaria cerrada). La licencia MIT del VST3 SDK no exime del cumplimiento de la licencia de JUCE.
- **VST3 SDK (Steinberg):** Licencia MIT.

---

## Plataforma y Entorno de Desarrollo

- **Plataforma Objetivo de Release V1:** Windows 10/11 x64.
- **Host de Validación Primario:** FL Studio (Windows x64, soporte nativo VST3).
- **IDE / Toolchain Windows:** Visual Studio 2022 (MSVC C++20) + CMake 3.22+.
- **Build System:** CMake + CMake Presets (`windows-msvc-debug`, `windows-msvc-release`).
- **Entorno de Pruebas Unitarias:** Catch2 v3.15.3.
- **Validador de Plugins:** `pluginval` v1.0.4 (estrictez nivel 5).

---

## Arquitectura

```text
UI (7 Diatonic Pads / Controls) / Host MIDI Input
                     │
                     ▼
             NoteCommand Queue (Bounded Lock-Free)
                     │
                     ▼
               Music Engine
       (Key, Scale, Diatonic Degree Mapping, Voicings)
                     │
                     ▼ (Internal MIDI Notes)
               Synth Engine
       (Voice Allocator, 16 Stereo Voices, Polyphony)
                     │
                     ▼ (Stereo Audio Buffer)
                DSP Chain
   (Filter Low-Pass -> Chorus -> Delay -> Reverb -> Master Gain)
                     │
                     ▼
         Audio Output (Standalone Device / VST3 Host)
```

---

## Documentación y Referencias

- Plan de implementación: `docs/plans/2026-08-20-chord-synth-implementation-plan.md`
- Referencias y marco regulatorio/funcional: `docs/references.md`
