# Portal 2: experimento de mapas ARM64 en macOS

## Alcance

La base actual es el motor de este repositorio con su módulo `portal2`
experimental. **No es una recompilación completa del motor ni de la campaña de
Portal 2.** Cargar un mapa y mostrar geometría no valida portales, puzles,
scripts, transiciones, guardados ni todas las entidades del juego.

El 7 de septiembre de 2026 se consiguió mostrar `sp_a1_intro1` con paredes,
suelo, muebles, texturas e iluminación en Apple Silicon. Se verificó la escena
mediante captura de la ventana del juego, no solamente porque el proceso
permaneciera abierto. Aún hay materiales y efectos exclusivos sin implementar.

## Pistola y primera cámara: prueba jugable

La prueba del 7 de septiembre también valida disparos azul/naranja, enlace y
cruce real del jugador entre portales en `sp_a1_intro1`. Valida recoger el cubo
con el manejador de uso, colocarlo sobre el botón y comprobar que la puerta se
abre; al retirarlo, se cierra. Se utilizan las superficies, entidades y conexiones
del BSP original, sin desactivar la validación de superficies ni reemplazar el
mapa por una habitación de demostración.

Para probarlo libremente, abre **`Jugar-Portal2-PortalGun.command`** en el staging.
Tras cargar el mapa y esperar unos 15 segundos de simulación, el lanzador salta
la cinemática del hotel y coloca al jugador en la primera cámara con la pistola
dual. Esto es un **modo de pruebas**, no el comienzo original de la campaña:
normalmente el jugador todavía no tiene la pistola en este mapa.

- WASD: desplazarse; ratón: mirar; espacio: saltar.
- Clic izquierdo/derecho: portal azul/naranja sobre paneles blancos válidos.
- E: recoger o soltar el cubo; colocarlo sobre el botón activa la puerta.
- Z: mantener el zoom; soltar para recuperar el campo de visión normal.
- F6: volver a equipar la pistola; F7: volver a la cámara de pruebas.
- Escape/F10: menú de pausa experimental. En teclados que usan teclas multimedia
  puede ser necesario pulsar Fn junto con F6/F7/F10.

`Jugar-Portal2-Experimental.command` conserva el inicio normal del mapa y añade
F6/F7 como opciones explícitas. No usa el controlador de pruebas automáticas.
La transición técnica del primer ascensor a `sp_a1_intro2` está comprobada; no
están validados aún el recorrido manual completo desde el hotel, los guardados
de campaña ni la continuación jugable de `sp_a1_intro2`.

## Compilar y preparar

Requisitos: herramientas de desarrollo de Xcode, dependencias de compilación
descritas en [la guía principal](COMPILAR_MACOS_ARM64.md), Python 3 con `vpk`,
y `ffmpeg` disponible en PATH, y las instalaciones locales de Portal 2 y Portal.
Portal aporta los recursos
base y el cache de shaders compatible con este renderer.

Desde la raíz de `source`:

```bash
bash scripts/build-macos-arm64.sh portal2
export PORTAL2_STAGE_DIR="$HOME/Library/Application Support/Steam/steamapps/common/Portal 2/portal2_arm64_nueva_prueba"
bash scripts/stage-macos-portal2.sh
open "$PORTAL2_STAGE_DIR/Jugar-Portal2-Experimental.command"
```

La carpeta de destino **no debe existir**. `PORTAL2_DIR`, `PORTAL_DIR` y
`BUILD_DIR` permiten indicar instalaciones alternativas. El script comprueba
que cliente y servidor estén configurados para `portal2`.

Los VPK originales se montan como recursos; no se extraen miles de modelos ni
se reemplazan binarios de Steam. `cfg` y los guardados se escriben en el área
aislada. Las bibliotecas del staging son enlaces al build: recompilar otro juego
en ese mismo build puede cambiar esta prueba. Usa builds separados si necesitas
conservar varios juegos simultáneamente.

Después del cambio de capacidad de esqueletos (128 a 256 huesos), **recompila
todos los módulos con `python3 ./waf build`**. No mezcles bibliotecas antiguas
con nuevas: las estructuras compartidas de huesos y algunos campos de red
cambian de tamaño. El formato antiguo de animación sigue usando su decodificador
previo, pero no se ha repetido aquí una campaña completa de Portal o Half-Life 2.

Los lanzadores usan `-nosoundcachewrite`: permiten leer recursos y caches de las
instalaciones montadas, pero impiden guardar desde este motor caches de audio
incompatibles sobre los del juego original. Las pruebas anteriores a esta opción
podían escribir caches auxiliares en rutas de contenido montadas; esta opción no
revierte los que ya existan. No se reemplazan los VPK originales.

El lanzador abre `sp_a1_intro1`. Para seleccionar otro mapa:

```bash
"$PORTAL2_STAGE_DIR/Jugar-Portal2-Experimental.command" sp_a1_intro2
```

Se puede abrir la consola para ejecutar `noclip` y explorar si la secuencia de
introducción no progresa; esto no sustituye la lógica que falta de Portal 2.
El lanzamiento desde el botón oficial de Steam no se modifica ni se valida aquí.

## Reparaciones que permiten ver 3D

1. **Montaje explícito de VPK.** Este filesystem no monta automáticamente
   `pak01_dir.vpk` al añadir su carpeta. `gameinfo.txt` ahora incluye los VPK
   de Portal 2, sus DLC disponibles y los recursos base de Portal/HL2.
2. **Cache de shaders correspondiente al código.** La cabecera generada actual
   de `lightmappedgeneric_ps20b` requiere 288 combinaciones dinámicas. El cache
   de Portal 2 instalado usa 32 y no es intercambiable. Se extraen los shaders
   del `hl2_misc_dir.vpk` de Portal, comprobando versión 6 y dimensión 288.
   Esta comprobación es un control de compatibilidad parcial, no una garantía
   de que todos los shaders de cualquier versión futura coincidan.
3. **Índices sin signo.** Los identificadores VCS pueden superar `INT_MAX`.
   El cargador ya no divide esos valores como enteros negativos y distingue
   expresamente VCS 5 (ID completo) de VCS 6 (ID dinámico). Se comprueban límites
   antes de acceder a las tablas. Se retiró la selección arbitraria del primer
   shader disponible usada por los parches anteriores.
4. **Adaptaciones locales de materiales.** Algunos modos de detalle de
   `LightmappedGeneric` están excluidos del shader compilado: el experimento
   usa modulación como aproximación visual. En materiales emisivos se desactiva
   el tinte por alfa incompatible. `portal2_material_compat/ADAPTATIONS.txt`
   enumera cada modificación. Los materiales de postprocesado y cuerdas del
   renderer se toman de la base compatible.

Estas adaptaciones de materiales solo se montan en el staging de Portal 2;
no alteran los recursos de Half-Life 2 o Portal. La aritmética VCS 5/6 tiene
pruebas de regresión, pero eso no equivale a una nueva prueba completa de ambos
juegos antiguos.

## Pruebas y diagnóstico

```bash
clang++ -std=c++11 -fsanitize=undefined,address \
  scripts/tests-shadercomboindex.cpp -o /tmp/portal2-shadercombo-test
/tmp/portal2-shadercombo-test

python3 scripts/test-portal2-render.py "$PORTAL2_STAGE_DIR" --screenshot

python3 scripts/test-portal2-render.py "$PORTAL2_STAGE_DIR" \
  --gameplay --seconds 90 --screenshot

python3 scripts/test-portal2-gameplay-assets.py \
  "$HOME/Library/Application Support/Steam/steamapps/common/Portal" \
  "$HOME/Library/Application Support/Steam/steamapps/common/Portal 2" \
  --stage "$PORTAL2_STAGE_DIR"

python3 scripts/test-portal2-render.py "$PORTAL2_STAGE_DIR" \
  --intro-scenes --seconds 75 --screenshot

python3 scripts/test-portal2-render.py "$PORTAL2_STAGE_DIR" \
  --elevator-transition --seconds 30
```

La prueba abre el juego, espera 20 segundos y lo cierra. Guarda `engine.log`,
`stdout.log` y la captura `map.png` en una subcarpeta de `diagnostics`.
`MAP LOAD PASS` exige que el proceso siga vivo, que el servidor active el mapa
y que no aparezcan los errores de índices de shaders detectados. **Hay que
inspeccionar la imagen**: no es una prueba automática de jugabilidad o calidad
visual. La captura requiere autorización de grabación de pantalla de macOS.

`GAMEPLAY PASS` exige además estas comprobaciones en el servidor:

- Outputs antiguos separados por comas y modernos separados por ESC, conservando
  comas dentro de parámetros de scripts.
- Cancelación de eventos durante su propio callback sin liberar el evento activo.
- Filtro de colisión seguro mientras un duplicado físico del portal se construye.
- Ataques con la pistola real, portales recíprocamente enlazados y un cruce
  observado dentro de `CProp_Portal::TeleportTouchingEntity`.
- Recogida del cubo y apertura/cierre de la puerta por el botón y los relés del mapa.
- Secuencia `idle_carrying` al sostener el cubo; zoom a 35 grados y retorno al FOV
  normal. Los ataques y el zoom entran por `PlayerRunCommand`, no por llamadas
  directas a `PrimaryAttack`/`SecondaryAttack`.
- Ausencia de fallos de lectura de los tres PCF compatibles del arma/portales
  y de mensajes `EntityPortalled` con longitud incorrecta.

La prueba jugable guarda además `gun-blue.png`, `gun-orange.png`,
`gun-carrying.png` y `gun-zoom.png`. En un arranque frío la carga puede consumir
más de 20 segundos: usa 90 segundos para alcanzar todas las comprobaciones.
La prueba de recursos compara modelos/texturas con Portal 2 y los tres PCF con
Portal 1 byte a byte, además de verificar los hashes de `ORIGIN.txt`. Sin
`--stage` genera y elimina exclusivamente una carpeta temporal de prueba.

La prueba coloca automáticamente al jugador y al cubo en posiciones de ensayo;
no es un recorrido manual completo. Durante ella se ignoran los controles del
usuario para no alterar los resultados. El lanzador jugable no activa esa opción.
`INTRO SCENES PASS` exige terminar las cuatro escenas encadenadas del inicio de
la cámara de relajación y que no se registren fallos de carga de sus voces.
Esto no comprueba subjetivamente la mezcla de audio ni todos los diálogos.

Registro de referencia de pistola y puzle: `diagnostics/20260907-160713-450706`
en el staging `portal2_arm64_render_20260907`.
La cadena de escenas y carga de voces pasó en `diagnostics/20260907-160845-652007`.

`test-macos-portal2.sh` prueba únicamente arranque y `+quit`; un timeout ahora
se considera fallo, no éxito.

Si reaparece la pantalla morada, comprueba las rutas de `gameinfo.txt` y que
`renderer_compat/shaders` provenga del cache esperado. No enlaces los shaders
de `Portal 2/platform` al renderer de esta rama. No ejecutes los antiguos
parches de extracción sobre una carpeta reparada: `prepare-portal2-map.sh`
detecta ahora el staging nuevo y evita ese paso.

El overlay ahora sustituye de forma aislada `SolidEnergy` y `Black`, y elimina
los proxies exclusivos `FizzlerVortex`, `LightedFloorButton` y `LightedMouth`.
Son aproximaciones estáticas: reducen los avisos y conservan textura/transparencia,
pero no recrean la animación de flujo ni la iluminación dinámica original. Siguen
sin ser legibles los cubemaps HDR de Portal 2; no se fuerzan como una textura
ordinaria porque hacerlo ocultaría un fallo de formato y puede causar cierres.

Algunos objetos pueden aparecer blancos o con iluminación incorrecta.
`--no-prop-lighting` es solo una opción diagnóstica para comparar la iluminación
de objetos; no se aplica al lanzador predeterminado.

## Menú, idioma y zoom del escenario

`F10` o `Escape` abren el menú de pausa del cliente Portal 2. Sus textos se
obtienen de catálogos UTF-16 dentro de `portal2_gameplay_compat/resource`, no de
los archivos de Steam. Con `portal2_ui_language auto` toma `cl_language`; también
se puede elegir explícitamente, por ejemplo `portal2_language spanish`. El botón
o la tecla `5` rota entre inglés, español, portugués brasileño, francés, alemán,
italiano, ruso y polaco. Esto localiza el menú de compatibilidad, no traduce los
diálogos, subtítulos ni todos los menús originales de Portal 2.

El lanzador aislado enlaza `Z` a `+zoom` y acepta los nombres heredados
`+zoom_in` y `+zoom_out`. El FOV sólo se modifica cuando el módulo se compila con
`PORTAL2`; Portal y los demás objetivos conservan sus controles sin cambios.

## Adaptaciones de jugabilidad y sus límites

`portal2_gameplay_compat` contiene los modelos y materiales Studio 49 de la
pistola y los marcos de portal de Portal 2 instalados localmente. El código de
colocación y teletransporte sigue siendo la implementación compatible de Portal,
porque no se ha reconstruido el DLL propietario de Portal 2. Los materiales de
la pistola eliminan localmente el proxy `LightedMouth`, que este renderer no
registra: conserva sus texturas y autoiluminación sin el aviso por cuadro.
`ORIGIN.txt` registra hashes y procedencia de los recursos.

La reparación del 9 de septiembre elimina el relleno opaco añadido sobre los
portales y conserva sus materiales `PortalRefract`. El contorno auxiliar es
elíptico y exclusivo de `PORTAL2`; no sustituye ni demuestra por sí mismo la
vista recursiva del otro lado. Su vida útil sigue a la entidad del portal, sin
el temporizador de 999 segundos que antes lo hacía desaparecer.

El lector de partículas admite DMX binario 2, no el binario 5 de Portal 2.
Había un error en el preparador: validaba los PCF de Portal 1, pero extraía los
de Portal 2. Ahora `portalgun.pcf`, `portal_projectile.pcf` y `portals.pcf` se
extraen realmente de Portal 1 y sus efectos cargan en la prueba. Esto no hace
compatibles todas las demás partículas de Portal 2.

La pistola Studio 49 no recibe los bodygroups de los chips del modelo antiguo:
ese grupo representa PotatOS en Portal 2. La actividad `ACT_VM_PICKUP_IDLE`
se registra solo para `PORTAL2`. Como el controlador de recogida evita el
`ItemPostFrame` del arma, su reposo con pinzas abiertas avanza desde
`ItemPreFrame` tanto en servidor como en cliente predictivo. Los objetivos
antiguos conservan su camino de animación.

`EntityPortalled` declara ahora los 32 bytes que realmente envía (dos handles
de 32 bits y seis floats), en lugar de depender de `sizeof(long)`, que daba
40 bytes en ARM64. Esta corrección conserva el formato de red de 32 bits.

Verificación del 9 de septiembre: compilación ARM64 y `GAMEPLAY PASS` durante
90 segundos en `portal2_arm64_gun_repair_20260909`, diagnóstico
`20260909-135051-614869`. También pasa la comprobación de 116 hashes de recursos.
Las capturas muestran la pistola, el cubo sostenido, el zoom y el borde con
partículas. **La vista del otro lado del portal todavía no aparece en estas
capturas**: el éxito del cruce no implica que el render recursivo esté reparado.
El registro confirma HDR entero activo y `mat_fullbright=0`; la sala sigue
siendo tenue, sin forzar iluminación plana para ocultarlo.

La prueba inyecta botones en el comando del servidor; no sustituye una sesión
manual para comprobar todos los controles y la predicción cliente. No se
recompilaron ni ejecutaron Portal 1 y Half-Life 2 en esta última verificación.
Los cambios de animación y el auxiliar de diagnóstico quedan delimitados por
`PORTAL2` para no añadir esa dependencia a los objetivos antiguos.

## Seguimiento: vista de portales y Chell (9 de septiembre)

En `portal2_arm64_portalview_20260909`, la captura `gun-zoom.png` del diagnóstico
`20260909-213413-519729` ya muestra la sala de destino y su puerta dentro del
portal azul. Esa ejecución termina con `GAMEPLAY PASS`, sin el relleno de
diagnóstico. Esto actualiza el resultado visual pendiente descrito arriba,
pero no certifica todos los mapas ni todos los casos de recursión.

La máscara con la separación antigua de 0,25 unidades quedaba rechazada por
profundidad en los paneles probados de `sp_a1_intro1`. Las lecturas de píxeles
antes y después de la escena remota permitieron localizar ese bloqueo.
`DrawStencilMask` usa ahora una separación de 4 unidades solo para `PORTAL2`:
mantiene la prueba de profundidad, el plano físico de teletransporte y el
camino antiguo de Portal 1. Es un ajuste del renderer de compatibilidad;
no una corrección general de la geometría de todos los BSP. Se retiraron las
sondas que pintaban el stencil de magenta y la prueba no activa `developer 1`
por defecto.

El jugador carga `models/player/chell/player.mdl` de Portal 2 junto con sus
animaciones y materiales. La comprobación de recursos valida 143 hashes,
incluidos seis archivos del modelo/animaciones del jugador. La prueba guarda
`player-model.png` en tercera persona, además de vistas de ambos portales.

La repetición limpia de 90 segundos, `20260909-213748-155580`, termina con
`GAMEPLAY PASS`. Se revisaron `portal-view-blue.png`, `portal-view-orange.png`
y `player-model.png`: ambas vistas muestran geometría del destino y Chell
usa el modelo esperado. Siguen pasando cruce físico, recogida, animación de
reposo con cubo, botón/puerta y zoom. No se han vuelto a ejecutar los otros
juegos ni se ha certificado una campaña completa con este ajuste.

`--portal-texture` sirve únicamente para comparar el camino alternativo:
durante esta investigación se cerró al enlazar los portales. Mantener
`r_portal_use_stencils 1`; no recomendar el camino de texturas como reparación.

## Ascensor: llegada a `sp_a1_intro2` (10 de septiembre)

La grabación del 9 de septiembre a las 21:57 muestra una transición que carga
el segundo mapa, pero deja al jugador en su sala técnica de inicio. El BSP
coloca allí `info_player_start` y un trigger que llama `OnPostTransition()`.
El adaptador anterior solo atendía scripts de `sp_a1_intro1`: no trasladaba
al jugador a `@arrival_teleport`. Registrar el landmark de entrada por sí solo
no soluciona esa llamada pendiente.

El adaptador de llegada, limitado a `sp_a1_intro2` y a `@transition_script`,
usa el punto de llegada y los nodos de vía del BSP original. Sustituye el
`MoveToPathNode` no soportado por el movimiento real del `func_tracktrain`
antiguo; conserva la colisión que transporta al jugador. Abre el relé de
puertas cuando el coche se detiene cerca del nodo terminal, no por un
temporizador que pueda abrirlo en mitad del descenso. En este coche de
`wheels=50`, el motor antiguo detiene el centro unos 25,5 puntos antes del
nodo final; se comprueba distancia menor de 32 y velocidad menor de 1.

La prueba `--elevator-transition --seconds 90 --screenshot` ahora exige
`PORTAL2_ARRIVAL ride=PASS` y `exit=PASS`, además de las dos cargas de mapa.
Solo con ese argumento automatiza tres segundos de avance para comprobar
que el jugador puede salir: las partidas normales conservan el control.
Guarda `elevator-arrival-start.png`, `elevator-arrival-end.png` y
`elevator-exit.png`. La repetición `20260910-023720-752576` registra llegada
a z=-74 y salida de la cabina hasta x=-632,6, subiendo por la escalera.

**Pendiente visual:** siguen los materiales multicolores de partes del segundo
mapa. El arreglo de llegada no los oculta ni constituye una reparación de
shaders. Tampoco implementa persistencia general entre landmarks, el VScript
completo del segundo mapa ni certifica la campaña. No modifica el renderer
de portales ni el código de trenes compartido con los juegos antiguos.

El 10 de septiembre se extendió el adaptador de salida a toda la cadena del
coche intro (intro2→intro3→intro4→intro5): cada mapa usa el mismo carro,
teletransporte y relés `ReadyForTransition`/`TransitionFromMap`, y el destino se
resuelve desde una tabla en lugar de estar fijo. Antes `sp_a1_intro3` registraba
`PORTAL2_INTRO unsupported script` y su ascensor de salida nunca arrancaba.
La llegada también cubre ahora `sp_a1_intro4`/`sp_a1_intro5` (mismo rig). La
cadena termina de forma explícita y avisando al llegar a un mapa sin destino
registrado; sigue sin implementarse la persistencia general de campaña.

## Bink en ARM64

El objetivo de macOS ahora compila `bin/libvideo_bink.dylib` para ARM64 con el
decodificador nativo de FFmpeg. `video_services` lo carga al solicitar Bink y
FFmpeg reconoce los `.bik` instalados de Portal 2; no se copia ni se intenta
cargar `portal2/bin/osx32/libbinkmachox86.dylib`, que es i386. La prueba deja
en el log `LoadLibrary: pModule: video_bink`.

La librería depende del paquete local Homebrew `ffmpeg`; el script de compilación
añade automáticamente su `pkgconfig`. Este módulo implementa vídeo en material,
no todavía el panel cliente `vgui_movie_display` ni audio Bink. Mientras ese
panel no se implemente, `logic_playmovie` conserva el flujo de I/O del mapa y
termina su salida de forma controlada. El SDK Bink oficial ofrece bibliotecas
ARM, pero su SDK se entrega a clientes licenciados; esta rama no incorpora
binarios propietarios de terceros.

Se comprueba con una etapa aislada mediante:

```sh
python3 scripts/test-portal2-render.py \
  "/Users/jesus/Library/Application Support/Steam/steamapps/common/Portal 2/portal2_arm64_bink_verify_20260908" \
  --bink --seconds 15
```

El resultado correcto es `BINK PASS` y una línea `PORTAL2_BINK opened` en el
registro de diagnóstico. Es una prueba de apertura y decodificación de vídeo,
no una afirmación de que todas las pantallas de vídeo del juego ya se muestren.

Los preparadores de la introducción hacen tres trabajos distintos:

1. `prepare-portal2-intro-script.py` extrae 49 entradas de las tablas de escenas
   locales y las convierte a KeyValues, sin ejecutar Squirrel.
2. `prepare-portal2-scenes.py` adapta las cabeceras/resúmenes VSIF 3 a VSIF 2
   conservando los VCD binarios versión 4, sus tiempos y eventos.
3. `prepare-portal2-intro-audio.py` localiza las voces usadas por esas escenas y
   por el BSP, y convierte MP3 con extensión `.wav` a PCM16/44100 mediante ffmpeg.
   Se preparan 63 archivos; `INTRO_AUDIO_ORIGIN.txt` registra su procedencia.

El servidor implementa un adaptador nativo **limitado a `sp_a1_intro1`**, no una
máquina virtual VScript general. Ejecuta callbacks de sus escenas, cancelaciones,
relés, cámaras, salto/agachado y el marco móvil del contenedor. Los nags de puerta
se recorren en orden en lugar de sortearse; las colas de voces no reproducen toda
la prioridad de Squirrel. Las llamadas desconocidas se registran como
`PORTAL2_INTRO unsupported script`, no se consideran exitosas silenciosamente.

En la rama dedicada `codex/portal2-compat`, `logic_playmovie` conserva su salida
`OnPlaybackFinished` cuando Bink no está disponible, para no bloquear el mapa;
`env_instructor_hint` expone sus avisos como texto y se registran los anclajes
`vgui_movie_display`, `info_game_event_proxy` e `info_landmark_exit`. Esto permite
que el I/O del prólogo continúe, pero todavía no reproduce vídeos Bink ni el HUD
visual original de las pistas.

Siguen pendientes las películas de introducción/ascensor, `env_instructor_hint`,
parte de la presentación de Wheatley y otras entidades. La introducción ya no
salta a `sp_a1_intro2` al abrir la primera puerta: espera los relés reales de
`ReadyForTransition`/`TransitionFromMap` del elevador. Si la cadena final de
teletransporte falta, el adaptador conserva el movimiento real del
`func_tracktrain` y, tras activar los mismos relés de salida, completa el cambio
a `sp_a1_intro2`. La prueba `--elevator-transition` verifica ese recorrido de
I/O y la segunda activación del servidor. No preserva estado de campaña mediante
landmarks ni demuestra aún un recorrido manual completo de principio a fin. No
presentar este experimento como «toda la programación de Portal 2».

Si vuelve un cierre al animar el contenedor, comprueba que todo el build usa
256 huesos y el decodificador `STUDIO_FRAMEANIM` de Studio 49. Si la pistola se
equipa pero no coloca portales, prueba paneles blancos: el vidrio, los modelos
y las caras `SURF_NOPORTAL` siguen rechazándose deliberadamente. Si una voz no
carga, vuelve a ejecutar el preparador de audio sobre el overlay, sin renombrar
ni sobrescribir archivos dentro de los VPK de Steam.

### Botones de pedestal de `sp_a1_intro2` (10 de septiembre de 2026)

La adaptación de `prop_button` esperaba un callback `ReachedEndOfSequence`
que `CDynamicProp` no invoca. Ahora la pulsación emite `OnPressed` una sola
vez y un context think independiente rearma el botón usando `Delay`, sin
sustituir el think de animación. `Press` usa despacho virtual y se admiten
`Lock`/`Unlock`, que los relés originales del mapa necesitan para seleccionar
un portal azul y desbloquear los otros botones. El modelo se establece antes
del precache y de crear la física. Los cambios de entidades quedan en
`game/server/portal2`; no se modifica `CDynamicProp` compartido ni el BSP.

Prueba reproducible, con el juego cerrado y tras compilar:

```sh
python3 scripts/test-portal2-render.py \
  "/Users/jesus/Library/Application Support/Steam/steamapps/common/Portal 2/portal2_arm64_portalview_20260909" \
  --buttons --map sp_a1_intro2 --seconds 60 --screenshot
```

La prueba selecciona 1 → 2 → 3 → 1, comprueba rearme y bloqueo del botón
seleccionado y que únicamente su portal azul esté activo. El primer botón
y su reutilización pasan por `PlayerUse`; los otros dos por la entrada
`Press`. Es una prueba acotada de I/O, no una certificación de que todo el
puzle pueda completarse ni de que los materiales del mapa sean correctos.
El modo de prueba está aislado del fixture de la portal gun de `sp_a1_intro1`.

Resultado: compilación correcta y `BUTTONS PASS` en
`diagnostics/20260910-025330-537808` del stage anterior, con cuatro selecciones
correctas y el proceso vivo al terminar los 60 segundos de observación.

### Paneles `func_detail` y sonido de recogida (10 de septiembre de 2026)

Dos reportes de juego concurrentes con la cadena intro: algunos paneles blancos
por los que sí debería colocar portales la rechazaban (y dejaban atrapado al
jugador), y la portal gun no sonaba al recogerla.

Los paneles blancos de Portal 2 casi siempre se compilan a `func_detail`, y el
mask de colocación `MASK_SHOT_PORTAL` no incluía `CONTENTS_DETAIL`: los probes
centro/esquinas de `portal_placement.cpp` atravesaban el panel fino y respondían
"sin superficie", por lo que solo los paneles respaldados por world brush (o por
otra geometría a menos de esas unidades) aceptaban el portal. Solución: añadir
`CONTENTS_DETAIL` a `MASK_SHOT_PORTAL` (`public/bspflags.h`, solo lo usa el código
de portal) y a los probes de esquina/pared/piso de `portal_placement.cpp`. El
vidrio, los modelos y `SURF_NOPORTAL` siguen rechazándose deliberadamente; si el
jugador encuentra otro panel que no acepte, `sv_portal_placement_debug 1` imprime
el motivo exacto.

La entrada `Weapon_Portalgun.powerup` de Portal 2 depende de `operator_stacks`
(`soundentry_version 2`) que este motor no puede parsear. Ahora el sonido de
recogida se toca desde el wav directo `weapon_ambient/wpn_portalgun_activation_01.wav`
(en el VPK de Portal 2), precargado con `PrecacheSound` y emitido en
`PlayPickupSound()` desde `SetCanFirePortal1` (equip/impulse/canales de juego:
llegada a un mapa intro re-equipa el arma) y desde `OnPickedUp` (recogida
física, como el pedestal de `sp_a1_intro4`). Se declara en las dos clases,
`CWeaponPortalgun` (server) y `C_WeaponPortalgun` (client).

Recoger una portal gun dejada en el suelo (drop y re-recogida) devolvía un arma
con los valores restrictivos del mapa o con los defaults de instancia (solo
azul). En `OnPickedUp`, si `portal2_resume_portalgun` está activo, se restaura
un arma azul usable (`SetCanFirePortal1`, que además toca el sonido una sola
vez). No se fuerza naranja: es la variante correcta de la cadena intro (ver más
abajo).

#### Persistencia del arma entre mapas y salidas de la cadena (10 de septiembre de 2026)

Hay tres variantes de portal gun en los mapas (la de prueba **solo-azul**, el
arma dual estándar y la de la patata GLaDOS). La cadena intro obtiene el arma
por primera vez en `sp_a1_intro3` y es la **solo-azul**; de `sp_a1_intro4` en
adelante solo existe la portal gun azul. El modelo queda como lo coloque el
mapa; `portal2_equip_portalgun` (F6 / playground) sigue siendo el **dual**
experimental, y con el argumento `blue` restaura la variante solo-azul.

`portal2_resume_portalgun` tiene default `1` (se puede apagar con `0`): la
llegada a cualquier mapa intro (intro2..intro5) re-equipa la portal gun
**solo-azul** (`portal2_equip_portalgun blue`) si el flag está activo. Verificado
en la prueba automatizada: `sp_a1_intro4` y `sp_a1_intro5` llegan con
`equipped=1 blue=1 orange=0` y `gun_owned=1 gun_active=weapon_portalgun`, y la
cadena completa intro4→intro5 conserva el arma. El reporter de rig
(`PORTAL2_RIG map=…`) y el estado de arma de llegada son los medios para
diagnosticar la cadena.

El adaptador de salida (`CPortal2IntroDeparture`) dependía de un
`ReadyForTransition` del VScript que los mapas de la cadena pierden, dejando el
carro de salida parado para siempre y sin cambio de nivel (el reporte final de
`sp_a1_intro4`). Ahora vigila el viaje: cuando el carro aparca cerca de
`@exit_teleport`, o tras 25 s de tope acotado, dispara los relays de transición
(`@transition_from_map`/`@transition_with_survey`) y encola `ChangeLevel` al
siguiente mapa de la tabla (`intro2`→`intro3`→`intro4`→`intro5`). `sp_a1_intro5`
no tiene salida, correctamente. Prueba `--intro2-exit` (ahora sirve para
cualquier mapa intro2..intro5): `sp_a1_intro2`→`sp_a1_intro3` e
`sp_a1_intro4`→`sp_a1_intro5` pasan con cambio de nivel y llegada con arma.

#### Sincronía de sonidos con los triggers (10 de septiembre de 2026)

macOS usa el driver AudioQueue (`engine/audio/snd_dev_mac_audioqueue.cpp`); su
`GetOutputPosition` reporta la posición de *escritura* (`buffersSent`), no la de
*reproducción*, así que el motor cree que el hardware va tan lejos como lo que
ha enviado y la latencia audible real es `snd_mixahead` + la profundidad de la
cola del AudioQueue. Con el vanilla (`snd_mixahead 0.1` ≈ 100 ms) y la cola
manteniendo ~16 buffers de 1024 B (≈ 93 ms), cada sonido sonaba ~200 ms después
del evento que lo disparó ("desfasado" con los triggers, botones y puertas).

Correcciones:

- `snd_mixahead` pasa a default `0.03` y se acota la programación de mezcla con
  `portal2_snd_mixahead_cap` (default `0.03`): aunque el `snd_mixahead` archivado
  de una sesión anterior guardara `0.1`, la mezcla nunca va más adelante de
  ~30 ms. `S_Update`/`S_ExtraUpdate` usan `GetSoundMixAhead()` en vez del valor
  crudo del ConVar.
- La profundidad de la cola nueva es `snd_audioqueue_depth` (default `8` buffers
  de 256 frames ≈ 46 ms, mínimo 4); antes el bucle llenaba hasta 16. Perilla de
  seguridad: si la máquina da hitches largos, subirla (más latencia, menos
  cortes); si aún se siente tarde, bajarla o bajar `portal2_snd_mixahead_cap`.
- `snd_audioqueue_debug 1` imprime `AUDIOQUEUE depth=N Xms mixahead=…` cada ~1024
  pasadas; un agotamiento de la cola imprime `AUDIOQUEUE STARVED, restarting`
  (el queue se reinicia solo). Medido en la prueba intro1: `depth=8 46.4ms`,
  `mixahead=0.030` estable en el juego, latencia audible ≈ 76 ms; un solo
  `STARVED` al cargar (hitch), que se recupera solo. `GAMEPLAY PASS`.

El runner `test-portal2-render.py` acepta ahora `--extra-cvar NAME=VALUE`
(repetible) para inyectar ConVars al arranque, p. ej.
`--extra-cvar snd_audioqueue_debug=1`.

#### Física dinámica (10 de septiembre de 2026)

El port trae la interfaz de física del motor reimplementada (`libvphysics.dylib`,
arm64, ~3.1 MB): entornos, cuerpos rígidos, constraints, colisiones, materiales,
motion controllers y vehículos. Se carga en el arranque (`LoadLibrary:
vphysics.dylib`). La prueba `--phys-probe` lo verifica de verdad: con
`-portal2_phys_probe`, el fixture crea un `prop_physics` del cubo de peso de
Portal 2 (`models/props_underground/underground_weighted_cube.mdl`) 96 u por
encima del jugador y loguea cuerpo/posición/velocidad cada segundo.

Resultado en `sp_a1_intro2`: `body=1 solid=1`, el cubo cae desde la altura del
jugador hasta el suelo real (z −77.5), se frena y queda quieto (velocidad 0) →
`PORTAL2_PHYS PASS settled z=-77.5`. La colisión con el suelo y la gravedad
funcionan.

Limitación conocida del contenido, no del simulador: 403 props estáticos salen
como `SOLID_VPHYSICS static prop with no vphysics model!` (esos modelos no
tienen vcollide y quedan fantasmas), y `Legacy game DLL may not support terrain
vphysics collisions with this BSP!` avisa de que el terreno puede no resolver
todas las colisiones vphysics contra el BSP.

#### Bóveda de sp_a1_intro1: sin repeticiones superpuestas (10 de septiembre de 2026)

El mapa enruta tanto el final del cine `p2_intro` como el trigger de volver a la
cama (`enter_chamber_trigger`) por `relay_start_map` → `@glados
RunScriptCode GladosPlayVcd(0)`, que arranca la cadena de la bóveda
(`PreHub01RelaxationVaultIntro01..04`, la narración del announcer). El resultado
era que la cadena se disparaba dos veces y, como el adaptador nativo no tiene
cola de escenas (a diferencia del SceneManager vanilla `glados.nut` con `queue`),
la segunda pasada arrancaba mientras la primera aún hablaba: voces del announcer
superpuestas y completos entrecruzados en `sp_a1_intro1`.

Fix en `portal2_intro_script.cpp`: la cadena de la bóveda se marca como iniciada
en el primer `PlayScene` de `PreHub01RelaxationVaultIntro01` y cualquier
reintento posterior se ignora con
`PORTAL2_INTRO vault chain replay skipped (already started)`. La cadena queda
así: `01 -> 02 -> 03 -> 04` limpio, `overlap=0`, una sola vez,
`GladosRelaxationVaultPowerUp()` al final.

Traza del antes/después (diagnostics): `previous-engine.log` muestra dos cadenas
intercaladas; `engine.log` muestra una sola. Debug opcional:
`portal2_intro_script_debug 1` (FCVAR_CHEAT) loguea cada llamada de script, cada
`play`/`complete` y el solapamiento.

#### Puerta de cámara que "se abre y luego se cierra" (11 de septiembre de 2026)

Las puertas de cámara `prop_testchamber_door` de Portal 2 utilizan el modelo
`models/props/portal_door_combined.mdl` y las secuencias `open`, `idleopen`,
`close`, `idleclose`. El `CDynamicProp::AnimThink` de este motor revierte
cualquier entidad a `m_iszDefaultAnim` al terminar su animación; la puerta
arrancaba con `m_iszDefaultAnim="idleclose"` y `ReachedEndOfSequence()` nunca se
invocaba, así que al acabar la secuencia `open` la puerta se deslizaba de nuevo
a `idleclose` por sí sola: el síntoma exacto "se abre y luego se cierra" sin
tocar nada.

Fix en `prop_testchamber_door.cpp`: `Open()`/`Close()` ahora fijan un context
think (`SequenceCompleteThink`, en `s_pSeqThinkContext`) que espera a que
la secuencia `open` o `close` llegue a su ciclo final (o sea looping) y entonces
dispara `ReachedEndOfSequence()` → pasa a `idleopen`/`idleclose`, fija
`m_iszDefaultAnim` y lanza `OnFullyOpen`/`OnFullyClosed`. El usejo de context
think (no `SetThink`) evita pisar el `AnimThink` de la base, igual que el patrón
ya usado en `prop_button_base.cpp` ("CDynamicProp does not call
ReachedEndOfSequence"). Esas salidas importan: en `sp_a1_intro3`,
`OnFullyOpen` de `door_0` dispara `fire_rotating_portalgun_tca01` (el giro del
pedestal) y `OnFullyClosed` desactiva los physics clips.

Verificación indirecta: antes del fix el test `--gameplay` fallaba su última
comprobación (cámara en el portal naranja a los ~70 s) porque la puerta
`door_1` de la cámara de `sp_a1_intro1` se recerraba sobre el portal colocado y
lo hacía fizzle; tras el fix la puerta queda abierta, el portal sobrevive y el
test completo vuelve a dar GAMEPLAY PASS.

#### Rango de colocación del portalgun y sonido de agarre (11 de septiembre de 2026)

El gun de Portal 1 trazaba la colocación hasta `MAX_TRACE_LENGTH` (~56.756
unidades, prácticamente sin límite; `weapon_portalgun_shared.cpp`). Nuevo ConVar
`sv_portal_placement_max_range` (por defecto 5000): `TraceFirePortal` y el ray
de puertas rotatorias usan `clamp_range(m_fMaxRange1)`, así que disparar contra
una superficie más lejos que el límite deja de alcanzar el muro → el portal hace
fizzle "passthrough" como en Portal 2 ("no puedes ponerlo tan lejos"). No hace
falta tocar la parte CLIENT (el crosshair verde/rojo se calcula con la misma
traza del servidor vía `m_fCanPlacePortal1OnThisSurface`).

Sonido de adquisición: `SetCanFirePortal2()` emitía el script
`Weapon_Portalgun.powerup`, que este motor no puede parsear (operator_stacks) y
quedaba mudo; ahora usa el mismo WAV directo que `PlayPickupSound()`
(`weapon_ambient/wpn_portalgun_activation_01.wav`), así el "chirrido" P2 se oye
al coger la mejora. El sonido de apertura del portal (`Portal.open_blue`,
`Portal.open_red`) ya existía en `CProp_Portal::NewLocation()` y se verifica que
su soundscript P2 carga sin errores.

Pendiente conocido: la adaptación de tamaño de Portal 2 (portales que encogen
para entrar en huecos/círculos pequeños) NO está implementada y no es un fallo
transitorio: la colocación usa un portal de tamaño fijo (64×108 u,
`PORTAL_HALF_WIDTH/HEIGHT`) y `FitPortalOnSurface` devuelve `CANT_FIT` (efecto
"nofit") cuando la superficie es más pequeña que el portal. Es un subsistema
nuevo (escala de malla, colisión, teleport y render) que no se abordó en esta
sesión.
