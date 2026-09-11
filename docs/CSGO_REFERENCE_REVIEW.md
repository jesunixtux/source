# Revisión acotada del repositorio de CS:GO

## Resultado

Descargado **después** de verificar las vistas azul/naranja y Chell en el
experimento de Portal 2. Puede aportar preguntas y comparaciones técnicas,
pero no es un reemplazo compilable inmediato del port. No se incorporó código,
no se ejecutaron sus herramientas y no se cambió la configuración del motor
para depender de él.

- Origen: [sr2echa/CSGO-Source-Code](https://github.com/sr2echa/CSGO-Source-Code/tree/master).
- Copia: `/Users/jesus/Downloads/CSGO-Source-Code-reference`.
- Commit: `dafb3cafa88f37cd405df3a9ffbbcdcb1dba8f63`.
- Clon superficial de `master`, aproximadamente 2,7 GB incluyendo `.git`.
- Alcance: inspección directa de proyectos, render de portales, Studio y
  dependencias concretas. El clon externo no está en el grafo del proyecto;
  no es una auditoría exhaustiva ni una prueba de compilación.

## Qué podría aportar

1. **Render de portales.** `cstrike15_src/game/client/portal/portalrender.cpp`
   conserva un camino antiguo y uno rápido condicionado al modo de renderizado
   en cola. Este último usa datos de mallas almacenados por fotograma. No es
   intercambiable con nuestra interfaz actual sin adaptar sus dependencias.
2. **Separación respecto a la pared.** En
   `game/client/portal/portalrenderable_flatbasic.cpp`, `DrawSimplePortalMesh`
   lleva el desplazamiento al shader en lugar de mover el plano en CPU.
   Merece investigar una solución propia equivalente para reducir la
   dependencia del ajuste de 4 unidades probado en nuestro primer mapa.
   No se ha demostrado que copiar ese renderer corrija nuestros fallos.
3. **Formatos de modelos.** `public/studio.h` usa Studio 49 y 256 huesos,
   coincidiendo con necesidades ya atendidas en nuestro adaptador. Es una
   referencia de formato, no una garantía de compatibilidad binaria completa.

Las rutas de esta sección son relativas a `cstrike15_src` salvo cuando se
indica el prefijo completo. También existen proyectos cliente/servidor de
Portal 2, pero su presencia no demuestra que estén completos.

## Impedimentos comprobados

- `devtools/bin/osx/vpc` y `lib/osx/tier0.dylib` son **i386**, según `file`.
  La biblioteca Steam examinada contiene i386/x86_64, no ARM64.
- `game/client/client_portal2.vpc` referencia
  `portal2/portal2_econ_ui.cpp`, ausente de la copia descargada.
- `avi/bink.cpp` depende de cabeceras del SDK Bink. La búsqueda de archivos
  rastreados `*bink.h`, `*bink*.a` y `*bink*.dylib` no devolvió resultados.
  Este archivo no nos proporciona por sí solo un decodificador ARM64.
- `CreateSolution.bat` apunta a VPC/Visual Studio para CS:GO; un documento
  adjunto pide recursos de mayo de 2017. Es información del repositorio,
  **no una instrucción que se haya ejecutado**: no se descargaron depots,
  torrents ni recursos adicionales.

## Procedencia y siguiente decisión

El [README](https://github.com/sr2echa/CSGO-Source-Code/blob/master/README.md)
lo describe como una filtración y su publicador declara no ser propietario.
El archivo [LICENSE](https://github.com/sr2echa/CSGO-Source-Code/blob/master/LICENSE)
contiene una licencia MIT a nombre del publicador. Esa combinación no acredita
por sí sola autorización del titular del código de Valve: no tratar esta
copia como SDK oficial ni dar por resueltos los permisos de integración.

Recomendación: conservar el port funcionando, ampliar sus pruebas de
oclusión/recursión y resolver procedencia y permisos antes de incorporar código
externo. El clon queda separado y sin modificaciones.
