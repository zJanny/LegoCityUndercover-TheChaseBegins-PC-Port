# Asset formats and decoders (S4a, S4c)

## 1. Scope

Every format decoder in `chase_assets` (namespace `chase::assets`, no SDL): RomFS and the FUSE `.fib` archive with RefPack, pooled `b*` resources, textures, `.bwav` sounds, `.bfnmdl` models, `.blvl` levels with placements, `.bfnnav` nav grids, `.loc` text and `.tnf` fonts, `.bxls` tables, brace-block config text (`.cam`, trigger and option lists). Skeletons and animation are [animation-characters.md](animation-characters.md); particles, [particles.md](particles.md); `.bxaml`, [hud-ui.md](hud-ui.md); sounds, [audio.md](audio.md); saves, [saves-options.md](saves-options.md).

## 2. Status

IN_PROGRESS, updated 2026-10-16T10:12:28+02:00. Decoders exist and are checked against every shipped file; level composition (materials, lighting, streaming, other sections) is incomplete. The collectables and object-count pregen files decode (`pregen.cpp`), the script-list files parse with the original reader's tolerance, and `options.txt` / `options3ds.txt` are unused by the game. Left in ASSET-3: customchar, traffic, pedestrians, map text files; meaning of the pregen values: ASSET-8.

## 3. Verified behaviour

- RomFS extraction; FUSE archive 12836 entries, 478387766 bytes; 12500 pooled resources; 3991 textures (all 14 PICA formats, ETC1/ETC1A4, cubemap strip) with FNV-1a checksums; 1361 WAVs; 66 localisation tables; 856 levels (`assets-real-data`).
- Texture v orientation corrected; placement Euler order and the mesh-centre rule; the game world is left-handed (`chase_render` perspective mirrors x).
- Python converters are thin tools over the C ABI (`chase_assets_c`, `tools/chase_native.py`).

## 4. Stand-ins

- ASSET-S1 Moved to [rendering.md](rendering.md) (RENDER-S2 to RENDER-S4; RENDER-S1 was removed with RENDER-1).
- ASSET-S2 Levels are composed from the archive's folder names (`models/<level name>/`). The original streams rooms of a world level inside a master level: see WSTREAM-S1 in [world-streaming.md](world-streaming.md).

## 5. Open items

- [ ] ASSET-1 — Moved to [rendering.md](rendering.md) (materials recovered; lighting RENDER-1, reflections RENDER-2, fog RENDER-3).
- [ ] ASSET-2 — The other `.blvl` sections: +0x1c event bindings and +0x34 room links are decoded; +0x20 and the +0x24 byte tuples remain (WSTREAM-5). `pregen/*.bin` is not streaming (traffic, pedestrians, collectables); its formats are undecoded. Streaming itself: [world-streaming.md](world-streaming.md). evidence: [streaming.md §2026-10-13T22:27](../../re/streaming.md#ts-20261013-222700).
- [ ] ASSET-3 — Several smaller file types are not decoded. Left: `customchar.bin`, the traffic and pedestrian `.bin` files and the map `.txt` files (ASSET-5 to ASSET-7 are closed; ASSET-8 holds the pregen meanings).
- [ ] ASSET-4 — `.cam` record selection details: first-gather values and pose blend rooms are open (see CAMERA-3); the `.cam` fields RotationY/X, Distance and Lookat are editor-only. evidence: [data-tables-config.md §2026-10-13T09:10](../../re/data-tables-config.md#ts-20261013-0910).
- [ ] ASSET-8 — The meaning of the three collectable groups (which collectable kind each is) and of the 82 `levelobjectscount.bin` values is not known, and the port does not use the files yet; the only reader of the count file discards what it reads (gate: flags bits 3 / 9 at [0x375b08]). Needed: the callers of 0x239184 results (counters at 0x5208e0+0x2d..0x32) in the HUD / status screen. evidence: [data-tables-config.md §2026-10-16T10:12:03](../../re/data-tables-config.md#ts-20261016-101203).

## 6. Findings summary

- Container, RomFS, FUSE: [container-and-archive.md §2026-10-12T15:32](../../re/container-and-archive.md#ts-20261012-153232); pooled layout: [pooled-resources.md §2026-10-12T15:55](../../re/pooled-resources.md#ts-20261012-1555).
- Textures: [textures.md §2026-10-12T15:40](../../re/textures.md#ts-20261012-1540), [textures.md §2026-10-12T22:40](../../re/textures.md#ts-20261012-2240); models: [models.md §2026-10-12T16:10](../../re/models.md#ts-20261012-1610).
- Levels, placement, nav grids, handedness: [levels.md §2026-10-12T17:20](../../re/levels.md#ts-20261012-1720), [levels.md §2026-10-12T23:05](../../re/levels.md#ts-20261012-2305), [levels.md §2026-10-12T23:55](../../re/levels.md#ts-20261012-2355), [levels.md §2026-10-13T12:49](../../re/levels.md#ts-20261013-1249).
- Text and fonts: [text-localisation-fonts.md §2026-10-13T00:25](../../re/text-localisation-fonts.md#ts-20261013-0025); tables and config text: [data-tables-config.md §2026-10-13T03:50](../../re/data-tables-config.md#ts-20261013-0350), [data-tables-config.md §2026-10-13T10:15](../../re/data-tables-config.md#ts-20261013-1015).

## 7. Code and tests

- `src/assets/` (`fuse.cpp`, `pooled.cpp`, `texture.cpp`, `audio.cpp`, `model.cpp`, `level.cpp`, `nav.cpp`, `text.cpp`, `table.cpp`, `config_text.cpp`, `pregen.cpp`, `c_api.cpp`).
- CTest: `assets-core` (synthetic), `assets-real-data` (skips with 77 when the RomFS is absent).
