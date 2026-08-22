# Guía de Usuario - ChordSynth

**ChordSynth** es un sintetizador e instrumento de acordes diatónicos diseñado para la interpretación en vivo y composición fluida sin riesgo de notas fuera de escala.

---

## 1. Conceptos Fundamentales

- **Tonalidad Mayor (12 Keys):** Selecciona la tonalidad raíz (C, C#, D, D#, E, F, F#, G, G#, A, A#, B). La armonía diatónica y las tríadas asociadas a cada pad se recalculan automáticamente.
- **7 Pads Diatónicos:**
  - **Pad 1 (I):** Tónica mayor (ej. en C: C mayor - C, E, G).
  - **Pad 2 (ii):** Supertónica menor (Dm - D, F, A).
  - **Pad 3 (iii):** Mediante menor (Em - E, G, B).
  - **Pad 4 (IV):** Subdominante mayor (F - F, A, C).
  - **Pad 5 (V):** Dominante mayor (G - G, B, D).
  - **Pad 6 (vi):** Relativo menor (Am - A, C, E).
  - **Pad 7 (vii°):** Sensible disminuida (Bdim - B, D, F).

---

## 2. Motor de Síntesis y Controles

- **Formas de Onda (Waveform):**
  - `Sine`: Tono puro, cálido y redondo.
  - `Saw`: Sonido brillante y rico en armónicos, ideal para leads y pads densos.
  - `Square`: Timbre hueco y vintage, estilo chiptune y bajos clásicos.
  - `Triangle`: Armónicos impares suaves, puente entre el seno y la sierra.
- **Detune Estéreo:** Desafinación sutil en centésimas entre voces estéreo para ensanchamiento del campo sonoro.
- **Filtro Low-Pass:** Filtro global con corte de frecuencia (`Cutoff` de 20 Hz a 20 kHz) y resonancia (`Resonance` de 0.1 a 2.0).

---

## 3. Sección de Efectos de Color y Ambiente

1. **Stereo Chorus:**
   - `Chorus Mix`: Proporción de efecto mezclado con señal seca (0.0 a 1.0).
   - `Chorus Rate`: Velocidad de modulación (0.1 Hz a 10.0 Hz).
   - `Chorus Depth`: Profundidad de modulación LFO.
2. **Tempo Delay:**
   - `Delay Mix`: Nivel de mezcla de eco (0.0 a 1.0).
   - `Delay Feedback`: Cantidad de repeticiones (0.0 a 0.95).
   - `Delay Time`: Tiempo libre de 10 ms a 2000 ms.
   - `Delay Sync`: Activación de sincronización al tempo del host DAW.
   - `Delay Rate`: Intervalos musicales de división rítmica (`1/4`, `1/8`, `1/16`).
3. **Reverb:**
   - `Reverb Mix`: Proporción de señal reverberada.
   - `Room Size`: Dimensión del espacio acústico virtual.
   - `Damping`: Atenuación de altas frecuencias en la cola.
   - `Width`: Amplitud estéreo del campo reverberante.

---

## 4. Arpegiador Sincronizado

- **Arp Enabled:** Interruptor de activación del arpegiador.
- **Arp Mode:**
  - `Up`: Recorre las notas sostenidas de grave a agudo.
  - `Down`: Recorre las notas sostenidas de agudo a grave.
  - `Up/Down`: Recorrido pendular sin duplicar los extremos.
  - `Random`: Selección aleatoria determinista sobre las notas sostenidas.
- **Arp Rate:** Sincronización rítmica (`1/4`, `1/8`, `1/16`).
- **Arp Gate:** Longitud de articulación y duración de cada nota disparada (10% a 100%).

---

## 5. Presets y Gestión de Estado

Los presets se guardan en formato JSON estructurado y versionado (`schema_version: 1`), conteniendo la totalidad de los parámetros del sintetizador, efectos y estado de reproducción para una interoperabilidad perfecta y restauración sin clics ni cuelgues.
