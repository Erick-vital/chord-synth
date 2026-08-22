# Guía de Compilación en Windows (MSVC & CMake)

Esta guía detalla los pasos para compilar, probar y empaquetar **ChordSynth** (Standalone y plugin VST3) en sistemas Windows 10/11 x64 utilizando Visual Studio 2022 y CMake.

---

## Requisitos Previos

1. **Windows 10 / 11 (64-bit)**.
2. **Visual Studio 2022** (Community, Professional o Enterprise) con la carga de trabajo *Desktop development with C++* instalada (incluyendo MSVC v143 y herramientas C++20).
3. **CMake 3.22 o superior** (incluido por defecto en Visual Studio o disponible vía PATH).
4. **Git** con soporte para submódulos.

---

## Clonar el Repositorio

Clona el repositorio asegurando la inicialización recursiva de los submódulos (JUCE y dependencias):

```powershell
git clone --recurse-submodules https://github.com/erickesc/chord-synth.git
cd chord-synth
```

Si ya lo habías clonado sin submódulos:

```powershell
git submodule update --init --recursive
```

---

## Compilación con CMake Presets (Recomendado)

### 1. Configurar y Compilar en Release

Abre una terminal de PowerShell o la consola de comandos de desarrollador de Visual Studio (*x64 Native Tools Command Prompt*):

```powershell
# Configurar Release
cmake --preset windows-msvc-release

# Compilar todos los artefactos en Release
cmake --build --preset windows-msvc-release --config Release --parallel
```

### 2. Configurar y Compilar en Debug

```powershell
# Configurar Debug
cmake --preset windows-msvc-debug

# Compilar en Debug
cmake --build --preset windows-msvc-debug --config Debug --parallel
```

---

## Ejecutar Pruebas Automatizadas

Una vez compilado, puedes correr la suite completa de pruebas unitarias y de render offline con `ctest`:

```powershell
ctest --test-dir out/build/windows-msvc-release -C Release --output-on-failure
```

---

## Ubicación de Binarios y Artefactos

Tras la compilación exitosa en Release, los binarios se encuentran en:

- **Standalone App (.exe):**
  `out/build/windows-msvc-release/ChordSynth_artefacts/Release/Standalone/ChordSynth.exe`
- **Plugin VST3 (.vst3):**
  `out/build/windows-msvc-release/ChordSynth_artefacts/Release/VST3/ChordSynth.vst3`

---

## Instalación para DAWs (FL Studio, Ableton, Reaper)

Copia la carpeta o binario `.vst3` en el directorio estándar de plugins VST3 de Windows:

```powershell
# Ruta estándar del sistema:
C:\Program Files\Common Files\VST3\ChordSynth.vst3
```

Abre FL Studio -> *Manage Plugins* -> *Find installed plugins* para detectarlo y cargarlo como generador de instrumento.
