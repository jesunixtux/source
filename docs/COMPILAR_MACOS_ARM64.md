# Compilar Source Engine para macOS ARM64

Este tutorial cubre el flujo probado para Half-Life 2 y Portal, además del modo
experimental de Left 4 Dead, en un Mac Apple Silicon. Los comandos se ejecutan desde la raíz del repositorio `source/`, salvo
cuando se indique otra carpeta.

## 1. Requisitos

- macOS 13 o posterior.
- Xcode o Xcode Command Line Tools.
- Homebrew para ARM64 en `/opt/homebrew`.
- Python 3.9 o posterior.
- Una instalación legal de Half-Life 2, Portal o Left 4 Dead en Steam.

Comprueba las herramientas:

```bash
uname -m
xcode-select -p
python3 --version
brew --version
```

`uname -m` debe responder `arm64`.

Instala las dependencias:

```bash
brew install sdl2 sdl2-compat sdl3 freetype fontconfig jpeg libpng \
  libcurl zlib openal-soft gettext pkgconf ffmpeg
```

Dependencias opcionales:

```bash
brew install opus libedit
```

`opus` habilita el códec de voz con `--enable-opus`; `libedit` se usa en algunas
configuraciones de servidor dedicado.

## 2. Preparar el repositorio

Si el repositorio todavía no está clonado:

```bash
git clone --recurse-submodules URL_DEL_REPOSITORIO
cd source
```

Si ya existe, inicializa sus submódulos una vez:

```bash
git submodule update --init --recursive
```

No hace falta instalar Rosetta ni pasar `--arch=arm64`. En Apple Silicon, Clang
produce ARM64 por defecto. Esta versión de WAF no reconoce la opción `--arch`.

Para Portal 2 se usa un staging separado basado en el mismo principio que
`PortalNXSideLoader`: los recursos originales se montan mediante rutas externas,
sin modificar `game.zip` ni la instalación de Steam.

## 3. Elegir el juego

El argumento del script decide qué implementaciones de `client` y `server` se
compilan:

```text
./scripts/build-macos-arm64.sh portal
./scripts/build-macos-arm64.sh hl2
./scripts/build-macos-arm64.sh stanley   # experimental, usa ABI Portal
./scripts/build-macos-arm64.sh portal2   # experimental, usa recursos Portal 2
```

Al cambiar de juego, ejecuta otra vez el script completo. WAF reconfigurará el
árbol antes de compilar. No copies los módulos compilados para Portal dentro de
Half-Life 2 ni a la inversa.

La alternativa equivalente sin el script es:

```bash
export PATH="/opt/homebrew/bin:/opt/homebrew/sbin:$PATH"
export PKG_CONFIG_PATH="/opt/homebrew/opt/jpeg/lib/pkgconfig:/opt/homebrew/opt/openal-soft/lib/pkgconfig:$PKG_CONFIG_PATH"
python3 ./waf configure -T debug --disable-warns --build-games=portal
python3 ./waf build
```

Cambia `portal` por `hl2` cuando corresponda.

## 4. Tipos de compilación

La compilación predeterminada es `debug`:

```bash
./scripts/build-macos-arm64.sh portal
```

Para jugar y medir rendimiento, usa `release`:

```bash
BUILD_TYPE=release ./scripts/build-macos-arm64.sh portal
```

Para habilitar Opus durante la configuración:

```bash
./scripts/build-macos-arm64.sh portal --enable-opus
```

Durante el desarrollo se pueden recompilar solo módulos concretos después de la
configuración inicial:

```bash
python3 ./waf build --targets=togl,shaderapidx9,stdshader_dx9
```

Si se modificó código de `game/client` o `game/server`, ejecuta el build completo
para evitar dejar módulos desactualizados.

## 5. Verificar el resultado

```bash
file build/launcher_main/hl2_launcher
file build/engine/libengine.dylib
file build/game/client/libclient.dylib
file build/game/server/libserver.dylib
rg '^GAMES' build/c4che/game/client_cache.py build/c4che/game/server_cache.py
```

Todos los binarios deben indicar `arm64`. Los dos valores `GAMES` deben ser
`'portal'`, `'portal2'` o `'hl2'`, según la compilación elegida.

## 6. Desplegar Portal

Con la instalación estándar de Steam:

```bash
./scripts/build-macos-arm64.sh portal
./scripts/deploy-macos-portal.sh
```

Con otra biblioteca de Steam:

```bash
PORTAL_DIR="/Volumes/Games/SteamLibrary/steamapps/common/Portal" \
  ./scripts/deploy-macos-portal.sh
```

El despliegue verifica el juego, conserva una copia inicial en
`backup_bin_i386/`, copia las bibliotecas ARM64, ajusta `@loader_path`, firma los
binarios y escribe el App ID `400`.

Ejecuta y carga una cámara de prueba:

```bash
cd "$HOME/Library/Application Support/Steam/steamapps/common/Portal"
HL2_SKIP_INTRO=1 ./hl2.sh -game portal -novid -windowed \
  -w 1280 -h 720 -condebug +map testchmb_a_05
```

El registro queda en `portal/console.log`.

Prueba automatizada con captura y salida limpia:

```bash
HL2_SKIP_INTRO=1 ./hl2.sh -game portal -novid -windowed \
  -w 1024 -h 640 -condebug -conclearlog +map testchmb_a_05 \
  +wait 500 +jpeg +wait 30 +quit
```

La captura se crea en `portal/screenshots/`.

## 7. Desplegar Half-Life 2

```bash
./scripts/build-macos-arm64.sh hl2
./scripts/deploy-macos-hl2.sh
```

Para una biblioteca de Steam no estándar:

```bash
HL2_DIR="/Volumes/Games/SteamLibrary/steamapps/common/Half-Life 2" \
  ./scripts/deploy-macos-hl2.sh
```

Ejecuta una cámara de prueba:

```bash
cd "$HOME/Library/Application Support/Steam/steamapps/common/Half-Life 2"
HL2_SKIP_INTRO=1 ./hl2.sh -game hl2 -novid -windowed \
  -w 1280 -h 720 -condebug +map d1_trainstation_01
```

El registro queda en `hl2/console.log`.

## 8. Desplegar Left 4 Dead (experimental)

No existe código de gameplay nativo de L4D en `game/client` ni `game/server`.
El objetivo aislado `l4d` usa actualmente un shell basado en Orange Box y el deploy
rechaza configuraciones `portal` o `hl2`. Esto permite validar el motor y el render,
pero la lógica de campañas, infectados, armas y Director requiere las fuentes
específicas de L4D.

```bash
./scripts/build-macos-arm64.sh l4d
L4D_DIR="$HOME/Library/Application Support/Steam/steamapps/common/left 4 dead" \
  ./scripts/deploy-macos-l4d.sh
```

Prueba mínima:

```bash
cd "$HOME/Library/Application Support/Steam/steamapps/common/left 4 dead"
HL2_SKIP_INTRO=1 ./hl2.sh -game left4dead -novid -windowed -w 1024 -h 640 \
  -condebug -conclearlog +developer 0 +map c1m1_hotel
```

## 9. Probar Portal 2 (experimental)

El repositorio `SteamDB2/Portal-2` aporta una parte del código de cliente y
entidades de servidor. Se compila contra nuestra ABI Portal/Orange Box; no es
todavía una reconstrucción completa del juego y algunos sistemas exclusivos de
Portal 2 siguen fuera del árbol.

Compila y crea un staging aislado junto a la instalación de Steam:

```bash
./scripts/build-macos-arm64.sh portal2
./scripts/stage-macos-portal2.sh
P2="$HOME/Library/Application Support/Steam/steamapps/common/Portal 2/portal2_arm64_test"
cd "$P2"
./hl2_osx -game portal2 -windowed -w 1280 -h 720 -novid -condebug
```

El staging conserva los binarios originales y monta el contenido mediante
`portal2_content`. El script extrae automáticamente los materiales VGUI que no
están disponibles en la disposición macOS y los coloca en `portal2_override`,
por lo que no se modifica la instalación de Steam. Para una prueba de mapa:

```bash
./scripts/prepare-portal2-map.sh sp_a1_intro1
./hl2_osx -game portal2 -windowed -w 1280 -h 720 -novid -condebug +map sp_a1_intro1
```

`prepare-portal2-map.sh` analiza el BSP y extrae sus modelos, gibs y materiales
dinámicos desde los VPK de Portal 2 y HL2 al overlay. Puedes cambiar
`sp_a1_intro1` por otro BSP de `portal2/maps/`.

Si el menú permanece abierto pero un mapa termina con `Model ... not found`, el
motor ARM64 ya está funcionando; ese mensaje indica que faltan dependencias de
contenido del mapa o módulos específicos de Portal 2, no un cierre por
arquitectura. El registro se guarda en `portal2/console.log`.

## 10. Probar contenido de Source SDK Base 2007

La instalación de Source SDK Base 2007 puede usarse como fuente de recursos
(`sourcetest`, VPK, materiales y mapas), pero sus ejecutables son i386 y sus
módulos `client.dll`/`server.dll` son Windows. No los sustituyas ni los mezcles
con las bibliotecas ARM64.

La prueba segura consiste en arrancar nuestro motor ARM64 indicando el directorio
`sourcetest`:

```bash
SDK2007="$HOME/Library/Application Support/Steam/steamapps/common/Source SDK Base 2007"
PORTAL="$HOME/Library/Application Support/Steam/steamapps/common/Portal"
cd "$PORTAL"
./hl2_osx -game "$SDK2007/sourcetest" -windowed -w 1280 -h 720 -novid -condebug
```

Esto valida la inicialización del motor y el render. Si aparecen errores de
`client.dll`, `server.dll` o materiales ausentes, son incompatibilidades de los
recursos originales, no un fallo de arquitectura del ejecutable ARM64.

## 11. Probar The Stanley Parable (rama de terceros)

La versión clásica de Steam usa App ID `221910` y una rama derivada de Portal 2.
En Apple Silicon, sus binarios macOS son únicamente `i386`; `thestanleyparable/bin`
contiene `client.dylib` y `server.dylib` i386. Por ello, el motor ARM64 no puede
cargar el juego original directamente.

Sí se puede usar el contenido como referencia para una adaptación ARM64. No
reemplaces `bin/engine.dylib` ni los módulos del juego en la instalación de Steam.
La prueba realizada con nuestro ejecutable confirma el bloqueo:
`dlopen(.../The Stanley Parable/bin/engine.dylib): incompatible architecture`.

## 12. Cambiar de juego correctamente

Después de compilar Portal, para probar Half-Life 2:

```bash
./scripts/build-macos-arm64.sh hl2
rg '^GAMES' build/c4che/game/client_cache.py build/c4che/game/server_cache.py
./scripts/deploy-macos-hl2.sh
```

Para volver a Portal:

```bash
./scripts/build-macos-arm64.sh portal
./scripts/deploy-macos-portal.sh
```

El script de despliegue se detiene si detecta el juego equivocado.

## 13. Restaurar los binarios oficiales

La opción más fiable es **Steam > Propiedades > Archivos instalados > Verificar
integridad**. Los scripts también conservan la primera copia local en
`backup_bin_i386/` y no la sobrescriben al volver a desplegar.

Consulta [Solucionar problemas en macOS](SOLUCIONAR_PROBLEMAS_MACOS.md) si el
build termina pero el juego no inicia o presenta errores gráficos.

## 14. Experimento con `source-sdk-portal2-private`

El snapshot privado de Portal 2 se mantiene fuera de este repositorio, por
ejemplo en `~/Downloads/source-sdk-portal2-private-arm64`. Contiene código de
cliente/servidor y VPC, pero no es un SDK autónomo: necesita la base completa de
Source, varias dependencias que no están incluidas y el archivo `gameui.rar` está
protegido. Por motivos legales y de mantenimiento no se copia al árbol público
ni se reemplazan los binarios de Steam.

Después de configurar una compilación ARM64, se puede comprobar qué módulos del
snapshot son compatibles con nuestros headers actuales:

```bash
./scripts/probe-portal2-private-arm64.sh
```

El script solo compila objetos temporales (no enlaza ni instala nada). Se ha
verificado que `prop_button.cpp` y `prop_testchamber_door.cpp` pasan esta fase
con los *shims* locales del snapshot. Los módulos que usan el simulador de
portales completo o pintura pueden quedar marcados como `FAIL` por cabeceras
ausentes como `paint_database.h`; eso indica una dependencia faltante, no un
fallo de ARM64. Para probar otra copia local:

```bash
PORTAL2_PRIVATE_SDK="/ruta/al/snapshot" \
  ./scripts/probe-portal2-private-arm64.sh
```

El objetivo de esta etapa es medir compilabilidad y aislar las adaptaciones de
macOS moderna. La integración funcional y cualquier ingeniería inversa deben
hacerse después, con una base cuya licencia permita redistribución.

Para repetir solo la prueba de arranque del staging:

```bash
PORTAL2_STAGE_DIR="$HOME/Library/Application Support/Steam/steamapps/common/Portal 2/portal2_arm64_test_20260906" \
  ./scripts/test-macos-portal2.sh
```

Esta prueba confirma que el launcher y las bibliotecas ARM64 cargan; no valida
que un mapa de Portal 2 sea jugable. Los mapas actuales todavía pueden detenerse
por entidades, shaders o recursos exclusivos que no están en la base Orange Box.

En la prueba de `sp_a1_intro1` se añadieron tres guardas de compatibilidad: los
eventos opcionales `player_connect`, y las reservas nulas de listas de física
(`GROUNDLINK`/`TOUCHLINK`). Con ellas el servidor alcanza `SV_ActivateServer`
sin `SIGSEGV`; si el proceso se mantiene abierto, se puede inspeccionar la
escena 3D y cerrar con Ctrl-C mientras seguimos completando entidades y assets.

### Actualización de renderizado: 7 de septiembre de 2026

Ya se ha observado `sp_a1_intro1` en 3D con texturas e iluminación. El bloqueo
visual no era únicamente la falta de un SDK: había VPK sin montar, shaders de
otra rama e índices VCS tratados incorrectamente como enteros con signo.
La preparación anterior queda sustituida por el staging aislado documentado en
[Portal 2 ARM64: experimento y pruebas](PORTAL2_ARM64_EXPERIMENTO.md).
Esto todavía no demuestra que la campaña de Portal 2 sea jugable.
