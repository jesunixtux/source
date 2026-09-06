# Source Engine: macOS ARM64 Port (Community Edition)

Port comunitario de Source Engine para macOS, compilado de forma nativa para
Apple Silicon (`arm64`). El motor se usa sobre una instalación legal de Steam;
este repositorio no incluye ni redistribuye contenido de Valve.

> Este proyecto es educativo, no comercial y no está afiliado ni respaldado por
> Valve Corporation.

## Estado de compatibilidad

| Juego | Steam App ID | Módulo WAF | Estado |
|---|---:|---|---|
| Half-Life 2 | 220 | `hl2` | Probado en Apple Silicon |
| Portal | 400 | `portal` | Probado en Apple Silicon, incluidos shaders Anniversary |
| Left 4 Dead | 500 | `l4d` | Experimental: arranque/render; sin gameplay completo |
| Otros juegos Source 1 | — | — | No probados todavía |

Los módulos `client.dylib` y `server.dylib` no son intercambiables entre juegos.
Siempre hay que volver a configurar y compilar al cambiar de `hl2` a `portal`, o
viceversa. Los scripts de despliegue comprueban esto antes de copiar archivos.

## Inicio rápido

Instala primero Xcode Command Line Tools, Homebrew y las dependencias:

```bash
xcode-select --install
brew install sdl2 sdl2-compat sdl3 freetype fontconfig jpeg libpng \
  libcurl zlib openal-soft gettext pkgconf ffmpeg
```

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

### Left 4 Dead (experimental)

Este árbol no contiene las fuentes propias de L4D. `l4d` es un objetivo aislado
que compila un shell Orange Box y permite probar launcher, OpenGL y recursos; no ofrece
campañas, infectados ni lógica de partida completa.

```bash
./scripts/build-macos-arm64.sh l4d
L4D_DIR="$HOME/Library/Application Support/Steam/steamapps/common/left 4 dead" \
  ./scripts/deploy-macos-l4d.sh
cd "$HOME/Library/Application Support/Steam/steamapps/common/left 4 dead"
HL2_SKIP_INTRO=1 ./hl2.sh -game left4dead -novid -windowed -w 1280 -h 720 +map c1m1_hotel
```

Para una compilación optimizada:

```bash
BUILD_TYPE=release ./scripts/build-macos-arm64.sh portal
```

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

## Licencia y contenido

Consulta los archivos de licencia del repositorio. Debes poseer legalmente cada
juego que uses. Los mapas, modelos, materiales, sonidos, shaders compilados y
demás assets siguen perteneciendo a sus respectivos titulares.
