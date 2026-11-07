# View edges: which draw decisions depend on the view's width

Findings from the port's draw path and the recorded findings of the original about what the widened 16:9 view of the `qol` plan can show that the original's 5:3 top screen does not. Code reading only, no emulator. Entries are chronological. Feature file: [qol-fullscreen](../plans/features/qol-fullscreen.md). Related: [rendering-fog.md](rendering-fog.md), [streaming-rooms.md](streaming-rooms.md), [pedestrians-fade.md](pedestrians-fade.md), [rendering-shadows.md](rendering-shadows.md), [rendering-draw-order.md](rendering-draw-order.md).

## Current state

- The widened view changes only the horizontal extent: same vertical field of view, `x scale = y scale / aspect` ([camera-follow.md](camera-follow.md)). With the follow camera (pose FOV 40, vertical 30.5 degrees) the horizontal half angle is 24.5 degrees at 5:3 and 25.9 degrees at 16:9; the half diagonal is 29.1 degrees at 16:9. [§2026-10-30T14:03:24](#ts-20261030-140324)
- Almost every range decision is radial or depth based (distance from the camera, the camera look point or the player), not angular: fades, spawn bands, removal, fog and the far plane give the same behaviour at every point of a ring, so the strip between 5:3 and 16:9 shows the same appearance as the centre of the view. [§2026-10-30T14:03:24](#ts-20261030-140324)
- The only angular decision is the pedestrian spawn cone (view direction dot spawn direction at least 0.8, half angle 36.9 degrees), which holds the 16:9 view for pose FOVs up to 52 degrees; room loading uses the view direction only to order loads and to pick the preload anchor. [§2026-10-30T14:03:24](#ts-20261030-140324)
- The port has no frustum test (the original's list frustum 0x24f2a4 is not ported, RENDER-S71), so nothing is dropped at the 5:3 edge in the port. [§2026-10-30T14:03:24](#ts-20261030-140324)
- The largest camera record FOV over all levels is 43 (every Bound setup; two Room setups hold 0 and are never a pose), so the pedestrian spawn cone holds the 16:9 view in every shipped record pose (limit 52.2 degrees). [§2026-11-02T15:16:57](#ts-20261102-151657)
- Rooms that only the 16:9 strip reaches and that are not loaded exist in every world level (follow camera, sampled), but in each level the 5:3 view already reaches an unloaded room nearer than any of them: the strip adds more of the same far, fogged pop-in, no new kind. [§2026-11-02T15:16:57](#ts-20261102-151657)
- At x20 the hooks scale by the same rule as at x4 with no overflow; the depth buffer resolves coarser far away (QOLVD-7). [§20261102-221857](#ts-20261102-221857)
- Distances of drawing and loading and what a view distance factor reaches: the far planes, the fog and the pedestrian fade words are scaled; room loading has no distance in the original rule (it follows the room graph); the traffic removal radius 50 is now an input of the traffic step (`TrafficSystemStep::despawnRadius`, [traffic-ai-driver.md §2026-11-03T08:37:32](traffic-ai-driver.md#ts-20261103-083732)) and not yet scaled. [§2026-11-02T21:25:47](#ts-20261102-212547)
- Left open as items: the cutscene and scanner cameras. [§2026-10-30T14:03:24](#ts-20261030-140324), [§2026-11-02T15:16:57](#ts-20261102-151657)
- The room request adds the rooms of the 16:9 strip (widened view on) and above x1.0 every room within 260 m times the factor (the user's rules of 2026-11-03) through the scripted-request set; at x20 all 43 rooms of Cherry Tree Hills load, the draw calls triple and memory and frame time stay level, because the port keeps every room model from the level start. [§2026-11-03T08:48:54](#ts-20261103-084854)

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

<a id="ts-20261102-151657"></a>

## 2026-11-02T15:16:57+01:00 — Largest record FOV and rooms only the 16:9 strip reaches (scan of the level data, 20261102-qolctl2-E)

Method: C++ test `qol-view-edges-real-data` (`tests/qol/view_edges_real_test.cpp`) over the shipped RomFS, through the decoders of `chase_assets` and `chase_runtime` (`CameraRecords`, `RoomStreamer`, `worldLevels`). Items QOLFS-16 and QOLFS-17 of [qol-fullscreen](../plans/features/qol-fullscreen.md). Corrects nothing; closes two unknowns of [§2026-10-30T14:03:24](#ts-20261030-140324).

**Camera record FOV (QOLFS-16)**

- The 30 level files of `levels/levels/` give 2,075 `.cam` setups in 10 files: 1,822 `Bound` blocks, all FOV 43, and 253 `Room` blocks, 251 of FOV 43 and two of FOV 0 (`room13` of lvl_03 auburn, `room38` of lvl_06 apollo island). A level bound named `Camera*` without a setup keeps the loader default 43 ([data-tables-config.md](data-tables-config.md)). The largest FOV over all `CameraRecords` is 43; no record exceeds 52.23.
- The two FOV 0 blocks are Room records. A Room record is the pose only through the follow controller (pose FOV 40, [camera-follow.md](camera-follow.md)) and never enters a record blend ([camera-records-blending.md](camera-records-blending.md)); the port applies only `Bound` setups to the records.
- The record pose blends FOV as a weighted mean of the entries (follow entry 40, records 43), so it stays at 43 or below. The spawn cone (dot at least 0.8, half angle 36.87 degrees) holds the 16:9 half diagonal for pose FOV up to 52.23 degrees (5:3: 54.45); at 43 the 16:9 half diagonal is 31.1 degrees. Verdict: no spawn fade-in beside the cone for any shipped gameplay camera. Cutscene lens FOVs are another source ([cutscenes.md](cutscenes.md)) and stay with QOLFS-18.

**Rooms reached only by the strip (QOLFS-17)**

- Model: the 3D follow camera (pose FOV 40, 10 m behind the player and 1.7 m above the look point, pitch from that offset, far plane 260) with the view direction of that pitched camera as the streaming view; the loaded set is the settled `RoomStreamer` set (current room, neighbours, up to three preloads of the best-dot neighbour). A room is reached when a part box of its model (meshes from 1, as `roomBoxOf`) meets the frustum (exact box against pyramid test); the "strip only" set is rooms reached by the 16:9 frustum but not by the 5:3 one at the same pose. Players: 50 m grid over all room boxes, 5 m above the floor of the smallest containing box, 12 yaws.
- Result: in all eight world levels some samples have a strip-only room that is not loaded (lvl_00 10 of 1,572 samples, lvl_01 295 of 6,972, lvl_02 20 of 864, lvl_03 470 of 3,996, lvl_04 202 of 6,396, lvl_05 106 of 3,276, lvl_06 227 of 2,496, lvl_07 427 of 3,156). These are rooms outside the loaded block (the current room and its neighbours, 100 m tiles in the streets) that the side of the wider frustum reaches; most are first reached at 208 m or deeper (lvl_01: 250 of 307 strip-only room events), almost none nearer than 104 m, the nearest per level 45 to 145 m (lvl_00 144.5, lvl_01 108.2, lvl_02 115.3, lvl_03 117.0, lvl_04 47.8, lvl_05 120.5, lvl_06 111.1, lvl_07 44.8).
- The same samples show that the original already has the same case at 5:3: in every level a room that the 5:3 frustum reaches and that is not loaded lies nearer than any strip-only room (nearest at 5:3: lvl_00 0.0 inside the eye box, lvl_01 54.1, lvl_02 22.6, lvl_03 0.0, lvl_04 44.9, lvl_05 100.1, lvl_06 30.0, lvl_07 32.5). The streaming rule does not guarantee that every room in view is loaded; the room-load distance and the fog hide the late loads ([rendering-fog.md](rendering-fog.md)). The strip therefore adds more of the same kind at the sides, not a new kind of pop-in.
- Sensitivity: the sample counts depend on the player height and the grid (a denser run with 24 yaws on a 25 m grid at 5 m and at 30 m above the floor gives the same pattern: every level has strip-only unloaded rooms, nearest depth 31 to 112 m, and in every level the 5:3 baseline is nearer). Room boxes are bounding boxes of whole parts, so a part box may be reached where its geometry is not; the depths are lower bounds.
- Rule, not a list: a room is a strip-only candidate when it is outside the current room's neighbour list and the three preloads and its model meets the 16:9 but not the 5:3 frustum within the far plane; in the street grids these are the tiles two rings out at the left and right edge of the view. What the widened view would need to load them: the room request rule (`planRoomStreaming`: neighbours of the current room and three preloads of the best-dot neighbour) would have to request the rooms the 16:9 frustum reaches, a change of the room streaming of the game, not a QoL-only draw change.

**Unknowns, last**

- Whether the original shows these far rooms unloaded at the sides at all (the oracle saw no abrupt pop-in at 5:3, [rendering-fog.md](rendering-fog.md)); the scan uses the port's rule and box geometry, not an oracle picture of the wide frame.
- Whether the user wants the extra rooms loaded for the widened view: item QOLFS-20.

<a id="ts-20261102-212547"></a>

## 2026-11-02T21:25:47+01:00 — Distances the port uses for drawing and loading, and what a view distance factor can reach (20261102-qolmenu-E)

Method: Graphify queries on the traffic removal radius (`TrafficSystem::syncCars`) and on room requests (neighbours, preloads), then `src/game_camera.cpp`, `src/game_cutscene.cpp`, `src/game_traffic.cpp`, `src/runtime/traffic_system.cpp`, `src/runtime/room_streaming.cpp` and the linked findings. Code reading only.

**Distances a factor can scale (the hooks scale them)**
- The follow far plane `kFollowFarPlane` 260, the cutscene far plane `game_cutscene::kFarPlane` 4000 and the free camera's `radius * 20`.
- The level fog start and end and the fog table built from them ([rendering-fog.md](rendering-fog.md)).
- The fade-in and fade-out words of the pedestrian `DistanceFadeTable` (36/72/108 and 38/76/114): the pedestrian spawn window is x0.5 and x1.5 of them, and the node fade and the pedestrian draw test read the same words ([pedestrians-fade.md](pedestrians-fade.md)).

**Distances that are not draw or load distances (not scaled)**
- The near planes, shadows (`kShadowFar` 25), the scanner marker fade (20 and 180), head icons (6 and 10), near-camera fades (8), pickup and AI distances.

**Room loading has no distance to multiply**
- Rooms are requested by the room graph: the current room, its neighbours and up to three preloads of the best-dot neighbour (`planRoomStreaming`, [streaming-rooms.md](streaming-rooms.md)). The 98 to 260 of the findings in [rendering-fog.md](rendering-fog.md) is where loads were seen, not a constant of the code. A wider view therefore loads rooms only by a new rule (for example rooms whose nearest box point lies within the scaled far plane), which is a change of the game's streaming, not a scale of a number.

**Traffic**
- The spawn band (x0.7 to x1.75 of the step's LOD bands) and the fade come from the step's LOD table and are reachable from `game_traffic.cpp`.
- The removal radius (`kTrafficStandInDespawnRadius`, 50, TRAFFIC-S8) is set inside `TrafficSystem::syncCars` in `chase_runtime`, not passed in by `TrafficSystemStep`, so a hook cannot reach it. Scaling only the band would spawn cars that the radius removes at once; the radius has to be an input of the step with default 50.

**Unknowns, last**
- Whether the user wants a streaming rule for a wider view (QOLVD-5).

<a id="ts-20261102-221857"></a>

## 2026-11-02T22:18:57+01:00 — What a view distance factor of x20 does to the hooks (20261102-qolmore-C)

Method: code reading of `src/game_level.cpp`, `src/runtime/fog.cpp`, `src/render/scene.cpp` and `src/render/draw_order.cpp`, the factor math in a test (`qol-view-distance`) and the depth formulas computed by hand. Range: x1.0 to x4.0 in quarters, then whole steps to x20.0 ([qol-view-distance.md §2026-11-02T18:43:07](../design/qol-view-distance.md#ts-20261102-184307)).

- Far plane: follow 260 gives 5200, cutscene 4000 gives 80000, free and scanner cameras times 20; all floats, no overflow. The fog start and end scale together; the fog table (128 entries from near and far plane) is built by the same rule, so its shape in the fog band is the same relative to the far plane at every factor.
- Pedestrian fade tables times 20 (fade-in 720, 1440, 2160; fade-out 760, 1520, 2280); the spawn window of tier 0 becomes 360 to 1140 m. Whether pedestrian nodes exist that far out was not measured; the windows only choose among existing nodes.
- Depth precision (QOLVD-7): the near plane does not scale, so a 24-bit depth buffer resolves z^2 / (near x 2^24) at distance z. At the far plane with near 0.5 that is 0.01 m at x1.0, 0.13 m at x4.0 and 3.2 m at x20.0; the cutscene camera (near 0.1, far 80000) gives about 3800 m. Distant coplanar surfaces can therefore flicker at high factors. Computed only; no picture was taken.
- `depthSortKey` and the integer fog table conversions work on the scaled values without overflow (range scaled with the far plane).

**Unknowns, last**

- Whether the flicker is visible in play at x20 and how far out; needs a picture check.

<a id="ts-20261103-084854"></a>

## 2026-11-03T08:48:54+01:00 — Rooms requested by the widened view and the View distance factor, and their cost at x20 (20261103-large-E)

Method: the rule of the user's answers of 2026-11-03 ([qol-view-distance.md §2026-11-02T18:43:07](../design/qol-view-distance.md#ts-20261102-184307), [qol-screen-layout.md §2026-11-02T15:42:07](../design/qol-screen-layout.md#ts-20261102-154207)) implemented in `src/qol/room_request_hook.*`; Graphify queries on the room request (`RoomStreamer`, `planRoomStreaming`), then `src/runtime/room_streamer.cpp`, `src/runtime/room_streaming.cpp` and `src/game_level_rooms.cpp`; short runs of `chase-game` in Cherry Tree Hills at the police station entrance after mission 1-1 (start of the CTest `game-room-streaming`, 960x540 window, offscreen video driver, 150 updates, values read from the debug overlay of the last frame). Code reading and port runs only, no emulator.

**Where the extra rooms enter the room request**
- The room request takes a set of extra wanted rooms each update: the rooms the scripts asked for (`StreamRoomIn`), which `planRoomStreaming` adds to the wanted set at priority 0 after the current room, its neighbours and the preloads. The hook adds its rooms to that set, so they are loaded by the same rule (9 updates of loading, unloaded when no longer wanted) and the original's own requests are kept. The level-start settle does not take that set, so the extra rooms finish loading 9 updates after the start.
- Camera of the rule: the follow camera, the camera the room request already uses (its eye, view direction, vertical field of view and the far plane 260); a cutscene or scanner camera does not change the request.
- Strip rule (widened view on): a room is added when its whole-model box meets the 16:9 frustum and not the 5:3 frustum (exact separating-axis test of box and pyramid), up to the far plane times the factor.
- Radial rule (factor above x1.0): a room is added when the nearest point of its box lies within 260 m times the factor of the camera eye, in every direction.

**What a loaded room changes in the port**
- Every room model is read and kept from the level start; loading a room makes its instances visible, marks the room loaded for the level scripts, and lets the traffic, the pedestrians and the attack characters use it (`LevelSession::roomShown`). More loaded rooms therefore also means more of the world active, by the port's own rules.

**Runs (Cherry Tree Hills, police station entrance)**
- Original (no `config/qol.cfg`), heading 100: 7 rooms loaded, no request beyond them in 12 updates. Improved at x1.0, same pose: Room20 is added by the strip (nearest box point 164.98 m from the camera) and finishes loading at update 9. Improved at x20.0, heading 11.3: all 43 rooms of the level are loaded at update 9 (the nearest new room 114.8 m from the camera).
- Cost at x20 (heading 100, Improved x1.0 against x20.0): draw calls 1862 against 5983, GPU time of the level draw 0.46 against 0.55 ms, resident memory 288.2 against 286.1 MB (peak 288.2 and 286.1), frame time average 25.5 against 26.2 ms over the last second (heading 11.3: Original 2164 draws, 0.21 ms, 281.9 MB, 24.9 ms; x20 5983 draws, 0.61 ms, 285.0 MB, 25.1 ms). Memory does not grow because every room model is already in memory; the draw calls triple; the frame time stays within the noise of the runs (other builds shared the CPU, frame time 17 to 43 ms within one second in every run).

**Unknowns, last**
- The cost on a slower GPU or with the whole level in view (the measured poses face a wall or a sign; the port has no frustum culling, so the draw count is the same in every direction).
- Whether the original's streaming could hold 43 rooms at once is not relevant to the port, which keeps every room model; it is not measured.
