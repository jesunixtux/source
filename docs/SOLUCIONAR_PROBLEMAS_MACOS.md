# Solucionar problemas en macOS ARM64

Esta guía se aplica a las compilaciones probadas de Half-Life 2 y Portal. Antes
de cambiar código, reproduce el fallo con `-condebug -conclearlog` y conserva el
registro del juego.

## Diagnóstico mínimo

Portal:

```bash
cd "$HOME/Library/Application Support/Steam/steamapps/common/Portal"
HL2_SKIP_INTRO=1 ./hl2.sh -game portal -novid -windowed \
  -condebug -conclearlog +map testchmb_a_05
```

Half-Life 2:

```bash
cd "$HOME/Library/Application Support/Steam/steamapps/common/Half-Life 2"
HL2_SKIP_INTRO=1 ./hl2.sh -game hl2 -novid -windowed \
  -condebug -conclearlog +map d1_trainstation_01
```

Busca los errores relevantes:

```bash
rg -n -i "error|failed|couldn't load combo|pure virtual|wrong jpeg" \
  portal/console.log
```

En Half-Life 2 cambia la última ruta por `hl2/console.log`.

## El deploy dice que se compiló el juego equivocado

Síntoma:

```text
ERROR: client/server were not configured for Portal.
```

Solución para Portal:

```bash
./scripts/build-macos-arm64.sh portal
./scripts/deploy-macos-portal.sh
```

Solución para Half-Life 2:

```bash
./scripts/build-macos-arm64.sh hl2
./scripts/deploy-macos-hl2.sh
```

Confirma manualmente la configuración:

```bash
rg '^GAMES' build/c4che/game/client_cache.py build/c4che/game/server_cache.py
```

## Pantalla blanca, cian, negra o texturas muy iluminadas en Portal

Las actualizaciones Anniversary de los recursos de Steam ampliaron varias
combinaciones dinámicas de shaders: añadieron un tercer tipo de niebla y, para
`LightmappedGeneric`, una variante de lightmap bicúbico. Si el código calcula
los índices con el diseño antiguo, carga una variante válida pero incorrecta;
por eso la imagen puede verse blanca o cian aunque no aparezca un crash.

Este port sincroniza los `.inc` generados y los helpers de `stdshader_dx9` con
los `.vcs` actuales. No corrijas el problema dividiendo el índice ni editando
los VPK: eso desplaza las combinaciones estáticas y produce shaders
semánticamente incorrectos.

Recompila y vuelve a desplegar:

```bash
./scripts/build-macos-arm64.sh portal
./scripts/deploy-macos-portal.sh
```

Durante desarrollo, si ya se configuró Portal:

```bash
python3 ./waf build --targets=shaderapidx9,stdshader_dx9
./scripts/deploy-macos-portal.sh
```

El motor invalida la caché GL al cambiar el diseño. Si se está probando una rama
anterior y la caché sigue cargándose, consérvala con otro nombre y vuelve a abrir:

```bash
cd "$HOME/Library/Application Support/Steam/steamapps/common/Portal/portal"
mv glshaders.cfg glshaders.cfg.before-shader-fix
```

## `Couldn't load combo ...` o `pixel shader null!`

Es una incompatibilidad entre el índice calculado por `stdshader_dx9` y la tabla
del shader `.vcs` que viene con el juego. Comprueba, en este orden:

1. Se compiló el juego correcto.
2. Se recompilaron `stdshader_dx9` y `shaderapidx9`.
3. El deploy copió ambos `.dylib` nuevos.
4. La caché GL fue invalidada.

```bash
file bin/libstdshader_dx9.dylib bin/libshaderapidx9.dylib
rg -n -i "couldn't load combo|pixel shader null" portal/console.log
```

Ambas bibliotecas deben indicar `arm64`. No mezcles shaders compilados de Portal
2, TF2 actual u otra rama de Source con este motor.

## El launcher o una biblioteca no es ARM64

```bash
file hl2_osx
file bin/libengine.dylib
file bin/libstdshader_dx9.dylib
file portal/bin/client.dylib
```

En Half-Life 2 usa `hl2/bin/client.dylib`. Si alguno indica `i386` o `x86_64`,
repite el build nativo y el deploy. No ejecutes el build ARM64 bajo Rosetta.

## `dyld: Library not loaded`

Inspecciona la dependencia que falta:

```bash
otool -L bin/libengine.dylib
otool -L bin/libstdshader_dx9.dylib
```

Las bibliotecas del port deben resolverse con `@loader_path`. Reinstala cualquier
paquete de Homebrew ausente y repite el deploy:

```bash
brew install sdl2-compat sdl3 freetype fontconfig jpeg libpng gettext
./scripts/deploy-macos-portal.sh
```

Cambia el último comando por el de Half-Life 2 cuando corresponda.

## Fallo de firma después de `install_name_tool`

macOS invalida la firma cuando se reescriben dependencias Mach-O. El deploy vuelve
a firmar todos los binarios. Si una prueba manual dejó uno sin firma:

```bash
codesign --verify --verbose bin/libengine.dylib
codesign -f -s - bin/libengine.dylib
```

Después conviene repetir el deploy completo para que todas las bibliotecas queden
coherentes.

## `Wrong JPEG library version` o crash al crear una captura

El port usa el encabezado del mismo `libjpeg` de Homebrew que despliega como
`libjpeg.10.dylib`. Recompila `engine` y `GameUI`, y vuelve a copiar las
bibliotecas:

```bash
python3 ./waf build --targets=engine,GameUI
./scripts/deploy-macos-portal.sh
```

Comprueba la captura con `+jpeg`; debe terminar sin `Pure virtual function call`.

## WAF no reconoce `--arch`

No uses `--arch=arm64`. No es una opción de este WAF. Ejecuta el script nativo:

```bash
./scripts/build-macos-arm64.sh portal
```

Comprueba que la terminal no esté bajo Rosetta:

```bash
uname -m
```

## Encabezados o paquetes no encontrados

Restaura el entorno Homebrew ARM64:

```bash
export PATH="/opt/homebrew/bin:/opt/homebrew/sbin:$PATH"
export PKG_CONFIG_PATH="/opt/homebrew/opt/jpeg/lib/pkgconfig:/opt/homebrew/opt/openal-soft/lib/pkgconfig:$PKG_CONFIG_PATH"
pkg-config --modversion sdl2
pkg-config --modversion freetype2
```

Después vuelve a ejecutar el script de build, que configura WAF desde cero para
el juego elegido.

## Avisos conocidos que no bloquean el juego

Estos mensajes pueden aparecer en builds de desarrollo sin explicar un fallo de
renderizado normal:

- `DebugLuxels`, `FillRate`, `DebugNormalMap` o `DebugDepth` desconocidos: son
  shaders de visualización para desarrolladores.
- materiales `vgui/touch/*` ausentes: pertenecen a la interfaz táctil opcional.
- `SteamHTMLSurface` no disponible al ejecutar directamente desde Terminal: puede
  afectar paneles web, no el renderizado 3D.
- materiales de depuración u overview ausentes al cargar un mapa con `+map`.

No ignores, en cambio, `Couldn't load combo`, errores de `dyld`, `Wrong JPEG
library version`, `Pure virtual function call` o binarios de arquitectura distinta.

## Restaurar el juego

La opción recomendada es **Steam > Propiedades > Archivos instalados > Verificar
integridad**. La primera versión de los binarios anteriores al deploy también se
conserva en `backup_bin_i386/`, y el script evita sobrescribir esa copia.
