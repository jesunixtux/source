# Source Engine: macOS Port (Community Edition)

Port del **Source Engine (Community Edition)** para macOS, compilado de forma nativa en **arm64 (Apple Silicon)** y también **x86_64 (Intel)**.

> **Aviso importante:** Este motor solo debe usarse con **copias legales** de los juegos
> que hayas adquirido (p. ej., a través de Steam). No se distribuye ningún juego, asset ni
> contenido de los mismos junto a este repositorio. El proyecto es únicamente para fines
> educativos y no está afiliado ni respaldado por Valve Corporation.
> No se permite el uso comercial de este código.

---

## Índice

1. [Juegos compatibles](#juegos-compatibles)
2. [Requisitos y dependencias](#requisitos-y-dependencias)
3. [Compilar en macOS (Apple Silicon / arm64)](#compilar-en-macos-apple-silicon--arm64)
4. [Compilar en macOS (Intel / x86_64)](#compilar-en-macos-intel--x86_64)
5. [Desplegar el motor en un juego](#desplegar-el-motor-en-un-juego)
6. [Ejecutar el juego](#ejecutar-el-juego)
7. [Solución de problemas](#solución-de-problemas)

---

## Juegos compatibles

> **Nota:** esta lista la mantiene el autor del port. Se irá actualizando conforme se
> verifiquen los juegos. Antes de instalar, confirma tu juego está en la lista o en las
> issue/notes del proyecto.

| Juego | App ID Steam | Estado |
|---|---|---|
| Portal | 400 | ✅ Probado (testchmb_a_05) |
| Half-Life 2 | 220 | ✅ Probado |
| Half-Life 2: Episode One | 380 | ⏳ Pendiente de probar |
| Half-Life 2: Episode Two | 420 | ⏳ Pendiente de probar |
| Half-Life 2: Lost Coast | 340 | ⏳ Pendiente de probar |
| Counter-Strike: Source | 240 | ⏳ Pendiente de probar |
| Day of Defeat: Source | 300 | ⏳ Pendiente de probar |
| Team Fortress 2 | 440 | ⏳ Pendiente de probar |
| Source SDK Base 2007 | 218 | ⏳ Pendiente de probar |

Requisitos generales para que un juego sea compatible:

- Debe usar el **Source Engine clásico (Source 1)** de la era HL2/Portal.
- Debe tener su propia carpeta de juego con `gameinfo.txt` y los `.vpk` correspondientes.
- No se revisan ni incluyen archivos de juego: solo ejecutas los binarios compilados aquí
  sobre tu instalación legal existente.

---

## Requisitos y dependencias

### Herramientas base

| Herramienta | Por qué | Comprobar con |
|---|---|---|
| macOS 13+ (Ventura o superior) | Sistema base | `sw_vers` |
| Xcode Command Line Tools | Compilador clang (en Apple Silicon viene con el SDK). También sirve con Xcode completo | `xcode-select -p` |
| Homebrew | Gestor de paquetes de las librerías | `brew --version` |
| Python 3.9+ | WAF (sistema de build) está escrito en Python | `python3 --version` |
| `pkg-config` | Configurar el build de WAF | `pkgconf --version` |
| `cmake` | Algunos submodulos (thirdparty) | `cmake --version` |
| `ffplay` (paquete `ffmpeg`) | Reproduce la intro del juego en el port | `/opt/homebrew/bin/ffplay` |

### Librerías (instalar con Homebrew)

El motor enlaza contra los siguientes paquetes. En Apple Silicon Homebrew instala en
`/opt/homebrew`; en Intel en `/usr/local`.

```bash
brew install \
  sdl2 \
  sdl2-compat \
  sdl3 \
  freetype \
  fontconfig \
  jpeg \
  libpng \
  libcurl \
  zlib \
  openal-soft \
  gettext \
  pkgconf \
  ffmpeg
```

Dependencias adicionales que pueden hacer falta si usas funcionalidades opcionales:

```bash
brew install opus   # códec de voz (build con --enable-opus)
brew install libedit # build del servidor dedicado (--dedicated)
```

Notas:

- `sdl2-compat` + `sdl3` son los que aportan el runtime SDL que el port copia a la carpeta
  `bin/` del juego (`libSDL2-2.0.0.dylib`, `libSDL3.0.dylib`).
- `gettext` aporta `libintl.8.dylib`, dependencia indirecta de fontconfig.
- Algunas fórmulas son *keg-only* (opacas): por eso los scripts de build exportan
  `PKG_CONFIG_PATH` apuntando a `/opt/homebrew/opt/...` (ver abajo).

---

## Compilar en macOS (Apple Silicon / arm64)

> En Apple Silicon el compilador produce binarios arm64 por defecto. Es la vía
> recomendada: nativa, sin Rosetta.

### Paso 1 — Clonar el repositorio con submódulos

```bash
git clone --recurse-submodules <URL-del-repo>
cd source
```

Si ya clonaste sin submódulos:

```bash
git submodule init
git submodule update
```

### Paso 2 — Preparar el entorno

```bash
export PATH="/opt/homebrew/bin:/opt/homebrew/sbin:$PATH"
export HOMEBREW_PREFIX="/opt/homebrew"
export PKG_CONFIG_PATH="/opt/homebrew/opt/jpeg/lib/pkgconfig:/opt/homebrew/opt/openal-soft/lib/pkgconfig:$PKG_CONFIG_PATH"
```

### Paso 3 — Configurar el build

Usa el script incluido (hace lo del paso 2 automáticamente):

```bash
./scripts/build-macos-arm64.sh
```

o manualmente:

```bash
./waf configure -T debug --disable-warns --arch=arm64
```

Variantes de configuración útiles:

```bash
./waf configure -T release --disable-warns --arch=arm64   # build optimizado
./waf configure -T debug  --disable-warns --arch=arm64 --enable-opus   # con voz
./waf configure -T debug  --disable-warns --arch=arm64 -d # solo servidor dedicado
```

### Paso 4 — Compilar

```bash
./waf build
```

Para compilar solo un subconjunto de módulos (más rápido durante el desarrollo):

```bash
./waf build --targets=togl,shaderapidx9,stdshader_dx9 -j 10
```

### Paso 5 — Verificar los binarios

Los `.dylib` y el launcher quedan en `build/`:

```bash
ls -la build/launcher_main/hl2_launcher
file build/launcher_main/hl2_launcher   # debe decir arm64
```

---

## Compilar en macOS (Intel / x86_64)

Igual que arm64 pero forzando arquitectura y (en Apple Silicon) usando Rosetta 2:

```bash
./scripts/build-macos-amd64.sh
```

o manualmente:

```bash
export PATH="/opt/homebrew/bin:/opt/homebrew/sbin:$PATH"
export PKG_CONFIG_PATH="/opt/homebrew/opt/jpeg/lib/pkgconfig:/opt/homebrew/opt/openal-soft/lib/pkgconfig:$PKG_CONFIG_PATH"
./waf configure -T debug --disable-warns --arch=x86_64
./waf build
```

En hardware Intel los prefijos serán `/usr/local` en lugar de `/opt/homebrew`.

---

## Desplegar el motor en un juego

El port se ejecuta **sobre una instalación legal del juego** (p. ej. la de Steam).
El despliegue copia los `.dylib` compilados a `bin/` del juego, reescribe los
*install names* a `@loader_path` y coloca el launcher como `hl2_osx`.

### Half-Life 2

```bash
./scripts/deploy-macos-hl2.sh
```

### Portal

```bash
./scripts/deploy-macos-portal.sh
```

**Importante:** los scripts hacen copias de seguridad del contenido i386 original en
`backup_bin_i386/` dentro de la carpeta del juego. Para restaurar el juego original,
devuelve esos archivos o verifica la integridad desde Steam.

---

## Ejecutar el juego

Desde la carpeta del juego (p. ej. `.../common/Portal`):

```bash
./hl2.sh -game portal
```

Con opciones útiles durante el desarrollo:

```bash
./hl2.sh -game portal -novid -windowed -condebug "+map testchmb_a_05"
```

- `-game <carpeta>` → elige el juego (carpeta con `gameinfo.txt`).
- `-novid` → omite la intro (evita también que `hl2.sh` la intente reproducir).
- `-windowed` → ventana en vez de pantalla completa.
- `-condebug` → vuelca la consola a `portal/console.log`.
- `HL2_SKIP_INTRO=1` → desactiva la reproducción externa de la intro.

---

## Solución de problemas

| Síntoma | Causa probable | Solución |
|---|---|---|
| `Couldn't load combo ... (dyn=...)` en consola | Fallo de lookup shader/vcs (HDR vs LDR) | Recompilar `shaderapidx9` + `stdshader_dx9` con el fix del divisor de vcs |
| `pixel shader null!` / pantalla corrupta | Consecuencia del combo no cargado | Ver fila anterior |
| `Couldn't load library ...dylib` | Faltan dylibs en `bin/` del juego | Repetir el script de deploy de ese juego |
| `install_name_tool: ... link edit command` | Binarios sin firmar tras modificar | Ya lo hace el deploy (codesign ad-hoc); si falla, firma manual: `codesign -f -s - bin/*.dylib` |
| `libSDL2` o `libSDL3` no encontrada | Runtime SDL no copiado | Re-empaquetar con el deploy o copiar `libSDL2-2.0.0.dylib`, `libSDL3.0.dylib` desde Homebrew |
| Compilación falla por encabezados faltantes | faltan paquetes brew | Revisar lista de dependencias e instalar las que falten |
| `--arch` no se reconoce | waf sin configuración previa | Correr `./waf configure` antes |

---

## Contribuir

Contribuciones bienvenidas (fixes de port, compatibilidad de juegos, documentación).
Cuando verifiques un juego nuevo, actualiza la [tabla de juegos compatibles](#juegos-compatibles).

---

*Proyecto educativo y sin fines comerciales. No afiliado a Valve Corporation.*