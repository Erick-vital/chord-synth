# Diseño: armonía Lo‑Fi/Jazz y rendimiento

## Alcance

ChordSynth separa la identidad del acorde de su presentación. La identidad procede de la tonalidad, escala, grado, forma y regla de calidad. La presentación decide cómo se distribuyen las notas, qué se omite, el registro, el bajo y el voice leading.

La primera versión admite tríada, séptima, novena, oncena, trecena, add9, 6/9, sus2 y sus4. Las tensiones alteradas (`b9`, `#9`, `#11`, `b13` y `alt`) no forman parte de esta versión.

## Receta frente a voicing

`ChordShape` define la receta musical; `VoicingSpec` define cómo se interpreta:

- Forma: tríada, 7, 9, 11, 13, add9, 6/9, sus2 o sus4.
- Calidad: diatónica o, en modo libre, mayor, menor, dominante o disminuida.
- Estilo: compacto, abierto o rootless.
- Quinta: automática, incluida u omitida.
- Bajo: sin bajo, raíz o slash por grado.
- Voice leading: manual o nearest.

Las distinciones de etiqueta son deliberadas:

- `Cadd9`: C–E–G–D; no contiene séptima.
- `Cmaj9`: C–E–G–B–D; séptima mayor y novena.
- `C9`: C–E–G–Bb–D; séptima menor dominante y novena.
- `Cm9`: C–Eb–G–Bb–D; tercera y séptima menores.
- `C6/9`: C–E–G–A–D; sexta y novena, sin séptima.

La escala mayor y la menor natural resuelven calidad y séptimas diatónicas. No se convierte implícitamente el V de menor natural en dominante de menor armónica.

## Reglas de voicing y registro

- Compacto ordena las notas de forma ascendente con el menor span legal.
- Abierto aplica una distribución determinista tipo drop/spread.
- Rootless elimina la raíz solo para recetas de séptima o superiores; una tríada o suspensión cae de forma segura a compacto.
- La política automática conserva la quinta en tríadas y suspensiones. Para recetas que superarían las seis voces, el motor aplica omisiones estructurales deterministas; en una trecena elimina antes la 11ª que la quinta.
- El motor conserva como máximo seis voces armónicas. Un bajo opcional se transporta por separado.
- Acordes densos de cinco o más tonos respetan un suelo de C3 (MIDI 48). Voicings rootless de séptima o superiores respetan además un suelo de E3 (MIDI 52); el techo armónico es C7 (MIDI 96). El bajo se transpone independientemente al rango MIDI 24–47.
- Cuando no existe una distribución legal, el motor produce un compacto seguro y determinista; no lanza excepciones durante la interpretación.

El voice leading nearest acepta explícitamente el `NoteSet` anterior y resuelve candidatos acotados. Minimiza movimiento con desempates deterministas; no oculta historial mutable dentro del voicer. El bajo no participa en ese cálculo.

## Escenas de fábrica

| Escena | Nombre | Formas I–vii° | Estilo | Bajo | Leading |
|---|---|---|---|---|---|
| A | Diatónica | tríada en todos los grados | compacto | ninguno | manual |
| B | Séptimas | séptima en todos los grados | compacto | ninguno | nearest |
| C | Lo‑Fi Warm | 9, 9, 7, 9, 13, 9, 7 | abierto | raíz | nearest |
| D | Jazz Tension | 6/9, 11, 9, 9, 13, 11, 7 | rootless | raíz | nearest |

Las escenas son datos por grado y no codifican tonos de C. El resolvedor las adapta a la tonalidad y a Mayor o Menor natural.

## Transformaciones temporales

Los colores de acorde son transformaciones no persistentes sobre la especificación guardada del grado activo. Al pulsar un color, el controlador calcula la transformación desde la base guardada, emite únicamente el delta de note-off/note-on y conserva el estado temporal. Al soltarlo, restaura la base usando otro delta. **Fijar en grado** persiste la transformación en la celda escena/grado activa sin rearticular el acorde que ya está sonando.

Cada paleta contiene ocho ranuras:

- Básica: flip mayor/menor, dominante 7, séptima color, add9, sus4, sus2, 6/9, disminuido.
- Lo‑Fi: 9, add9, 6/9, 11, open 9, rootless 7, warm 13, nearest open.
- Spice: dominante 7, disminuido 7, sus4 tensión, dominante 9, dominante 13, menor 9 tensión, rootless 9, open 11.

No se acumulan transformaciones: cambiar de ranura vuelve a partir de la base. Cambiar tonalidad, escala o escena, perder el foco o cerrar el editor libera el acorde activo y termina la transformación.

## Entrada de interpretación

### Teclado del ordenador

- `Q W E R T Y U`: grados I–vii°; pulsar mantiene el acorde y soltarlo envía sus note-offs.
- `1`–`4`: seleccionan las escenas A–D.
- `A S D F G H J K`: mantienen las ocho ranuras de color de la paleta seleccionada.

Los atajos de color no se capturan cuando un `TextEditor` o `ComboBox` tiene el foco.

### MIDI de rendimiento

El mapeo MIDI es opcional y viene desactivado. Cuando está activo:

- Notas MIDI 36–42 disparan grados I–VII.
- CC 20–27: valor `>= 64` pulsa la ranura de color correspondiente; valor `< 64` la libera.
- Notas, controles y otros eventos no mapeados pasan sin cambios y conservan su sample offset.
- Note-off repetidos y CC 120/123 limpian el estado de notas mapeadas para evitar notas colgadas. El mapeo mantiene un único acorde/grado activo; las pulsaciones MIDI superpuestas no tienen todavía semántica de conteo independiente.

## Enrutamiento bajo/arpegiador

Los tonos armónicos generados usan el canal MIDI interno 1. El bajo independiente usa el canal interno 2. Con el arpegiador activo, el canal 2 evita el arpegiador y se mezcla directamente con el sintetizador; las notas externas en otros canales siguen la ruta normal del arpegiador. El sintetizador renderiza ambos canales.

## Estado, presets y compatibilidad

`HarmonyState` actual es versión 2 y persiste forma, inversión, estilo, octava, regla de calidad, política de quinta, modo/slash de bajo y voice leading para cada grado. Carga estados v1 y conserva sus defaults legados en vez de sustituirlos por las escenas nuevas.

Los presets JSON actuales usan `schema_version: 3`. Los esquemas 1 y 2 siguen cargando con defaults/mapeos de compatibilidad; las nuevas escrituras emiten esquema 3. Cargar un preset desde la interfaz siempre libera primero el acorde activo y sincroniza APVTS, HarmonyState, controlador, toolbar, escena visible, pads y diseñador.

## Seguridad en tiempo real

La generación de recetas, voicings, transformaciones, diferencias MIDI y mapeo MIDI está diseñada para usar capacidad fija. En `processBlock` no deben introducirse locks, logging, parsing ni I/O. La eliminación completa de asignaciones es una meta aún pendiente de endurecimiento: el voicer construye etiquetas y algunos búferes MIDI pueden crecer en rutas actualmente activas. La comunicación desde la interfaz usa una cola MIDI lock-free y los parámetros automatizables se leen como atómicos.
