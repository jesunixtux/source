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
  --gameplay --seconds 60 --screenshot

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

En ARM64 la pasada `PortalRefract` de etapa 2 puede quedar negra aunque el
portal exista y teletransporte correctamente. La preparación adapta sólo
`portalstaticoverlay_1/2` a `PortalStaticOverlay`, conservando la máscara y el
color azul/naranja. Es una superficie estática de compatibilidad: prioriza que
el portal se vea y pueda usarse sobre la refracción animada original.

Las partículas PCF de Portal y Portal 2 todavía no se deserializan con el
lector de partículas presente en esta rama; por ello el disparo, la creación y
el cruce de portales funcionan, pero faltan estelas/chispas y se registran como
partículas desconocidas. No se debe presentar el DMX heredado como solución:
la verificación de arranque lo detecta explícitamente como incompatible.

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
