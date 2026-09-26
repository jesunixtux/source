# Portal 2 Placement Helper Design

This specification is independent of external implementations. It describes
the contract observed in installed Portal 2 BSP entity data and the existing
Portal weapon consumer.

## Observed BSP Contract

Across 392 installed Portal 2 BSP helper entities, the following keys were
observed:

- `classname "info_placement_helper"`
- `origin`
- `angles`
- `radius`
- `StartDisabled`
- `snap_to_helper_angles`
- `force_placement`
- `hide_until_placed`
- `parentname`
- `targetname`
- `target_size`
- `usesizelimit`

The phase will implement the confirmed spatial/orientation fields. The exact
runtime semantics of `force_placement`, `hide_until_placed`, `target_size` and
`usesizelimit` are not established by this repository or the BSP entity lump.
They remain `TODO(PORTAL2-RESEARCH)` and will not be guessed in this phase.

## Authority and State

The server owns the helper entity state:

- enabled/disabled state from `StartDisabled` and `Enable`/`Disable`;
- world-space origin, including parent movement;
- `radius`;
- `angles` as target orientation;
- `snap_to_helper_angles` as the orientation opt-in.

The client receives the same values through normal entity networking. It does
not maintain an independent authoritative helper list.

## Selection Contract

`UTIL_FindPlacementHelper(position, player)` will:

1. enumerate registered P2 helper entities;
2. reject disabled helpers;
3. reject `radius <= 0`;
4. compute squared distance from the query point to the helper center;
5. reject points outside the helper radius;
6. choose the smallest distance;
7. use the smallest entity index as the tie-breaker for equal distances;
8. return `NULL` only when no valid candidate exists.

The same order and comparison tolerance must be used on client and server.
`player` is reserved for future player-specific visibility/filter behavior and
will not silently change selection in this phase.

## Placement Consumer

`CWeaponPortalgun::AttemptSnapToPlacementHelper()` already performs the second
surface trace, normal comparison, radius check and final placement validation.
The helper implementation must provide the real candidate and data without
duplicating those checks.

## Debugging

`portal2_debug_placement_helpers` is P2-only:

- `0`: no overlay/output;
- `1`: selected helper only;
- `2`: all candidates, including rejection reasons.

The debug view will show center, radius, enabled state, target angles and the
selection result. Console output is rate-limited to placement queries rather
than emitted every frame.

## Test Contract

The isolated test must cover:

- point inside radius;
- point outside radius;
- invalid radius;
- disabled helper;
- no helper;
- deterministic selection between two valid helpers;
- target-angle replication and orientation opt-in.

The test must exercise the actual helper lookup and placement consumer, not
replace the lookup with a direct expected-value call.
