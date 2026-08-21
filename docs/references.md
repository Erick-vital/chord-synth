# Referencias del Proyecto y Marco Funcional

Este documento recopila las referencias de diseño, técnicas y licencias consultadas para el desarrollo de **ChordSynth**.

---

## 1. Referencia Funcional e Inspiración: HiChord

- **Fuente:** HiChord Manual & User Guide (Revisión 2.8)
- **URL:** <https://hichord.shop/pages/manual#quickstart>
- **Rol en el proyecto:** Referencia funcional conceptual.
- **Límites éticos y legales:**
  - El manual y producto de HiChord se usan únicamente como caso de estudio y referencia para conceptos generales de teoría musical y flujo de acordes diatónicos asistidos.
  - **No** se copia código fuente, recursos gráficos, marcas registradas, nombres de presets propietarios ni muestras de audio de HiChord.
  - ChordSynth es una implementación original y limpia desde cero en C++20 con JUCE.

### Conceptos Diatónicos adoptados
- 7 botones/pads mapeados a los grados diatónicos de la escala (I, ii, iii, IV, V, vi, vii°).
- Modificación/inversión de acordes manteniendo relaciones tonales consistentes.
- Cadena de audio modular: Generador -> Envolvente -> Filtro -> Efectos estéreo -> Master.

---

## 2. Framework y Librerías

### JUCE Framework
- **Versión:** `9.0.1` (fijada vía Git Submodule en `external/JUCE`).
- **Repositorio:** <https://github.com/juce-framework/JUCE>
- **Licencia:** GPLv3 (código abierto) / Licencia Comercial de JUCE (distribución comercial propietaria).
- **Módulos clave:** `juce_core`, `juce_events`, `juce_audio_basics`, `juce_audio_devices`, `juce_audio_processors`, `juce_audio_utils`, `juce_dsp`, `juce_gui_basics`.

### VST3 SDK (Steinberg Media Technologies GmbH)
- **Licencia:** MIT (incorporado transparentemente a través de JUCE).
- **Nota legal:** La licencia MIT del VST3 SDK permite su uso sin coste de regalías, pero el uso conjunto con JUCE exige cumplir las condiciones de licencia de JUCE.

### Catch2
- **Versión:** `v3.15.3`
- **Repositorio:** <https://github.com/catchorg/Catch2>
- **Licencia:** BSL-1.0 (Boost Software License 1.0).
- **Uso:** Framework de pruebas unitarias para el motor de teoría musical (`src/music`) y componentes DSP aislados (`src/dsp`).

### pluginval (Tracktion)
- **Versión:** `v1.0.4`
- **Repositorio:** <https://github.com/Tracktion/pluginval>
- **Licencia:** GPLv3 / Comercial.
- **Uso:** Validación estricta de estabilidad, gestión de buses, sincronización de host y seguridad de hilos del binario VST3 antes de pruebas en DAW.

---

## 3. Parámetros e Identificadores Inmutables

Para garantizar que los archivos de proyecto de FL Studio (`.flp`) y otros DAWs no pierdan la referencia al plugin al actualizar versiones:

- **Plugin Name:** `ChordSynth`
- **Product Name:** `ChordSynth`
- **Manufacturer Code:** `Eesc`
- **Plugin Code:** `Chs1`
- **Plugin Category:** `Synth` / `Instrument`
