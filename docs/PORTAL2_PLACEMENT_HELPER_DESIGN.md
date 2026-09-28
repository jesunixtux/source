# Portal 2 Placement Helper Design

This specification is independent of external implementations. It describes
the contract observed in installed Portal 2 BSP entity data and the existing
Portal weapon consumer.

## Confirmed Map Contract

The installed Portal 2 FGD defines the entity as a point entity based on
`Targetname`, `Parentname`, `Angles` and `EnableDisable`. It confirms these
entity-specific names:

- `radius`, default `16`;
- `proxy_name`;
- `attach_target_name`;
- `snap_to_helper_angles`;
- `force_placement`;
- outputs `OnObjectPlaced` and `OnObjectPlacedSize`.

The `EnableDisable` base confirms `StartDisabled` and the `Enable`/`Disable`
inputs. The installed `placement_helper.vmf` also uses `StartDisabled`.

A direct read of the entity lumps in all 117 installed official Portal 2,
Peer Review and Perpetual Testing Initiative BSPs found 437 helpers in 88
maps. The following keys were observed on those helpers:

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

No helper in those BSPs contained `spawnflags`, `proxy_name` or
`attach_target_name`. `proxy_name` and `attach_target_name` remain accepted as
FGD-confirmed keyvalues, but their runtime behavior is
`TODO(PORTAL2-RESEARCH)`. No spawnflag behavior is invented.

There is no separate `target_radius` or `target_angles` key in the inspected
data: the placement consumer's target radius is `radius`, and its target
orientation is the entity's normal `angles` transform. The exact runtime
semantics of `force_placement`, `hide_until_placed`, `target_size`,
`usesizelimit`, the two FGD-only names and the placement outputs remain
`TODO(PORTAL2-RESEARCH)`; this phase stores or replicates confirmed data but
does not guess those behaviors.

## Authority and State

The server owns the helper entity state:

- enabled/disabled state from `StartDisabled` and `Enable`/`Disable`;
- world-space origin, including parent movement;
- `radius`;
- `angles` as target orientation;
- `snap_to_helper_angles` as the orientation opt-in.

The client receives the data required by the portal-gun consumer through the
normal entity send/receive table. Entity origin and angles use the base entity
network transform; radius, enabled state and the confirmed placement flags are
explicit properties. The client does not maintain an independent source of
truth.

## Selection Contract

`UTIL_FindPlacementHelper(position, player)` will:

1. enumerate live P2 helper entities (server by classname, client through the
   replicated client entity list), without a persistent handle cache;
2. reject disabled helpers;
3. reject `radius <= 0`;
4. compute squared distance from the query point to the helper center;
5. reject points outside the helper radius;
6. choose the smallest distance;
7. treat squared distances within `0.01` as a practical tie and use the
   smallest entity index as the tie-breaker;
8. return `NULL` only when no valid candidate exists.

The same order and comparison tolerance are used on client and server.
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

The isolated server test creates real `info_placement_helper` entities and
must cover:

- point inside radius;
- point outside radius;
- invalid radius;
- disabled helper;
- no helper;
- deterministic selection between two valid helpers;
- target angles and orientation opt-in.

The test must exercise the actual helper lookup against real helper entities,
not replace the lookup with a direct expected-value call. The existing portal
gun gameplay fixture separately exercises the placement consumer.

Client/server send/receive table compatibility is additionally exercised by
the normal client/server connection and data-table handshake in the fixture;
the server remains authoritative.
