# ChordSynth — Diseño de interfaz de interpretación armónica

Fecha: 2026-08-21

Prototipo interactivo: `docs/design/chord-synth-interface-prototype.html`

## Propósito

ChordSynth debe abrir como un instrumento inmediatamente interpretable: el usuario elige un contexto armónico y toca uno de siete acordes. No hay botón genérico “Generate” ni reproducción automática implícita. Presionar inicia las notas; soltar inicia su release.

## Jerarquía de la pantalla

1. **Contexto armónico:** tonalidad, escala y regla diatónica/libre.
2. **Interpretación:** teclado de siete acordes y control de re-voicing sostenido.
3. **Estado actual:** acorde y notas que están sonando.
4. **Diseño por grado:** extensión, inversión, distribución, bajo, política de quinta y registro del grado seleccionado.
5. **Timbre y movimiento:** oscilador, filtro, arpegiador y efectos.
6. **Infraestructura:** preset y configuración Audio/MIDI, fuera del flujo musical principal.

## Teclado de siete acordes

Cada tecla representa un grado, no una nota cromática. En C mayor, las siete teclas muestran `I C`, `ii Dm`, `iii Em`, `IV F`, `V G`, `vi Am`, `vii° Bdim`.

Fuentes de interacción:

- puntero/touch: `pointerDown` produce note-on; `pointerUp`, cancelación, pérdida de foco y destrucción producen los note-off exactos;
- teclado del ordenador: `Q W E R T Y U`;
- MIDI externo: permanece cromático en el procesador inicialmente o se mapea a grados con MIDI Perf activado.

## Voicing interactivo y presets

ChordSynth maneja una configuración editable de siete grados por preset. Las recetas de fábrica se acceden mediante los presets incorporados o configurando manualmente cada grado:

- **Init / Diatónica:** tríadas en posición raíz cerrada;
- **Séptimas:** calidad diatónica del séptimo acorde en posición compacta;
- **Lo‑Fi Warm:** novenas y trecenas con distribución abierta y bajo raíz;
- **Jazz Tension:** extensiones rootless con bajo raíz.

Comportamiento predeterminado: editar un grado en el diseñador afecta al siguiente acorde disparado. No altera un acorde sostenido, evitando cortes sorpresivos.

Con **Re-voicing del acorde sostenido** activado, modificar el grado sostenido calcula la diferencia entre las notas anteriores y nuevas:

- conserva notas comunes;
- envía note-off solo para notas eliminadas;
- envía note-on solo para notas añadidas;
- actualiza el registro de notas sostenidas para que la liberación posterior sea exacta.

## Diseño por grado

Seleccionar una tecla abre su configuración sin dispararla necesariamente. La primera entrega editable incluye:

- extensión: triada o séptima;
- inversión: raíz, primera o segunda;
- distribución: cerrada o abierta;
- registro base: octavas 2–4.

El modo **Diatónico** conserva la calidad derivada de la escala. El modo **Libre** permite sustituir la calidad de un grado en una fase posterior; por ejemplo, usar Dmaj7 dentro de una progresión cuyo contexto inicial sea C mayor. No se implementarán 28 voicings ni un secuenciador en esta entrega.

## Estado y feedback

- La tecla pulsada se hunde visualmente.
- “Ahora” muestra grado, nombre y notas MIDI musicales.
- Los cambios guardados muestran confirmación visible en el panel.
- El estado de Audio/MIDI está presente pero no domina la interfaz.
- Al perder foco se ejecuta all-notes-off de las notas originadas por la UI.

## Layout visual

- Ventana objetivo: `1180 × 760`, redimensionable con mínimo operativo definido durante implementación.
- Tema oscuro original con un único acento lima para estados activos y cian para notas/información musical.
- Controles con objetivos de al menos 36 px en escritorio y foco de teclado visible.
- Sin glassmorphism, medidores inventados ni decoración que compita con las siete teclas.

## Separación arquitectónica

La UI no calcula teoría musical ni toca el sintetizador directamente:

- `music/` genera acordes y voicings de forma pura y determinista;
- un controlador de interpretación conserva las notas activas y crea comandos MIDI;
- `UiMidiQueue` cruza de message thread a audio thread sin locks ni allocations nuevas en `processBlock`;
- el editor presenta estado y usa attachments APVTS para parámetros automatizables;
- el procesador continúa siendo la única fuente de audio para Standalone y VST3.

## Criterios de aceptación de experiencia

1. Abrir Standalone muestra las siete teclas sin abrir menús.
2. En C mayor, mantener `I` produce C–E–G y soltar libera exactamente esas notas.
3. `Q–U` replica las siete teclas y no deja notas colgadas al perder foco.
4. Tocar en modo Séptimas o con preset de séptimas produce las extensiones esperadas (ej. `V` produce G–B–D–F en C mayor).
5. Modificar un grado mientras se sostiene un acorde no lo modifica por defecto hasta el siguiente disparo.
6. Con re-voicing activo, el cambio conserva notas comunes y no duplica note-ons.
7. Tonalidad, parámetros de sonido y configuración armónica sobreviven al guardado/restauración.
8. Standalone y VST3 muestran la misma interfaz y comportamiento musical.
