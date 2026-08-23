# Guía de usuario — ChordSynth

ChordSynth es un sintetizador polifónico y un instrumento de acordes por grados. Comparte el mismo motor musical entre Standalone y VST3.

## 1. Tocar por grados

Selecciona una tonalidad y una escala (Mayor o Menor natural) en la barra de armonía. Los siete pads representan los grados I a vii° y se actualizan con sus etiquetas y notas.

| Tecla | Grado en C mayor |
|---|---|
| Q | I: C |
| W | ii: Dm |
| E | iii: Em |
| R | IV: F |
| T | V: G |
| Y | vi: Am |
| U | vii°: Bdim |

Mantén la tecla o el pad para sostener el acorde y suéltalo para liberar exactamente las notas que se enviaron. Los cambios desde los controles de la interfaz para tonalidad, escala y preset, y la pérdida de foco, liberan el acorde activo para no dejar notas colgadas.

## 2. Escenas de armonía

Usa los botones A–D o las teclas `1`–`4` para cambiar escena. Cada escena se adapta a la tonalidad y escala actuales.

| Escena | Uso |
|---|---|
| A · Diatónica | tríadas compactas; buen punto de partida |
| B · Séptimas | séptimas diatónicas compactas con nearest voice leading |
| C · Lo‑Fi Warm | 9/13 abiertos, bajo raíz y voice leading cercano |
| D · Jazz Tension | 6/9, 9, 11 y 13 rootless con bajo raíz |

Por defecto, un cambio de escena afecta al siguiente acorde. Activa **Re-voicing del acorde sostenido** si quieres actualizar diferencialmente el acorde que mantienes pulsado.

## 3. Diseñar un acorde

Selecciona un grado y abre **Diseñar acorde**. Los cambios muestran una previsualización antes de guardar. La calidad manual está bloqueada en modo Diatónico y se habilita en modo Libre.

### Formas

- **Triada**: raíz, tercera y quinta.
- **7**: añade la séptima determinada por escala/calidad.
- **9 / 11 / 13**: extensiones diatónicas; el motor omite voces de forma determinista cuando es necesario.
- **add9**: novena sin séptima.
- **6/9**: sexta y novena, sin séptima.
- **sus2 / sus4**: sustituyen la tercera por segunda o cuarta.

Por ejemplo, en C mayor: `Cadd9` es C–E–G–D, `Cmaj9` añade B, y `G9` contiene la séptima menor F. Las alteraciones `b9`, `#9`, `#11`, `b13` y `alt` no están incluidas todavía.

### Voicing, bajo y registro

- **Compacto** reúne las voces; **Abierto** las separa; **Rootless** omite la raíz en acordes de séptima o superiores.
- La quinta puede ser Auto, Incluir u Omitir.
- El bajo puede ser Sin bajo, Raíz o Slash. El bajo se muestra aparte y viaja por un canal interno separado.
- Manual usa el voicing solicitado; Automático/nearest busca la continuación más cercana desde el acorde anterior.

El motor limita la armonía a seis notas y un bajo opcional, conserva registros musicales seguros y usa un compacto seguro si la solicitud no cabe en rango.

## 4. Colores de acorde

El panel **Chord Color** ofrece tres paletas: Básica, Lo‑Fi y Spice. Sus ocho botones se pueden mantener con `A S D F G H J K`.

Mientras un grado está sostenido, pulsar un color modifica temporalmente el acorde. Soltarlo restaura la configuración guardada. **Fijar en grado** guarda el color actual en la escena y grado seleccionados; no hace un corte ni rearticula las notas que ya suenan.

La paleta Básica ofrece flip mayor/menor, dominante 7, séptima color, add9, sus4, sus2, 6/9 y disminuido. Lo‑Fi añade 9, add9, 6/9, 11, open/rootless y warm 13. Spice ofrece dominantes, disminuidos y tensiones disponibles en v1. Los atajos no actúan mientras editas texto o un selector tiene el foco.

## 5. MIDI de rendimiento

Activa **MIDI Perf** para habilitar el mapeo semántico. Por defecto está apagado; cuando está apagado, todo el MIDI pasa intacto.

- Notas 36–42: grados I–VII.
- CC 20–27: ocho colores. `>=64` pulsa; `<64` libera.
- Otros eventos conservan canal y sample offset.
- CC 120/123 y note-offs limpian las notas mapeadas para prevenir notas colgadas.

Los tonos de acorde generados usan canal interno 1. El bajo generado usa canal interno 2; con el arpegiador activo, el canal 2 evita el arpegiador. El MIDI externo conserva su canal y, salvo el canal 2, sigue la ruta normal del arpegiador.

## 6. Sonido, arpegiador y efectos

Selecciona Sine, Saw, Square o Triangle; ajusta detune, filtro low-pass y resonancia. El arpegiador ofrece Up, Down, Up/Down y Random con `1/4`, `1/8` o `1/16` y control de gate. Chorus, delay libre/sincronizado y reverb son globales.

## 7. Presets y estado

El selector **Preset** incluye:

- Default (Init): escena A.
- Warm Saw Chords: escena C.
- Ambient Open Keys: escena C con re-voicing activo.
- Arp Plucks: escena B y arpegiador activo.
- Jazz Tension: escena D con re-voicing activo.

Al cargar un preset, ChordSynth libera primero el acorde activo y sincroniza sonido, tonalidad, escena, pads, diseñador y paleta. Los presets actuales usan el esquema JSON 3; se mantienen compatibles los esquemas 1 y 2. HarmonyState v2 carga además estados v1 con sus valores legados.

## 8. Standalone y VST3

En Standalone, **Audio / MIDI** permite seleccionar dispositivos y entradas. En VST3 el host controla el dispositivo, por lo que esa acción aparece administrada por el host.

Para validar en Windows, compila y prueba ambos formatos:

```powershell
cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release --config Release --parallel
ctest --test-dir out/build/windows-msvc-release -C Release --output-on-failure
```

Después abre Standalone y valida escenas, colores, liberación por foco, mapeo MIDI, bajo con arpegiador, restauración de presets y proyectos. Prueba el VST3 en tu DAW y con pluginval siguiendo las listas del repositorio.
