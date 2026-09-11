# Portal 2: plan de migración al refactor P2 completo

## Contexto

Este repo es un motor época-P1 con un módulo `portal2` experimental que ya carga
y juega `sp_a1_intro1..5` (ver `PORTAL2_ARM64_EXPERIMENTO.md`). Hoy el jugador y
los portales usan lógica P1 adaptada (`CProp_Portal`/`C_Prop_Portal`,
`CPortal_Player`, pickups de `weapon_physcannon.cpp`).

**Decisión tomada:** portar el refactor P2 completo desde el SDK de referencia
(`source-sdk-portal2-private-arm64`, copia idéntica de `source-sdk-portal2-private`).
Eso significa sustituir la lógica P1 por la infraestructura P2 real:

- `CPortal_Base2D` / `C_Portal_Base2D` en lugar de `CProp_Portal`.
- `CPortal_Base2D_Shared::AllPortals` en lugar de `CProp_Portal_Shared::AllPortals`.
- Rework VM-grab del jugador (`WantsVMGrab`, `IsUsingVMGrab`,
  `SetUsingVMGrabState`, `UpdateVMGrab`), `m_GrabControllerPersistentVars`,
  `m_hPortalThroughWhichGrabOccured`, `C_PlayerHeldObjectClone`.
- El grab controller P2 real en lugar de los pickups de `weapon_physcannon.cpp`.

## Inventario de importación

Fuente: `SOURCE_ENGINE_SDK/source-sdk-portal2-private-arm64/game/`

### Capa 0 — infraestructura base (imprescindible para todo lo demás)

| Archivo | Ruta SDK | Notas |
|---|---|---|
| `portal_base2d_shared.{h,cpp}` | `shared/portal/` | NUEVO; define `CPortal_Base2D_Shared` + `AllPortals`; al final incluye `portal_base2d.h`/`c_portal_base2d.h` |
| `portal_placement.{h,cpp}` | `shared/portal/` | NUEVO (verificar: en MAIN hay `server/portal/portal_placement.*` bajo FClassnameIs) |
| `pvs_extender.{h,cpp}` | `server/portal/` | NUEVO |
| `physicsclonearea.{h,cpp}` | `server/portal/` | NUEVO (MAIN tiene `PhysicsCloneArea.*` propio; decidir si se reemplaza) |
| `portal_base2d.{h,cpp}` | `server/portal/` | NUEVO; `CPortal_Base2D : CBaseAnimating, CPortalSimulatorEventCallbacks, CPVS_Extender, CPortal_Base2D_Shared` |
| `c_portal_base2d.{h,cpp}` | `client/portal/` | NUEVO |
| `prop_mirror.cpp` | `server/portal/` | NUEVO |
| `PortalSimulation.h/.cpp` **P2** | `shared/portal/` | REEEMPLAZA el P1 de MAIN (3322→5298 líneas) |

### Capa 1 — jugador P2

| Archivo | Ruta SDK | Notas |
|---|---|---|
| `portal_player.h/.cpp` **P2** | `server/portal/` | REEEMPLAZA (2411→5737) |
| `portal_player_shared.h/.cpp` **P2** | `shared/portal/` | REEEMPLAZA (925→5227; .h 38→342) |
| `c_portal_player.h/.cpp` **P2** | `client/portal/` | REEEMPLAZA (1653→4212; .h 222→721) |
| `portal_gamemovement.h/.cpp` **P2** | `shared/portal/` | REEEMPLAZA (755→5245); en MAIN es `portal_gamemovement.cpp` + header ya existente |
| `portal_util_shared.h/.cpp` **P2** | `shared/portal/` | REEEMPLAZA (1774→3472) |
| `portal_playeranimstate.*` | `shared/portal/` | verificar si P2 difiere de MAIN |
| `portal_shareddefs.h/.cpp` | `shared/portal/` | verificar diferencia |
| `weapon_portalgun_shared.*` | `shared/portal/` | verificar diferencia |
| `weapon_portalbase.*`, `weapon_portalbasecombatweapon.*` | `shared/portal/` | verificar diferencia |

### Capa 2 — sistema de portales P2 (render y proyección, solo si hace falta)

| Archivo | Ruta SDK | Notas |
|---|---|---|
| `baseprojectedentity_shared.{h,cpp}` | `shared/portal/` | NUEVO (proyección de entidades) |
| `portalrender.{h,cpp}` | `client/portal/` | NUEVO (rendering P2; MAIN tiene `PortalRender.*` P1) |
| `portal_dynamicmeshrenderingutils.{h,cpp}` | `client/portal/` | NUEVO |
| `c_prop_mirror.cpp` | `client/portal/` | NUEVO |
| `c_portalghostrenderable.{h,cpp}` | `client/portal/` | NUEVO (MAIN tiene `C_PortalGhostRenderable.*`) |
| `materialproxy_portal_pickalphamask.cpp` | `client/portal/` | NUEVO |

### Capa 3 — sistemas P2 opcionales (fuera del alcance mínimo)

Pintura (`paint_*`), tractor beams, third rail, proyectores, `trigger_*` P2,
NPCs de P2, etc. NO se importan en la primera pasada.

### Capa 4 — cooperación con map compats existentes

- `server/portal2/portal2_map_compat.cpp` etc. dependen de la interfaz P1
  (`CProp_Portal_Shared`, `CPortal_Player` P1). Hay que reconciliarlos o
  reimplementar sus hooks sobre la interfaz P2.

## Orden de migración propuesto

1. **Capa 0** en `shared/portal` + `server/portal` + `client/portal`, con
   compilación individual de cada TU de capa 0 contra MAIN (checkpoint).
2. Adoptar `CPortal_Base2D`/`c_portal_base2d.h` en `portal2_entity_compat.cpp`
   y `portal2_map_compat.cpp` (adaptar accesos a `CProp_Portal` → `CPortal_Base2D`).
3. **Capa 1**: reemplazar jugador P1 por P2 (el punto más invasivo).
4. **Grab controller** (ya copiado en `shared/portal2/`) cableado al jugador P2.
5. Compilar `portal` y `portal2` completos; reintroducir gameplay test de intro1-5.

## Riesgos

- **Rompimiento del target `portal` P1**: compartimos `server_portal.vpc`.
  Hay que aislar la importación (p. ej. `server_portal2_extra.vpc` y una
  variante P2 de vpc, o `#ifdef PORTAL2`).
- **Diferencias de cabeceras base**: `baseentity.h` +250 líneas en SDK,
  `baseplayer_shared.h` +59, `gamemovement.h` +32. Verificar que MAIN los
  contiene, o importar.
- **`weapon_physcannon.cpp` P1** seguirá compilando en `portal`; el reto es
  desactivar sus pickups en `portal2` y no duplicar símbolos.
- **Compat de mapas intro1-5** puede perder comportamiento si el rework P2
  cambia semántica (E pickups, botones, puertas).
- **Archivos no compartidos**: `static_bullet_impacts` etc. pueden quedar sin
  referencia cruzada (`.vpc` mal formado).

## Estado

- [ ] Capa 0 import y compilación TU
- [ ] Compat entidades sobre interfaz P2
- [ ] Jugador P2 (capa 1)
- [ ] Grab controller P2 ligado
- [ ] Build completo `portal2` y gameplay intro1-5