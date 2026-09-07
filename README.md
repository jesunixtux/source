# Source Engine: macOS ARM64 Port (Community Edition)

Port comunitario de Source Engine para macOS, compilado de forma nativa para
Apple Silicon (`arm64`). El motor se usa sobre una instalación legal de Steam;
este repositorio no incluye ni redistribuye contenido de Valve.

> Este proyecto es educativo, no comercial y no está afiliado ni respaldado por
> Valve Corporation.

## Estado de compatibilidad

| Juego / objetivo | Steam App ID | Objetivo de build | Estado |
|---|---:|---|---|
| Half-Life 2 | 220 | `hl2` | ✅ Probado en Apple Silicon |
| Portal | 400 | `portal` | ✅ Probado en Apple Silicon, incluidos shaders Anniversary |
| Portal 2 | 620 | `portal2` | 🧪 Experimental: compatibilidad parcial y entidades importadas; no es un port completo |
| Left 4 Dead | 500 | `l4d` | 🧪 Experimental: shell Orange Box, arranque/render y pruebas de recursos; sin gameplay completo |
| Left 4 Dead Lite | 500 | `l4d_lite` | 🧪 Prueba jugable basada en gameplay de HL2 usando mapas y recursos de L4D |
| The Stanley Parable | 221910 | `stanley` | 🧪 Experimental: usa ABI compatible con Portal para pruebas de recursos |
| Otros juegos Source 1 | — | — | No probados todavía |

Los módulos `client.dylib` y `server.dylib` no son intercambiables entre juegos.
Siempre hay que volver a configurar y compilar al cambiar de objetivo. Los scripts
de despliegue comprueban esto antes de copiar archivos cuando corresponde.

Los objetivos experimentales no implican compatibilidad total con el juego
original. En algunos casos se reutiliza la ABI o la implementación de gameplay
más cercana disponible para comprobar launcher, renderizado, mapas y recursos.

## Inicio rápido

Clona el repositorio junto con todos sus submódulos:

```bash
git clone --recurse-submodules https://github.com/jesunixtux/source.git
cd source
```

No uses `git clone` sin `--recurse-submodules`, porque `ivp`, `lib` y `thirdparty`
son necesarios para compilar el motor.

Instala primero Xcode Command Line Tools, Homebrew y las dependencias:

```bash
xcode-select --install
brew install sdl2 sdl2-compat sdl3 freetype fontconfig jpeg libpng \
  libcurl zlib openal-soft gettext pkgconf ffmpeg
```

## Compilación y ejecución

### Portal

```bash
./scripts/build-macos-arm64.sh portal
./scripts/deploy-macos-portal.sh
cd "$HOME/Library/Application Support/Steam/steamapps/common/Portal"
HL2_SKIP_INTRO=1 ./hl2.sh -game portal -novid
```

### Half-Life 2

```bash
./scripts/build-macos-arm64.sh hl2
./scripts/deploy-macos-hl2.sh
cd "$HOME/Library/Application Support/Steam/steamapps/common/Half-Life 2"
HL2_SKIP_INTRO=1 ./hl2.sh -game hl2 -novid
```

### Portal 2 (experimental)

```bash
./scripts/build-macos-arm64.sh portal2
```

El soporte de Portal 2 todavía es parcial. El repositorio contiene trabajo de
compatibilidad y herramientas de preparación de recursos, pero no todos los
módulos del juego original están disponibles o implementados.

### Left 4 Dead (experimental)

Este árbol no contiene las fuentes propias completas de L4D. `l4d` es un objetivo
ais lado que compila un shell Orange Box y permite probar launcher, OpenGL y
recursos; no ofrece campañas, infectados ni lógica de partida completa.

```bash
./scripts/build-macos-arm64.sh l4d
L4D_DIR="$HOME/Library/Application Support/Steam/steamapps/common/left 4 dead" \
  ./scripts/deploy-macos-l4d.sh
cd "$HOME/Library/Application Support/Steam/steamapps/common/left 4 dead"
HL2_SKIP_INTRO=1 ./hl2.sh -game left4dead -novid -windowed -w 1280 -h 720 +map c1m1_hotel
```

### Left 4 Dead Lite (experimental)

`l4d_lite` utiliza la implementación de gameplay de HL2 con mapas y recursos de
Left 4 Dead. Sirve como objetivo de prueba jugable, no como reemplazo del juego
original.

```bash
./scripts/build-macos-arm64.sh l4d_lite
```

### The Stanley Parable (experimental)

`stanley` usa el objetivo compatible con Portal como shell ARM64 para probar
contenido y recursos. Los binarios públicos originales del juego son i386 y no
se consideran módulos ARM64 nativos de este proyecto.

```bash
./scripts/build-macos-arm64.sh stanley
```

## Builds optimizadas

Para una compilación optimizada:

```bash
BUILD_TYPE=release ./scripts/build-macos-arm64.sh portal
```

`BUILD_TYPE` admite `debug`, `release` y `none`.

No se pasa `--arch=arm64`: Clang selecciona ARM64 de forma nativa cuando WAF se
ejecuta en un Mac Apple Silicon. La opción `--arch` no existe en esta versión de
WAF.

## Documentación

- [Tutorial completo de compilación y despliegue](docs/COMPILAR_MACOS_ARM64.md)
- [Diagnóstico y solución de problemas](docs/SOLUCIONAR_PROBLEMAS_MACOS.md)

## Comprobación rápida

```bash
file build/launcher_main/hl2_launcher
rg '^GAMES' build/c4che/game/client_cache.py build/c4che/game/server_cache.py
```

El launcher debe indicar `Mach-O 64-bit executable arm64`. Los dos archivos de
caché deben mostrar el mismo juego que se va a desplegar.

Los scripts admiten instalaciones no estándar mediante variables de entorno:

```bash
PORTAL_DIR="/Volumes/Games/SteamLibrary/steamapps/common/Portal" \
  ./scripts/deploy-macos-portal.sh

HL2_DIR="/Volumes/Games/SteamLibrary/steamapps/common/Half-Life 2" \
  ./scripts/deploy-macos-hl2.sh
```

## Copias de seguridad

En el primer despliegue, los scripts conservan los binarios originales en
`backup_bin_i386/` dentro del juego. No sobrescriben esa copia en despliegues
posteriores. La forma recomendada de volver por completo a la versión oficial es
usar **Steam > Propiedades > Archivos instalados > Verificar integridad**.

## Alcance del proyecto

El objetivo principal es mantener y ampliar la compatibilidad de Source Engine en
macOS moderno y Apple Silicon. Los experimentos con juegos adicionales se usan
para validar subsistemas del motor, compatibilidad de recursos y diferencias de
ABI entre distintas ramas de Source 1.

Que un objetivo consiga arrancar un mapa o renderizar recursos no significa que
el juego correspondiente esté completamente porteado.

## Licencia y contenido

Consulta los archivos de licencia del repositorio. Debes poseer legalmente cada
juego que uses. Los mapas, modelos, materiales, sonidos, shaders compilados y
demás assets siguen perteneciendo a sus respectivos titulares.
