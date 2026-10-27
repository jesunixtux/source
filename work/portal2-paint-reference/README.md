# Referencia de Paint para Portal 2

Estos archivos se copiaron desde `SourceSDK2013Ports/csgo-src` porque contienen
la implementación del administrador de paintmap, las texturas de paintmap y
los shaders de blobs de pintura.

No forman parte del build actual. Su integración requiere adaptar, como mínimo,
las interfaces de `materialsystem`, el registro del `CPaintmapDataManager`, la
implementación de `IVEngineServer`, los mensajes de red de paintmap y los
targets de shaders. Copiarlos directamente al target Portal 2 produciría
dependencias faltantes.

Los archivos compartidos de blobs y sprayers ya existen en el árbol principal y
se dejaron fuera para evitar duplicados. Esta carpeta es una base de comparación
para una integración por etapas.
