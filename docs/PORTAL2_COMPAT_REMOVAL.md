# Portal 2 Compatibility Removal

This is a removal ledger. No shim is removed by this document alone.

| Shim | Current state | Replacement | Status |
|---|---|---|---|
| `game/client/portal/portal_placement_compat.cpp` | Client placement checks return `false` or `PORTAL_PLACEMENT_SUCCESS` without doing the corresponding work. | Wire the real shared placement API for the P2 client after the helper contract and client trace dependencies are available. | NOT STARTED |
| `game/client/portal2/portal_grabcontroller_compat.cpp` | Client `CGrabController` and `C_PlayerHeldObjectClone` methods are empty/false. | Compile and connect `game/shared/portal2/portal_grabcontroller_shared.cpp` for `CLIENT_DLL`; keep client-only clone code in the shared implementation. | NOT STARTED |
| `game/server/portal/portal_runtime_compat.cpp` | Placement checks return artificial success; object rotation is empty; paintgun methods are empty shims. | Route P2 to the real shared placement/pickup implementations and isolate any remaining P1 compatibility API. | NOT STARTED |
| `game/server/portal/weapon_portalgun_server_compat.cpp` | Supplies empty/duplicate weapon methods next to the real Portal weapon implementation. | Establish one owner for each symbol after a clean P2 link and retain P1 behavior separately. | NOT STARTED |
| `game/client/portal2/portal_mp_gamerules_compat.cpp` | P2 multiplayer queries return false and map data loading is empty. | Implement only if a supported P2 multiplayer/co-op target is explicitly added. | NOT STARTED |
| `game/server/portal/portal_mp_gamerules_server_compat.cpp` | Stats, community/co-op and spawn suppression methods are no-ops/false. | Implement against the target's supported game rules or keep the feature explicitly disabled and documented. | NOT STARTED |
| `game/server/portal/trigger_tractorbeam_server_compat.cpp` | Blob cleanup methods are empty. | Connect the real paint/tractor state owner before enabling tractor-beam maps. | NOT STARTED |
| `game/server/portal/paint_blobs_server_compat.cpp` | Blob update/collision loop is empty. | Wire the real paint simulation and its server authority. | NOT STARTED |
| `game/server/portal2/portal2_entity_compat.cpp` | Movies are skipped and instructor hints are text-only. | Add client movie/instructor systems only after the core map I/O contract is stable. | PARTIAL |
| `game/server/portal2/portal2_intro_script.cpp` | Native adapter covers known intro callbacks and explicitly reports unsupported calls. | Extend only from observed map/script requirements; do not turn it into an unbounded guess-based VM layer. | PARTIAL |

## Removal Gate

A shim may move to `DONE` only when the replacement:

- compiles for both required sides (`CLIENT_DLL`/`GAME_DLL`);
- links without duplicate symbol ownership;
- loads `sp_a1_intro1`;
- preserves Portal 1 and HL2 builds;
- has a behavior-specific test rather than only a compile test;
- has no artificial success return for a failed operation.
