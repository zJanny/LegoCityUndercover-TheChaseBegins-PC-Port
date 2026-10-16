# Sky: the sky pass, placement and depth

Reverse-engineering findings. Entries are chronological. Addresses are for the Europe v1.0 build. Feature file: [rendering](../plans/features/rendering.md). Related: [rendering-fog.md](rendering-fog.md), [materials.md](materials.md).

## Current state

- The sky pass is 0x180fac (called with r0 = 1 from 0x2e4980 and 0x2e4e34 after the colour clear, before the world draw 0x228d20) [§2026-10-15T15:39](#ts-20261015-153917).
- The camera matrix is copied with its translation row set to (0, lift, 0), so the dome follows the camera in position; lift = 20 x clamp(player y - y of the object "Sea", 0, 120) / 120, and 0 without a Sea object [§2026-10-15T15:39](#ts-20261015-153917).
- `props/prop_skybox` has no code of its own; its model is a dome of radius 99.94 with one mesh node, unlit and unfogged; the placement matrix does not put the dome around the camera [§2026-10-15T15:39](#ts-20261015-153917).
- Before the draw the pass writes depth entries `[0x52a138] = [0x52a13c] * -0.99951` and the same at 0x52a620, pushing the sky to the far end of the depth range [§2026-10-15T15:39](#ts-20261015-153917).
- `SkyboxOverride` is at 0x192254 and `SKY%02u` at 0x1813b4, which corrects the addresses in rendering-fog.md [§2026-10-15T15:39](#ts-20261015-153917).
- Port: `src/runtime/sky.*` draws the sky at (eye.x, eye.y - lift, eye.z) after the clear without depth test or write; the top-row colour check against the oracle is within 2 per channel [§2026-10-15T15:39](#ts-20261015-153917).
- Open: exact depth the sky gets; what `SkyboxOverride` does; the per-frame reader of `SetBackgroundColour`; which of two sky objects in lvl_06 is used; the uninitialised gate flag.

<a id="ts-20261015-153917"></a>

## 2026-10-15T15:39:17+02:00 — The sky pass 0x180fac: camera orientation with the eye at (0, lift, 0), drawn after the clear and before the world (llvm-objdump on `artifacts/game/game.arm.elf`, Azahar oracle `read` of memory in state `street-after-1-1`; REA was not used)

[Corrects rendering-fog.md §2026-10-14T18:59](rendering-fog.md#ts-20261014-185910): the string addresses of `SkyboxOverride` (0x192254) and `SKY%02u` (0x1813b4).

**Where the sky model lives**
- The object class `props/prop_skybox` has no code of its own; its `meshname` (SkyBox_Skyline_Cherry, _Albatross, _Auburn, _FortMeadows, _BlueBell, _Apollo, SkyBox_Satellite, Skybox_Skyline_Downtown) names a model with one mesh node (`SkyboxMesh`, `skymesh`, `Dome`), one part, textures `SkyBox_SkylineV2.tga` or `StarField.tga`, a dome of radius 99.94 (cherry: y -11.5 to 48.4, apollo and satellite reach down to -138 and -152). Parts are unlit, unfogged (material byte 0xb bit 6) and have depth test and write on.
- In the oracle the sky model hangs at `[[[0x3772d0] + 0x3c] + 0xc0]` (0x08b70070 in `street-after-1-1`, kind byte +8 = 2, node table at +0x14 with one entry named `SkyboxMesh`). The context object at `[[0x3772d0] + 0x3c]` holds the sky object's placement matrix at +0x1c (identity rotation, translation (4.5115, 16.8037, -29.3838) = the `Prop_SkyBox` position of lvl_01) and a (1, 1, 1) vector at +0xcc.
- Level placements (objectPlacements, same command as the test `sky-real-data`): lvl_01 (4.5, 16.8, -29.4), lvl_02 (751.5, 83.0, 1709.2), lvl_03 (953.3, 328.8, 270.2), lvl_04 (720.9, 62.4, -1305.3), lvl_05 (946.0, 422.6, -1817.3), lvl_06 two objects (`Sky` Apollo and `SkySatellite`, both near (-2610, 586, 990 / 1217)), lvl_07 (-668.0, 195.4, 412.8), lvl_00 none. The positions are far apart and far from the play area, so the placement matrix is not what puts the dome around the camera.

**The pass** (function 0x180fac, called with r0 = 1 from 0x2e4980 and 0x2e4e34, each right after the colour clear 0x25bed0 / 0x21d360 and before the world draw 0x228d20)
- It copies the camera matrix (object `[0x529e30 + 0x2c]`, matrix at +0x5c, copy 0x258858) to the stack, sets its translation row to (0, lift, 0) and installs it with 0x24a6cc. After the pass it restores the camera matrix through 0x24a554. So the dome is seen with the camera's orientation from a point at (0, lift, 0) over the model's own origin: it follows the camera in position.
- lift = 0x22a048(0, 120, d, 0, 20) with d = y of the player node (`[0x3772b8] -> [+0x3c]`, float at +0x34) minus y of the node of the object named "Sea" (found by name 0x23b350 with the string at 0x359b84): lift = 20 * clamp(d, 0, 120) / 120. Without a Sea object the lift stays 0 (lvl_00 has none; lvl_01 and lvl_02 have `Sea` at y = 0).
- Before the draw it writes the projection depth entries `[0x52a138] = [0x52a13c] * -0.99951`, `[0x52a620] = [0x52a624] * -0.99951` (literal 0xbf7ffd61 at 0x1813bc) and copies `[+0x318] = [+0x31c]`, `[0x52a630] = [0x52a634]`: a depth scale that pushes the sky to the far end of the depth range. The values at draw time were not captured (outside the pass all four read 0 or the saved values), so the exact depth the sky gets is not known.
- The pass orders the nodes of the sky model by the names `SKY00` to `SKY31` (format at 0x1813b4, compared on 5 characters) first and the rest after; the sky models shipped have no such node names, so the order is the model's.
- Gate: the lifted-camera path needs the word `[[0x3772d0] + 0x8a]` bits 1 and 2 to equal 1, or that word's bits 1 and 2 to be 0 with a stack flag set (the flag is not initialised in this function and was not traced). In the oracle the word was 1 and the picture shows the sky, so the path ran.

**Port** (`src/runtime/sky.*`, `Scene::drawSky`, `LevelSession::draw`): the first `props/prop_skybox` object's model is an instance marked sky, left out of the scene bounds and the normal draw. Per frame its transform is the translation (eye.x, eye.y - lift, eye.z), which gives the same view as the camera at (0, lift, 0) over the origin; it is drawn after the clear and before the level without depth test or depth write. The "sky" texture-name skip is gone, so the part `SkyBox_Skyline_02` of lvl_00 room1 (a backdrop x -218..454, y -9..115, z -466..1, unfogged, lighting 1) is an ordinary world part.

**Check** (`street-after-1-1` pose of [rendering-fog.md §2026-10-14T21:42](rendering-fog.md#ts-20261014-214220), 200 updates): top rows, oracle (153, 169, 192) at the top of the picture; port before this change (clear colour) (157, 170, 188), after (151, 169, 193). At a pose on the lvl_01 rooftop (472.6, 104, 874.6) the dome's clouds show and its lower rim shows against the clear colour; the original was not compared at that pose.

**Not read**
- `SkyboxOverride` (string 0x192254, not 0x392254) is a parameter of another object class: the constructor at 0x192008 reads it with the object parameter lookup 0x24242c into `[state + 0x78]` and calls 0x18be2c when it is set. What it does to the sky was not read.
- `SetBackgroundColour` (native at 0x291624) writes the colour bytes r, g, b (float to int) and 255 to offsets 4 to 7 of the singleton returned by 0x23878c, the same bytes the level setup 0x1bd054 writes from the level's BackgroundColour. The per-frame reader of those bytes was not found.
- Which of two sky objects (lvl_06) the original uses, and the exact depth of the sky (the 0.99951 scale).
