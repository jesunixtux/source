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

## Compilar y preparar

Requisitos: herramientas de desarrollo de Xcode, dependencias de compilación
descritas en [la guía principal](COMPILAR_MACOS_ARM64.md), Python 3 con `vpk`,
y las instalaciones locales de Portal 2 y Portal. Portal aporta los recursos
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
```

La prueba abre el juego, espera 20 segundos y lo cierra. Guarda `engine.log`,
`stdout.log` y la captura `map.png` en una subcarpeta de `diagnostics`.
`MAP LOAD PASS` exige que el proceso siga vivo, que el servidor active el mapa
y que no aparezcan los errores de índices de shaders detectados. **Hay que
inspeccionar la imagen**: no es una prueba automática de jugabilidad o calidad
visual. La captura requiere autorización de grabación de pantalla de macOS.

`test-macos-portal2.sh` prueba únicamente arranque y `+quit`; un timeout ahora
se considera fallo, no éxito.

Si reaparece la pantalla morada, comprueba las rutas de `gameinfo.txt` y que
`renderer_compat/shaders` provenga del cache esperado. No enlaces los shaders
de `Portal 2/platform` al renderer de esta rama. No ejecutes los antiguos
parches de extracción sobre una carpeta reparada: `prepare-portal2-map.sh`
detecta ahora el staging nuevo y evita ese paso.

Persisten recursos/proxies exclusivos, cubemaps que no se leen correctamente y
entidades sin lógica completa. Algunos objetos pueden aparecer blancos o con
iluminación incorrecta. `--no-prop-lighting` es solo una opción diagnóstica para
comparar la iluminación de objetos; no se aplica al lanzador predeterminado.
