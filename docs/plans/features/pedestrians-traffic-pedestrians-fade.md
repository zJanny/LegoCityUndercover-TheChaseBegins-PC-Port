# Pedestrians: the spawn fade-in and the mesh node alpha

## 1. Scope

The alpha fade of a spawned pedestrian's mesh node (start 0x275434, per-frame step 0x10eb70), the node the slots hang on, and how the instances are drawn with the node alpha. The manager, walk and rooms are in [pedestrians-traffic-pedestrians.md](pedestrians-traffic-pedestrians.md).

## 2. Status

IN_PROGRESS, updated 2026-10-20T14:13:30+02:00.
- The fade-in of a spawned node is ported as `nodeFadeStart` / `nodeFadeStep` (0x275434, 0x10eb70) in `PedestrianFades`; `chase-game` hangs each slot on a mesh node, starts the fade after a spawn, steps it every update and draws the instances with the node alpha. A spawn beyond 36 m drops to alpha 0 and stays until it walks inside; the original parks it under room +0x7c and removes it (PED-21). The draw test 0x2930e0 now runs from the node (PED-S11).

## 3. Verified behaviour

The pinned values of `pedestrian-fade` (0x275434 stores alpha 1 and step -1; 0x10eb70 turns the step to +16 within the squared fade-in distance and to -16 at the squared fade-out distance, clamps the alpha, honours bytes 0x52a1cc / 0x52a1cd) and the real-data check in lvl_01 (a spawn lies in the tier 0 window 18 .. 57 m and the view cone): [pedestrians-fade.md §2026-10-20T14:12:57](../../re/pedestrians-fade.md#ts-20261020-141257).

## 4. Stand-ins

- PED-S11 The pedestrian's mesh node is built from the model: its bounds centre and union box from `assets::modelBounds`, its radius +0x9c as the length of the half extents, and the fade point 0x52a1b4 of the draw test and the fade step as the camera eye. The matrix argument of 0x10eb70 is taken as the node's world matrix. needs: PED-13, PED-22. evidence: [pedestrians-fade.md §2026-10-20T14:12:57](../../re/pedestrians-fade.md#ts-20261020-141257).
- PED-S12 An instance with an alpha below 255 draws its opaque parts in the blended pass with the premultiplied blend (1, 1 - alpha); the node draw 0x10bcc8's pass 2 is not read. needs: PED-23. evidence: [pedestrians-fade.md §2026-10-20T14:12:57](../../re/pedestrians-fade.md#ts-20261020-141257).

## 5. Open items

- [ ] PED-21 — A faded-out pedestrian (node alpha 0) is not parked under room +0x7c by the room pass 0x2e6ce4 and removed by the manager, and a parked node does not return when the draw test passes; one spawned beyond 36 m stays invisible and keeps its slot until it walks inside. needs: PED-11. evidence: [pedestrians-fade.md §2026-10-20T14:12:57](../../re/pedestrians-fade.md#ts-20261020-141257).
- [ ] PED-22 — The source of the mesh node's radius +0x9c and bounds +0xa0 (the 40-byte record of 0x10d2d4) is not read. blocks: PED-S11. evidence: [pedestrians-fade.md §2026-10-20T14:12:57](../../re/pedestrians-fade.md#ts-20261020-141257).
- [ ] PED-23 — The node draw 0x10bcc8 and its pass for an alpha byte below 255 (blend, depth) are not read. blocks: PED-S12. evidence: [pedestrians-fade.md §2026-10-20T14:12:57](../../re/pedestrians-fade.md#ts-20261020-141257).

## 6. Findings summary

- The spawn fade-in 0x275434, the node step 0x10eb70, the node set-up and *0x37674c = 2: [pedestrians-fade.md §2026-10-20T14:12:57](../../re/pedestrians-fade.md#ts-20261020-141257).

## 7. Code and tests

- `src/runtime/node_fade.*` (`nodeFadeStart`, `nodeFadeStep`), `src/runtime/pedestrian_fade.*` (`PedestrianFades`, `pedestrianMeshNode`, `kMeshNodeType`), glue `src/game_pedestrian_fade.*`, `Scene::setInstanceTranslucentFade`.
- CTest: `pedestrian-fade`, `pedestrian-fade-real-data`.
