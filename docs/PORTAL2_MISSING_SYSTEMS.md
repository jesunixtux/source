# Portal 2 Missing Systems

Audit of `codex/portal2-compat` before the `info_placement_helper` phase.

This audit preserves the pre-existing dirty worktree. The current tree already
contains Portal 2 and Wheatley compatibility changes; those changes are not
attributed to this phase.

## Status Classification

| System | Status | Evidence and consequence |
|---|---|---|
| Server `info_placement_helper` | MISSING / COMPAT-STUB | `game/server/portal/info_placement_helper.h` exposes a zero radius and a lookup that always returns `NULL`; no P2 entity implementation is registered. |
| Client `info_placement_helper` | MISSING / COMPAT-STUB | `game/client/portal/c_info_placement_helper.h` uses a hard-coded radius and always returns `NULL` from the lookup. |
| Client portal placement | COMPAT-STUB | `game/client/portal/portal_placement_compat.cpp` returns zero/false/artificial success for the collision checks. |
| Server portal placement | PARTIAL / LEGACY-P1 | `game/server/portal/portal_placement.cpp` contains a real P1 placement path with a different API and fixed-size assumptions. |
| Shared portal placement | FUNCTIONAL / P1-BASED | `game/shared/portal/portal_placement.cpp` contains real trace, fit, bumper, cleanser, no-portal-volume and overlap checks. It is not currently the client P2 placement source of truth. |
| Server placement runtime wrapper | COMPAT-STUB | `game/server/portal/portal_runtime_compat.cpp` returns `PORTAL_PLACEMENT_SUCCESS` for checks that are not performed and has an empty object-rotation wrapper. |
| Client Portal 2 grab controller | COMPAT-STUB | `game/client/portal2/portal_grabcontroller_compat.cpp` leaves attach, detach, update, simulate and held-object clone operations empty or false. |
| Shared Portal 2 grab controller | P2-IMPLEMENTATION-NOT-WIRED | `game/shared/portal2/portal_grabcontroller_shared.cpp` contains the real implementation, but the current generated compile database shows it only under `GAME_DLL`; the client still uses the compat shim. |
| Shared Portal 2 player pickup | P2-IMPLEMENTATION-NOT-WIRED | `game/shared/portal2/player_pickup.cpp` exists but is absent from the current generated compile database. The build still contains the legacy server pickup path. |
| Portal 2 server pickup controller | FUNCTIONAL / NOT-FULLY-WIRED | `game/server/portal2/player_pickup_controller.cpp` contains a real controller, but its ownership path is not yet connected to `CPortal_Player` and the P2 shared pickup API end to end. |
| Portal 2 prediction | PARTIAL | `game/client/portal2/portal2_prediction.cpp` is compiled for `CLIENT_DLL`, but its command and finish paths currently delegate to the base implementation. |
| Portal 2 VScript | FUNCTIONAL / PARTIAL | `game/server/portal2/portal2_vscript.cpp` runs a real Squirrel VM with a deliberately limited binding surface and reports missing native functions. |
| Portal 2 map I/O | FUNCTIONAL / PARTIAL | `portal2_map_compat.cpp`, `portal2_entity_compat.cpp` and `portal2_intro_script.cpp` support bounded map flows, not general Portal 2 campaign semantics. |
| Movie/instructor presentation | COMPAT-STUB | `portal2_entity_compat.cpp` completes unavailable movie playback and turns instructor hints into text; it does not recreate the client presentation. |
| Portal 2 render targets | MISSING | No `CPortal2ExtraRenderTargets` or equivalent exists. |
| Portal render targets | LEGACY-P1 / PARTIAL-P2 | `game/client/portal/portal_render_targets.*` provides the P1 portal targets used by the current compatibility renderer. |
| Portal renderer | FUNCTIONAL / P1-BASED | `PortalRender.*`, `C_Prop_Portal` and the existing portal simulation provide the current P2 visual path, but it is not the P2 refactor renderer. |
| Paint blob simulation | COMPAT-STUB / PARTIAL | `paint_blobs_server_compat.cpp` leaves collision and update operations empty. |
| Tractor beam cleanup | COMPAT-STUB | `trigger_tractorbeam_server_compat.cpp` leaves both blob cleanup methods empty. |
| Portal multiplayer rules | COMPAT-STUB | The client/server P2 gamerules compat files contain false-return and no-op methods. |
| Portal gun server compat | COMPAT-STUB / DUPLICATED | `weapon_portalgun_server_compat.cpp` supplies empty methods beside the real Portal weapon implementation. Symbol ownership must be resolved before removal. |
| Instructor lesson hooks | COMPAT-STUB | `c_portal2_lesson.cpp` leaves lesson preprocessing and action handling empty. |

## Wiring Findings

`game/client/wscript` selects `client_base.vpc`, `client_portal.vpc` and
`client_portal2_extra.vpc` for `portal2`. It does not select
`client_portal2.vpc`.

`game/server/wscript` selects `server_base.vpc`, `server_portal.vpc` and
`server_portal2_extra.vpc` for `portal2`. It does not select
`server_portal2.vpc`.

The last generated `build/compile_commands.json` therefore needs to be treated
as a build-state observation, not as a substitute for explicit VPC ownership:

- `portal2_prediction.cpp` is compiled with `CLIENT_DLL` and `PORTAL2`.
- `portal_grabcontroller_compat.cpp` is compiled with `CLIENT_DLL` and `PORTAL2`.
- `portal_grabcontroller_shared.cpp` is compiled with `GAME_DLL` and `PORTAL2`.
- `player_pickup.cpp` under `game/shared/portal2` is not present.
- `player_pickup_controller.cpp`, map compat and the Portal 2 gameplay test are
  present in the generated server commands.
- `portal_placement_compat.cpp` is the client placement TU.
- `portal_runtime_compat.cpp` is the server placement wrapper TU.

The VPC files and generated command database should be reconciled with a clean
Portal 2 configure before later shim removal.

## Current Test Coverage

Existing tests cover bounded intro maps, portal firing/traversal, cube/button
I/O, scenes, elevator transitions, VScript and the experimental Wheatley hit
loop. There are no dedicated placement-helper or real P2 grab-controller tests.

`docs/PORTAL2_ARM64_EXPERIMENTO.md` explicitly limits the experiment to a
partial ARM64 map/gameplay port and states that the current placement path is
P1-compatible rather than a full Portal 2 campaign implementation.
