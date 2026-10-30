# View edges: which draw decisions depend on the view's width

Findings from the port's draw path and the recorded findings of the original about what the widened 16:9 view of the `qol` plan can show that the original's 5:3 top screen does not. Code reading only, no emulator. Entries are chronological. Feature file: [qol-fullscreen](../plans/features/qol-fullscreen.md). Related: [rendering-fog.md](rendering-fog.md), [streaming-rooms.md](streaming-rooms.md), [pedestrians-fade.md](pedestrians-fade.md), [rendering-shadows.md](rendering-shadows.md), [rendering-draw-order.md](rendering-draw-order.md).

## Current state

- The widened view changes only the horizontal extent: same vertical field of view, `x scale = y scale / aspect` ([camera-follow.md](camera-follow.md)). With the follow camera (pose FOV 40, vertical 30.5 degrees) the horizontal half angle is 24.5 degrees at 5:3 and 25.9 degrees at 16:9; the half diagonal is 29.1 degrees at 16:9. [§2026-10-30T14:03:24](#ts-20261030-140324)
- Almost every range decision is radial or depth based (distance from the camera, the camera look point or the player), not angular: fades, spawn bands, removal, fog and the far plane give the same behaviour at every point of a ring, so the strip between 5:3 and 16:9 shows the same appearance as the centre of the view. [§2026-10-30T14:03:24](#ts-20261030-140324)
- The only angular decision is the pedestrian spawn cone (view direction dot spawn direction at least 0.8, half angle 36.9 degrees), which holds the 16:9 view for pose FOVs up to 52 degrees; room loading uses the view direction only to order loads and to pick the preload anchor. [§2026-10-30T14:03:24](#ts-20261030-140324)
- The port has no frustum test (the original's list frustum 0x24f2a4 is not ported, RENDER-S71), so nothing is dropped at the 5:3 edge in the port. [§2026-10-30T14:03:24](#ts-20261030-140324)
- Left open as items: the largest camera pose FOV in the level data, rooms that only the widened strip reaches, and the cutscene and scanner cameras. [§2026-10-30T14:03:24](#ts-20261030-140324)

<a id="ts-20261030-140324"></a>

## 2026-10-30T14:03:24+01:00 — The culling and range decisions against the 16:9 edge (code reading of the port, 20261030-qolstores-D)

Method: Graphify queries on culling and `topViewRegion`, then `src/game_level.cpp` (`LevelSession::draw`), `src/game_level_rooms.cpp`, `src/runtime/room_streaming.cpp`, `src/runtime/room_streamer.cpp`, `src/runtime/pedestrian_manager.cpp`, `src/runtime/traffic_manager.cpp`, `src/runtime/shadow.cpp`, `src/game_shadows.cpp`, `src/runtime/semi_trans_near_camera.hpp` and the linked findings. Geometry: vertical half tangent t = 0.75 tan(pose FOV / 2); the horizontal half tangent is t x aspect (5/3 or 16/9).

**Decisions and verdicts**

| Decision | Rule in the port | Depends on the width? | Verdict at 16:9 |
| --- | --- | --- | --- |
| Frustum test of the draw lists (original 0x24f2a4) | not ported; the GPU clips (`LevelSession::draw`, [rendering-draw-order.md](rendering-draw-order.md)) | only in the original, and it drops only invisible items | no pop-in |
| Room visibility | a room is drawn when the streamer holds it loaded (`roomShown`): the current room, its neighbours and the preloaded rooms; not tested against the view | no; the view direction only orders the loads (`planRoomStreaming`: neighbour priority by dot) and picks the preload anchor (best dot among loaded neighbours) | the loaded set is the same; rooms that only the strip reaches are left open (below) |
| Room load distance | nearest-box distance 98 to 260, fog full at the far end ([rendering-fog.md](rendering-fog.md)) | no (radial) | the late loads are in 70 to 100 percent fog or out of view in the oracle; the strip shows the same ring |
| Placed objects | drawn by object state, script fades and room (`drawnPlacements`, `applyObjectFades`, `roomShown`) | no | no pop-in; no distance rule in the port (the `_lod1` switch is not ported, [models.md](models.md)) |
| Pedestrian spawn | 11 tries; spawn point within the tier's window (tier 0: 18 .. 57 m from the view point) and `dot(direction to point, camera forward) >= 0.8` (`kPedestrianSpawnFacing`) | yes, the cone | cone half angle 36.9 degrees; the 16:9 half diagonal is 29.1 degrees at pose FOV 40, so every spawn is still inside the cone; holds for pose FOVs up to 52.2 degrees (5:3: 54.4) |
| Pedestrian fade and draw test | fade-in inside 36 m, fade-out and removal by distance squared from the camera look point (`pedestrianNodeDrawn`, 0x10eb70, [pedestrians-fade.md](pedestrians-fade.md)) | no (radial) | same fade as at the centre of the view |
| Traffic spawn, removal and fade | spawn band 0.7 .. 1.75 of the class distances from the camera; removal beyond the despawn radius from the player; fade by the level's fade boxes (`trySpawn`, `refreshFade`) | no (radial and level boxes) | no pop-in; cars spawn on a ring in view in the original as well |
| Shadows | a fixed square of 2 x radius (radius 15) around a focus about 24 m ahead of the camera, orthographic, casters not culled (`shadowFocus`, [rendering-shadows.md](rendering-shadows.md)) | no; the focus ray is the central ray (x = 0) | the strip adds at most 0.03 x depth lateral width (1.2 m at 39 m); the region edge is visible in the original too, so no new kind of edge |
| Fog and far plane | depth along the view axis; far plane 260 with full fog there | no | same |
| Near-camera see-through fade | along the horizontal direction eye to player, distance 8 | no | same |
| Studs, particles, markers | drawn without a view test in the port | no | none (the original's particle culling is unknown, [particles.md](particles.md)) |

**Unknowns, last**

- The largest camera pose FOV over all levels: the pedestrian cone holds the 16:9 diagonal only below 52.2 degrees (a record sets 43 in the police station lobby, [camera-records-blending.md](camera-records-blending.md)); the other records were not scanned. Only the spawn fade-in is at stake. Item QOLFS-16.
- Whether rooms exist that are visible only in the strip (the side of the 16:9 view beyond the 5:3 view) and are neither loaded nor preloaded when the player looks straight ahead; the preload anchor is the best-dot neighbour. Needs a level data check of the room boxes against the 16:9 frustum. Item QOLFS-17.
- What the cutscene, director and scanner cameras show beyond the 5:3 frame (objects parked or posed outside the shot): the widened view applies to them with their own vertical FOV, and the spec does not say whether they stay 5:3. A question for the user. Item QOLFS-18.
