# Portal 2 — Mecánicas internas y arquitectura de compatibilidad

> **Documento técnico de investigación — ReSource / Portal 2**  
> **Estado:** investigación en curso · **Fecha inicial:** 8 de octubre de 2026  
> **Ámbito:** el port experimental de Portal 2 construido sobre Source, y la comparación pendiente con el juego original.  
> **No confundir:** las pruebas aquí descritas demuestran el comportamiento **del port**, no automáticamente el de los binarios comerciales de Valve.

## Índice

1. [Propósito y alcance](#1-propósito-y-alcance)
2. [Método de investigación y grados de evidencia](#2-método-de-investigación-y-grados-de-evidencia)
3. [Mapa general de los sistemas](#3-mapa-general-de-los-sistemas)
4. [Movimiento del jugador](#4-movimiento-del-jugador)
5. [Ascensores y plataformas móviles](#5-ascensores-y-plataformas-móviles)
6. [Resultados de instrumentación: `sp_a1_intro1`](#6-resultados-de-instrumentación-sp_a1_intro1)
7. [Portales y teletransporte](#7-portales-y-teletransporte)
8. [Geles y superficies](#8-geles-y-superficies)
9. [Tractor Beam y otras mecánicas](#9-tractor-beam-y-otras-mecánicas)
10. [Entidades, mapas y entradas/salidas](#10-entidades-mapas-y-entradassalidas)
11. [Escenas, VScript y secuencias](#11-escenas-vscript-y-secuencias)
12. [Video, audio y materiales](#12-video-audio-y-materiales)
13. [Compilación, pruebas y diagnóstico](#13-compilación-pruebas-y-diagnóstico)
14. [Hipótesis abiertas y matriz de validación](#14-hipótesis-abiertas-y-matriz-de-validación)
15. [Hoja de ruta para ReSource](#15-hoja-de-ruta-para-resource)
16. [Glosario y referencias internas](#16-glosario-y-referencias-internas)

---

## 1. Propósito y alcance

Este archivo registra **cómo se observan y reconstruyen las mecánicas de Portal 2** en la rama experimental de Source, con especial atención a la reproducibilidad. Su objetivo no es presentar pseudocódigo aproximado como si fuese el código original de Valve. Separa cuatro cuestiones:

- Qué hace el **juego comercial** (solo cuando existe evidencia directa verificable).
- Qué hace **nuestro port** (instrumentación, logs, pruebas y código compilado).
- Qué **inferimos** a partir de esos datos.
- Qué **falta medir** antes de implementar o afirmar compatibilidad.

El documento complementa `ALLAZGOS.md` y no lo reemplaza. `ALLAZGOS.md` sigue siendo el registro primario de investigaciones detalladas, direcciones, hipótesis y evidencia. Una implementación funcional en un mapa tampoco demuestra compatibilidad con toda la campaña.

## 2. Método de investigación y grados de evidencia

### 2.1 Etiquetas obligatorias

| Etiqueta | Significado | Ejemplo |
|---|---|---|
| **OBSERVADO-PORT** | Medido en nuestra compilación experimental | `ground` conserva el tren durante el descenso probado. |
| **OBSERVADO-ORIGINAL** | Medido directamente en una copia del Portal 2 original, con archivo/versión y método registrados | *Pendiente para el comportamiento exacto del ascensor.* |
| **CÓDIGO-PORT** | Verificado por lectura del código de nuestra rama | La sonda de transición está en el override de `CPortalGameMovement`. |
| **INFERENCIA** | Explicación compatible con la evidencia, pero todavía no demostrada por completo | El transporte vertical parece deberse al movimiento de una entidad empujadora. |
| **HIPÓTESIS** | Posibilidad que requiere una prueba diseñada | Una ruta de salida puede comportarse distinto a la llegada. |
| **PENDIENTE** | No hay evidencia suficiente | Equivalencia exacta de los geles con retail. |

### 2.2 Procedimiento

1. Definir una pregunta concreta y un resultado esperado.
2. Identificar archivo, mapa, entidad y rutina efectivos; no asumir que la clase base es la que corre.
3. Añadir instrumentación **opt-in**, de alcance limitado y sin alterar la mecánica observada.
4. Compilar; conservar comando, plataforma, versión y resultado.
5. Ejecutar una prueba repetible; guardar `engine.log` y artefactos.
6. Comparar por **tiempo** los registros que usen contadores distintos.
7. Repetir en varias condiciones y contrastar, si es posible, con el original.
8. Separar causa demostrada, correlación y posibles explicaciones.
9. No convertir una solución temporal de pruebas en un cambio permanente sin validación.

**Regla:** encontrar una cadena, `ConVar` o un bloque `#if 0` dentro de un binario o árbol de código **no demuestra** que tal comportamiento estuviera activo en retail. En particular, no habilitar a ciegas `ClientVerticalElevatorFixes`.

## 3. Mapa general de los sistemas

```text
Portal 2 (objetivo de compatibilidad)
├── Motor Source / infraestructura común
│   ├── mundo, colisiones y físicas
│   ├── entidades y E/S de mapas
│   ├── audio, materiales, escenas y recursos
│   └── carga de mapas, transición y estado
├── Código específico de Portal 2
│   ├── CPortalGameMovement
│   ├── portales y transformaciones espaciales
│   ├── objetos interactivos y armas
│   ├── geles, campos y otras mecánicas
│   └── lógica de progresión
└── Herramientas de investigación del port
    ├── sondas de movimiento
    ├── scripts de compilación y staging
    ├── pruebas automatizadas
    └── ALLAZGOS.md + este documento
```

Esta es una **representación conceptual**, no una transcripción comprobada de la estructura interna de los binarios originales.

## 4. Movimiento del jugador

### 4.1 Conceptos

- **`GroundEntity`:** entidad que el sistema considera la superficie de apoyo del jugador.
- **`SetGroundEntity`:** operación que establece o cambia esa entidad y participa en la gestión del estado de movimiento.
- **`BaseVelocity`:** vector empleado por Source para ciertas contribuciones externas de movimiento; **no equivale necesariamente** a la velocidad visible de una plataforma.
- **`FL_ONGROUND`:** bandera que expresa contacto con suelo.
- **`MOVETYPE_PUSH`:** tipo de movimiento utilizado en entidades móviles que pueden desplazar otras entidades.
- **`player_vel`:** velocidad reportada del jugador; puede ser cero aunque su posición mundial cambie por el transporte de una plataforma.

### 4.2 Ruta efectiva de código en el port

**CÓDIGO-PORT:** la primera sonda se añadió a `CGameMovement::SetGroundEntity` en `game/shared/gamemovement.cpp`, pero **no registró las transiciones** durante las pruebas de Portal 2. La investigación encontró la ruta específica `CPortalGameMovement::SetGroundEntity`, en `game/shared/portal/portal_gamemovement.cpp`, y movió allí el log de transiciones. A partir de entonces se registraron cambios.

**Lección:** localizar una función homónima en la clase base no basta para saber qué implementación utiliza un juego. Hay que seguir la ruta efectiva, incluidos overrides y llamadas virtuales.

### 4.3 Instrumentación disponible

- `portal2_ground_probe`: `ConVar` compartida, predeterminada `0`, definida solo para servidor de Portal 2.
- `PORTAL2_GROUND_SET`: transiciones `old` → `new`, velocidades asociadas, `base_vel`, flags y número de comando.
- `PORTAL2_GROUND`: muestreo por frame de suelo, tren, posición, velocidades, `onground`, `movetype`, tiempo y tick global.

**Advertencia de contadores:** `PORTAL2_GROUND_SET` usa `CurrentCommandNumber()` mientras que `PORTAL2_GROUND` usa `gpGlobals->tickcount`. **Sus valores no son intercambiables**. Correlacionar por timestamp de log o registrar expresamente ambos relojes.

## 5. Ascensores y plataformas móviles

### 5.1 Entidades observadas

En la prueba de llegada de `sp_a1_intro1` aparece una entidad `func_tracktrain`, denominada `arrival_elevator-elevator_1`. El port registra el tren del viaje mediante un `EHANDLE` llamado `g_pPortal2RideTrain`, asignado en las rutinas de llegada y salida.

**CÓDIGO-PORT:** el seguimiento del viaje existe en `game/server/portal2/portal2_map_compat.cpp`. La sonda por frame está construida como sistema de juego que revisa el estado del jugador y del tren.

### 5.2 Secuencia comprobada del ascensor de llegada

```text
Inicio del viaje
    ↓
Jugador se apoya sobre el tren (`GroundEntity = func_tracktrain`)
    ↓
Tren desciende a ~−300 unidades/s en el escenario observado
    ↓
Jugador conserva `GroundEntity`, `FL_ONGROUND = 1`
    ↓
Tren se detiene
    ↓
Prueba `ride=PASS`
    ↓
Jugador se separa del tren y llega al suelo del mundo
    ↓
Prueba `exit=PASS`
```

**OBSERVADO-PORT:** en esta ejecución la pérdida de apoyo se produjo **después** de finalizar el recorrido, compatible con la salida del jugador de una plataforma estacionada. No se observó `ground=NULL` mientras `train_vel_z != 0`.

### 5.3 Lo que NO debe asumirse

La hipótesis inicial de que `base_vel_z` debía ser `−300` **no se sostuvo** en la ejecución estudiada. El tren se movía verticalmente, mientras `base_vel=(0,0,0)` y `player_vel=(0,0,0)`, sin que el jugador se desprendiera. Esto **es compatible** con transporte por el movimiento de la entidad, pero no demuestra por sí solo todo el mecanismo interno ni su equivalencia al Portal 2 comercial.

### 5.4 Salida y transiciones entre mapas

**PENDIENTE:** no se ha validado en estas evidencias un viaje completo del **ascensor de salida**. El escenario anterior ejercitó el recorrido de llegada y una salida de la prueba; el itinerario de departure puede utilizar otra lógica. Hay que probar rutas como `sp_a1_intro2 → sp_a1_intro3` y otras transiciones relevantes.

## 6. Resultados de instrumentación: `sp_a1_intro1`

### 6.1 Entorno y ejecución

**Fuente:** salida de OpenCode/Big Pickle compartida el 8 de octubre de 2026; los números aquí indicados provienen de esa ejecución y no de una auditoría independiente del código actual.

- Arquitectura: **macOS ARM64**.
- Compilación del target `portal2`: **terminó sin errores**; compilación incremental posterior `EXIT=0`.
- Prueba: `GROUND PROBE PASS`, con `ride=PASS` y `exit=PASS`.
- Registros: **1.526** líneas de sonda por frame y **25** transiciones `PORTAL2_GROUND_SET`.
- Descenso observado: movimiento a **−300 unidades/s**.
- `GroundEntity`: conservó la entidad del tren durante el descenso; `onground=1`.
- **Cero** muestras `ground=NULL` durante movimiento vertical del tren en ese registro.
- La última separación del tren se observó **después de aparcar**.

### 6.2 Muestras representativas

Extracto de estructura de campo y valores observados:

```text
PORTAL2_GROUND tick=129 t=1.935 ground=arrival_elevator-elevator_1
  ground_vel=(0.0 0.0 -300.0)
  train_vel=(0.0 0.0 -300.0)
  base_vel=(0.0 0.0 0.0)
  player_vel=(0.0 0.0 0.0)
  onground=1 movetype=2

PORTAL2_ARRIVAL ride=PASS
PORTAL2_ARRIVAL exit=PASS
```

La muestra está presentada en varias líneas para facilitar su lectura; el log original almacena el estado por línea. `movetype=2` describe el **movetype del jugador**, y no debe confundirse con el tipo de movimiento del tren.

### 6.3 Resultado respecto del fallo investigado

El patrón hipotético de fallo de `ALLAZGOS.md` §40 consistía en que el jugador perdía el suelo mientras el tren seguía bajando, posiblemente acompañado de una caída de `base_vel_z` a cero. **No ocurrió en esta corrida.** Dado que `base_vel_z` ya era cero durante el descenso exitoso, la condición `base_vel_z=0` por sí sola **no es indicador suficiente de fallo**.

**Conclusión válida:** el descenso de llegada del port **pasó el escenario instrumentado**.  
**Conclusión NO válida:** “la física del ascensor de Portal 2 retail ha sido reconstruida exactamente” o “todos los ascensores están reparados”.

### 6.4 Ruta del registro

```text
~/Library/Application Support/Steam/steamapps/common/Portal 2/
  portal2_arm64_test/diagnostics/20261008-043953-420682/engine.log
```

Esta ruta era válida en el equipo donde se hizo la prueba; no es una ruta universal.

## 7. Portales y teletransporte

**Estado: EN INVESTIGACIÓN.** La conversación previa menciona trabajo en colocación de portales, movimiento del jugador, predicción y compatibilidad. No se adjuntó en esta sesión una traza suficiente para declarar que su comportamiento reproduce el juego original.

### Aspectos a documentar

| Subsistema | Pregunta técnica | Prueba propuesta |
|---|---|---|
| Colocación | ¿Qué superficies admiten portales y cómo se decide la orientación? | Casos con superficies válidas, bordes, pendientes y superficies móviles. |
| Transformación | ¿Cómo se transforman posición, orientación y velocidad al atravesar el portal? | Medir entrada/salida y conservar magnitudes esperadas. |
| Colisiones | ¿Cómo interactúan jugador, objetos y trazas con el plano portal? | Pruebas de contacto, clipping y pasos parciales. |
| Render | ¿Cómo se resuelve la vista a través de portales? | Pruebas visuales con profundidad/recursión controladas. |
| Predicción | ¿Coinciden cliente y servidor en teletransportes? | Pruebas con latencia o estados reproducibles. |
| Objetos | ¿Cómo se transportan cubos y objetos físicos? | Cruces lentos, rápidos y simultáneos. |

No incorporar “algoritmos originales” sin identificar su procedencia, versión y evidencia.

## 8. Geles y superficies

**Estado: PENDIENTE DE VALIDACIÓN COMPLETA.** Se ha discutido la reconstrucción de geles, pero aquí no hay logs suficientes para describir el funcionamiento exacto de Portal 2 retail.

Investigaciones prioritarias:

- Detección de cobertura y material de superficie.
- Efectos sobre movimiento y colisión del jugador.
- Aplicación a objetos físicos y elementos dinámicos.
- Persistencia visual y física después de guardar/cargar.
- Interacciones con portales, agua y geometrías especiales.

**Regla de implementación:** documentar fórmulas y resultados experimentales antes de ajustar constantes para que “se sienta parecido”.

## 9. Tractor Beam y otras mecánicas

**Estado: PENDIENTE.** Crear secciones específicas cuando existan pruebas instrumentadas de:

- Excursion Funnel / Tractor Beam: fuerzas, velocidades, entrada/salida y objetos.
- Placas de presión: entidades, señales y cambios de estado.
- Cubos: tipos, transporte, colisión y activación de receptores.
- Turretas: estados de percepción, animación y disparo.
- Ascensores especiales, paneles móviles y mecanismos de progresión.

Estas categorías son un **inventario de estudio**, no afirmaciones de implementación terminada.

## 10. Entidades, mapas y entradas/salidas

### 10.1 Sistema de E/S

Source utiliza entidades, propiedades y conexiones de entradas/salidas para orquestar eventos dentro de mapas. La implementación exacta de las clases y callbacks utilizadas por Portal 2 debe verificarse en el árbol actual y mediante pruebas.

### 10.2 Lo observado en el port

- El staging aislado carga `sp_a1_intro1`.
- Se registraron marcadores de llegada y salida (`PORTAL2_ARRIVAL`).
- Se observó referencia a `sp_a1_intro2` durante el flujo de salida.
- El sistema de pruebas afirma expresamente que sus escenarios **no equivalen a una campaña completa**.

### 10.3 Pruebas pendientes

- Recorrido real de todos los cambios de mapa iniciales.
- Conservación de arma, inventario y estado del jugador.
- Eventos diferidos, entidades persistentes y scripts entre niveles.
- Comparación de orden de eventos frente a una referencia original.

## 11. Escenas, VScript y secuencias

### Escenas

El staging de investigación registró preparación de escenas y audio para la introducción, incluidos recursos de diálogo. Esto demuestra trabajo de compatibilidad de recursos, **no** la ejecución completa y fiel de todas las escenas del juego.

### VScript

**Estado: BLOQUEADOR GENERAL PENDIENTE.** Se ha identificado como un área extensa de reconstrucción. Antes de implementar API o comportamiento, preparar una tabla de funciones requeridas por los scripts reales, su disponibilidad en la base y sus diferencias comprobadas.

| Área | Evidencia que necesitamos |
|---|---|
| Inicialización del runtime | Punto real de carga, errores y ciclo de vida. |
| Vinculación a entidades | Métodos expuestos, eventos y parámetros. |
| Estado y persistencia | Qué sobrevive a transición y carga. |
| Temporizadores y callbacks | Orden, tiempo y posibles condiciones de carrera. |
| Compatibilidad de scripts | Suite pequeña con casos reproducibles. |

Evitar importar implementaciones de otros juegos Source como si fuesen equivalentes a las de Portal 2.

## 12. Video, audio y materiales

### 12.1 Audio

**OBSERVADO-PORT:** aparecieron mensajes repetidos:

```text
AUDIOQUEUE STARVED, restarting
```

En una corrida previa estos avisos coincidieron con una prueba que no terminó correctamente. Una nueva corrida con **`snd_audioqueue_depth=32`** permitió completar la prueba.

**Interpretación:** es un **workaround de ejecución**, no una corrección permanente del motor ni prueba de causalidad única. Investigar backend de audio, tamaños de buffer, sincronización y rendimiento antes de cambiar valores por defecto.

### 12.2 Materiales y texturas

Se observó una **mejora visual** según la evaluación manual del desarrollador: disminuyeron las texturas moradas. Durante el proceso se instaló Portal (app 400) y se preparó el staging con recursos adicionales. **No está demostrado** si la mejora provino de montaje de VPK, rutas de búsqueda, materiales, shaders u otra modificación.

Prueba futura: comparar captura, inventario de materiales, avisos de error y `search paths` antes/después, manteniendo controladas las dependencias de Portal 1.

### 12.3 Video (Bink / FFmpeg)

**PROPUESTA, NO IMPLEMENTADO:** incorporar un backend FFmpeg, compilado de forma reproducible para macOS ARM64, como dependencia modular de Source/ReSource. Antes de desarrollarlo:

1. Inventariar las interfaces de video existentes en `main`.
2. Identificar la variante real de archivos Bink que necesitan los juegos objetivo.
3. Probar decodificación de **video y audio**, tiempos y sincronización.
4. Adaptar salida a texturas y al sistema de sonido del motor.
5. Conservar un backend antiguo hasta superar pruebas de compatibilidad.
6. Revisar licencias, distribución de bibliotecas y procedencia de archivos de juego.

Compilar FFmpeg con opciones selectivas **no requiere necesariamente un fork modificado**. Solo mantener parches si existen motivos y pruebas concretas.

## 13. Compilación, pruebas y diagnóstico

### 13.1 Comandos documentados

Desde la raíz del repositorio experimental:

```bash
./scripts/build-macos-arm64.sh portal2
```

Con los recursos de staging ya preparados:

```bash
python3 scripts/test-portal2-render.py \
  "$HOME/Library/Application Support/Steam/steamapps/common/Portal 2/portal2_arm64_test" \
  --elevator-transition \
  --ground-probe \
  --map sp_a1_intro1 \
  --seconds 60 \
  --extra-cvar snd_audioqueue_depth=32
```

**Resultado registrado:** `GROUND PROBE PASS` en la ejecución de referencia. El comando necesita archivos locales y el entorno de compilación adecuado; no es una receta universal.

### 13.2 Cambios de código asociados a la sonda

```text
game/shared/gamemovement.cpp
  ↳ definición de portal2_ground_probe (servidor Portal 2)

game/shared/portal/portal_gamemovement.cpp
  ↳ registro de transiciones en CPortalGameMovement::SetGroundEntity

game/server/portal2/portal2_map_compat.cpp
  ↳ CPortal2GroundProbe y g_pPortal2RideTrain

scripts/test-portal2-render.py
  ↳ --ground-probe, comprobación de marcadores y --extra-cvar
```

**Estado de cambios según el registro aportado:** cuatro archivos modificados en working tree, **sin commit** al momento del informe; último HEAD comunicado `35c49232`. Verificar el estado real con `git status` antes de cualquier operación.

### 13.3 Criterios mínimos antes de declarar una mecánica reparada

- Build reproducible sin fallos del componente.
- Prueba automatizada de éxito y prueba negativa.
- Validación manual en el juego cuando sea aplicable.
- Varios intentos, incluyendo extremos y escenarios alternativos.
- Evidencia conservada y análisis de regresiones.
- Si se afirma fidelidad a retail, comparación documentada con el original.

## 14. Hipótesis abiertas y matriz de validación

| ID | Hipótesis / pregunta | Evidencia actual | Siguiente prueba |
|---|---|---|---|
| H-01 | Se pierde suelo durante descenso de llegada | **No reproducido** en una corrida instrumentada | Repeticiones y escenarios variados. |
| H-02 | `BaseVelocity.z` debe igualar `train_vel_z` | **Refutado como requisito universal del port probado**: descenso exitoso con ambos valores distintos | Investigar rutas de transporte y contrastar retail. |
| H-03 | El transporte depende de movimiento de plataforma | **Compatible con la observación**, mecanismo exacto por completar | Trazar relación entre `MOVETYPE_PUSH`, desplazamiento y colisiones. |
| H-04 | Fallo aparece en ascensor de salida | **Sin comprobar** | `--intro2-exit` y otras rutas. |
| H-05 | AudioQueue causa por sí solo el fallo original | **No demostrado**; ajuste de profundidad permitió terminar | Medición de backend, bloqueo y repetición con controles. |
| H-06 | Recursos adicionales resolvieron texturas moradas | **Hipótesis plausible**, no verificada | Comparación A/B de rutas, VPK y shaders. |
| H-07 | Algoritmos coinciden con Portal 2 retail | **No demostrado** | Binarios legales de referencia y pruebas comparables. |

### Próximo experimento recomendado

```bash
python3 scripts/test-portal2-render.py \
  "$HOME/Library/Application Support/Steam/steamapps/common/Portal 2/portal2_arm64_test" \
  --intro2-exit \
  --ground-probe \
  --map sp_a1_intro2 \
  --seconds 120 \
  --extra-cvar snd_audioqueue_depth=32
```

Analizar **por timestamp** las transiciones y separar el ascensor de partida del ascensor de llegada. La disponibilidad y funcionamiento real del flag `--intro2-exit` deben verificarse en la versión del script que se vaya a ejecutar.

## 15. Hoja de ruta para ReSource

ReSource busca modernizar la base técnica de juegos Source antiguos sin introducir cambios accidentales en su jugabilidad.

**Principio de separación:**

- **`main` / ReSource Core:** portabilidad, infraestructura, diagnósticos generales, multimedia modular y correcciones reutilizables.
- **Ramas de compatibilidad por juego:** movimiento y secuencias exclusivas, entidades particulares y adaptadores de contenido.
- **Ramas experimentales:** investigaciones con riesgos, antes de integrarlas a una rama estable.

La integración de cambios reutilizables exige pruebas en más de un escenario, documentación de licencias y ausencia de dependencia involuntaria de recursos propietarios de otro juego. El proyecto de investigación no confiere derechos para redistribuir código, binarios o assets de Valve.

### Prioridades inmediatas

1. Preservar la prueba exitosa del ascensor de llegada y verificar el estado del repositorio.
2. Probar ascensor de salida y transiciones de mapa.
3. Entender el problema del backend AudioQueue antes de fijar workarounds globales.
4. Registrar de manera objetiva los problemas de materiales y rutas VPK.
5. Continuar documentación de portales, geles, VScript y escenas con evidencias.
6. Diseñar FFmpeg modularmente en una rama separada; no mezclarlo con fixes de movimiento.

## 16. Glosario y referencias internas

- **`ALLAZGOS.md`:** registro detallado de investigación; especialmente §§38–40 (instrumentación y patrón de fallo), §45 (hipótesis), §§46–48 (calidad de evidencia) y §60 (plan de observación).
- **`GroundEntity`:** entidad que actúa como suelo del jugador.
- **`BaseVelocity`:** vector de contribución externa que no es sinónimo de velocidad de un tren.
- **`func_tracktrain`:** entidad de plataforma/tren observada en el descenso probado.
- **`MOVETYPE_PUSH`:** movimiento de entidad que puede desplazar otras entidades; revisar implementación concreta antes de asignarle causalidad única.
- **`EHANDLE`:** manejador de referencia a entidades.
- **`VPK`:** formato de empaquetado de recursos usado por Source.
- **`VScript`:** sistema de scripting; estado de compatibilidad sujeto a pruebas.
- **Staging:** entorno aislado preparado para ejecutar pruebas con recursos del juego.
- **`engine.log`:** evidencia principal de la ejecución instrumentada de referencia.

### Registro de futuras investigaciones

Cada nueva sección debe conservar esta plantilla:

```markdown
### [Nombre de mecánica]
**Estado:** PENDIENTE / CÓDIGO-PORT / OBSERVADO-PORT / OBSERVADO-ORIGINAL
**Pregunta:** …
**Entorno y versión:** …
**Archivos / funciones:** …
**Comandos y mapa de prueba:** …
**Evidencia (logs, hashes, capturas):** …
**Resultado observado:** …
**Interpretación / alternativas:** …
**Diferencias conocidas con retail:** …
**Siguiente experimento:** …
```

---

**Resumen del estado actual:** el port experimental de Portal 2 en macOS ARM64 compiló y completó una prueba instrumentada de ascensor de llegada, con contacto continuo entre jugador y tren durante el descenso. Eso prueba un comportamiento **concreto y local**; la reconstrucción integral de la lógica de Portal 2 y su fidelidad al original siguen abiertas.
