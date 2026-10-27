# Referencia de Portal 2 Paint Gun

Selección del repositorio `thecatontheceiling/Portal-2-Paint-Gun-src`.

La parte más útil para este port es la ruta servidor de impacto y aplicación:
`cpaintblob.cpp`, `paint_database.cpp`, `paint_stream.cpp`, `paint_sprayer.cpp`
y `trigger_paint_cleanser.cpp`. También se conservan las contrapartes cliente
para estudiar la replicación de streams y blobs.

Estos archivos son referencia y no forman parte del build actual. El README
original indica que el proyecto no incluye Blobulator. El código también espera
interfaces, clases y símbolos que no están en esta rama, incluyendo la API de
paintmap, proyectores, eventos, CEG y varias entidades de Portal 2.

La integración debe hacerse por piezas y con pruebas: primero `CPaintBlob` y
`CPaintDatabase` para entidades, después el camino de superficies del mundo y
finalmente la replicación cliente. No sustituir los stubs completos de una vez.
