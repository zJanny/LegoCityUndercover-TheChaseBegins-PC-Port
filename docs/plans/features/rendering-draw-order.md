# Rendering: draw order and semi-transparent lists

## 1. Scope

The order in which the scene pass draws parts (the sorted draw lists 0x530634, the material key 0x115128, the depth key 0x25688c), which parts are drawn blended, where the particle pass sits among them, the distance fade of mesh nodes and the near-camera semi-transparent lists that `AddToSemiTransNearCameraList` fills. Materials, lighting and the other passes: [rendering.md](rendering.md).

## 2. Status

IN_PROGRESS, updated 2026-10-27T18:49:04+01:00. The scene draws list 0 as the original does: opaque parts sorted by material key and front to back, then blended parts back to front by the view depth of each part's box (`Scene::draw`, `render/draw_order.*`). A node alpha, a material alpha below 255 or a blend destination other than zero makes a part blended. The waypoint columns and badges named by `AddToSemiTransNearCameraList` fade out when the camera comes within 8 units of them on its way to the player and fade in again.

## 3. Verified behaviour

- Keys, comparator and library sort transcribed from 0x115128, 0x25688c, 0x10bbe8, 0x2d0f84, 0x259ebc / 0x259b94 (CTest `draw-order`, values worked by hand from the disassembly and the CRC-16/USB catalogue).
- Every shipped part's record box equals its vertex box (16122 parts, CTest `draw-order-real-data`).
- The see-through distance 0x374630 is 8.0 at gameplay-start (oracle read, both loads); the near-camera rule and its alpha steps are pinned in CTest `semi-trans-near-camera`.

## 4. Stand-ins

- RENDER-S71 The draw list inputs the port chooses: the texture words of the material key are the order in which the scene first meets each texture name (the original hashes texture resource pointers), parts enter the list in the port's instance order, there is no frustum test, the sky is drawn before the opaque half, and the second draw of node +0xde bit 2 is not made. needs: RENDER-78. evidence: [rendering-draw-order.md §2026-10-27T18:49:04](../../re/rendering-draw-order.md#ts-20261027-184904).
- RENDER-S72 Near-camera lists: every scene instance of the named model is an entry (the original walks the entities of the level's rooms), the entry box is the model bounds, not the node box the original reads, the entry name and node bit 5 are ignored, only the normal camera case with distance 8 runs, and the entry alpha caps the alpha the node tweens set. needs: RENDER-79. evidence: [rendering-draw-order.md §2026-10-27T18:49:04](../../re/rendering-draw-order.md#ts-20261027-184904).
- RENDER-S64 Emitters are drawn in creation order after the scene and the studs, and the view depth of a particle (near fade, far-to-near sort) is the distance along the camera's forward axis; the original sorts the emitters into the blended half of list 0 and its model-view product was not read. needs: RENDER-65. evidence: [particles.md §2026-10-18T09:40](../../re/particles.md#ts-20261018-094004).

## 5. Open items

- [ ] RENDER-65 Draw the particle emitters inside the sorted blended half of list 0 at the depth key of the emitter position instead of after the scene. blocks: RENDER-S64. evidence: [rendering-draw-order.md §2026-10-27T18:49:04](../../re/rendering-draw-order.md#ts-20261027-184904).
- [ ] RENDER-66 The distance fade of mesh nodes: every mesh node fades out beyond a distance that depends on its radius (tiers below 3, 6 and 12 m radius, fade-out at 38, 76 and 114 m of surface distance, fade-in again below 36, 72 and 108; the alpha byte steps by 16 per update) unless `EnableDistanceFade(object, 0)` switched it off; the port draws every mesh at full alpha. Needs the inputs of 0x10eb70 checked in the oracle (eye position, step order, the two flag bytes); the blended pass is ported. blocks: SCRIPT-S233, SCRIPT-282. evidence: [script-natives-world.md §2026-10-18T11:01:40](../../re/script-natives-world.md#ts-20261018-110140).
- [ ] RENDER-78 What the second draw of nodes with +0xde bit 2 is and which nodes set the bit; the order of the scene walk that fills the list, which decides ties. blocks: RENDER-S71. evidence: [rendering-draw-order.md §2026-10-27T18:49:04](../../re/rendering-draw-order.md#ts-20261027-184904).
- [ ] RENDER-79 The near-camera rule for the other cameras (distance 2.5, target 5 units ahead of the camera), the close distance 1.0 and its conditions, and whether the rule runs before or after the node tweens. blocks: RENDER-S72. evidence: [rendering-draw-order.md §2026-10-27T18:49:04](../../re/rendering-draw-order.md#ts-20261027-184904).

## 6. Findings summary

- Draw lists, keys, sort, pass choice, particles in the list and the near-camera rule: [rendering-draw-order.md §2026-10-27T18:49:04](../../re/rendering-draw-order.md#ts-20261027-184904).
- Distance fade of mesh nodes: [script-natives-world.md §2026-10-18T11:01:40](../../re/script-natives-world.md#ts-20261018-110140).

## 7. Code and tests

- `src/render/draw_order.*` (library `chase_draw_order`, no OpenGL): keys, comparator, sort, `DrawList`, pass and blend choice; `Scene::draw` and `Scene::setDepthRange` in `src/render/scene.cpp`; part boxes and mesh flags in `assets::parseModel`. CTest `draw-order`, `draw-order-real-data`.
- `src/runtime/semi_trans_near_camera.*` (rule and alpha step), `src/game_semi_trans.*` (entries to scene instances, update after the camera, alpha before the draw). CTest `semi-trans-near-camera`.
