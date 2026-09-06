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

## 3. Elegir el juego

El argumento del script decide qué implementaciones de `client` y `server` se
compilan:

```text
./scripts/build-macos-arm64.sh portal
./scripts/build-macos-arm64.sh hl2
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
`'portal'` o `'hl2'`, según la compilación elegida.

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

## 9. Cambiar de juego correctamente

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

## 10. Restaurar los binarios oficiales

La opción más fiable es **Steam > Propiedades > Archivos instalados > Verificar
integridad**. Los scripts también conservan la primera copia local en
`backup_bin_i386/` y no la sobrescriben al volver a desplegar.

Consulta [Solucionar problemas en macOS](SOLUCIONAR_PROBLEMAS_MACOS.md) si el
build termina pero el juego no inicia o presenta errores gráficos.
