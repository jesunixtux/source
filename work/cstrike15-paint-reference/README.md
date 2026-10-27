# Referencia de paintmap desde cstrike15_src

El repositorio contiene la implementación que falta en este port:

- `engine/paint.cpp` / `engine/paint.h`: almacenamiento, pintado de superficies,
  consulta de color, RLE y guardado/restauración del paintmap.
- `materialsystem/cmatpaintmaps.cpp` / `.h`: texturas de paintmap en el cliente.
- `public/materialsystem/ipaintmapdatamanager.h` y
  `public/game/shared/portal2/paint_enum.h`: interfaces y enumeraciones.

Estos archivos se conservan como referencia y todavía no forman parte del build.
La rama actual no tiene `CPaintmapDataManager`, `CMatPaintmaps`, los puntos de
registro del material system ni las llamadas de inicialización del BSP. Copiarlos
solos no habilitaría `HasPaintmap()` y produciría errores de enlace o de integración.

La siguiente integración mínima debe ser por capas: primero `CPaintmapDataManager`
en el motor y su inicialización al cargar el mundo; después `CMatPaintmaps` y los
shaders; finalmente conectar `IVEngineServer::HasPaintmap`,
`SpherePaintSurface` y `SphereTracePaintSurface`.
