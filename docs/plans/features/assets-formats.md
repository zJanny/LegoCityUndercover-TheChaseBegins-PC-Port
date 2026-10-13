# Asset formats and decoders (S4a, S4c)

## 1. Scope

Every format decoder in `chase_assets` (namespace `chase::assets`, no SDL): RomFS and the FUSE `.fib` archive with RefPack, pooled `b*` resources, textures, `.bwav` sounds, `.bfnmdl` models, `.blvl` levels with placements, `.bfnnav` nav grids, `.loc` text and `.tnf` fonts, `.bxls` tables, brace-block config text (`.cam`, trigger and option lists). Skeletons and animation are [animation-characters.md](animation-characters.md); particles, [particles.md](particles.md); `.bxaml`, [hud-ui.md](hud-ui.md); sounds, [audio.md](audio.md); saves, [saves-options.md](saves-options.md).

## 2. Status

IN_PROGRESS, updated 2026-10-13T21:29:01+0200. Decoders exist and are checked against every shipped file; level composition (materials, lighting, streaming, other sections) is incomplete.

## 3. Verified behaviour

- RomFS extraction; FUSE archive 12836 entries, 478387766 bytes; 12500 pooled resources; 3991 textures (all 14 PICA formats, ETC1/ETC1A4, cubemap strip) with FNV-1a checksums; 1361 WAVs; 66 localisation tables; 856 levels (`assets-real-data`).
- Texture v orientation corrected; placement Euler order and the mesh-centre rule; the game world is left-handed (`chase_render` perspective mirrors x).
- Python converters are thin tools over the C ABI (`chase_assets_c`, `tools/chase_native.py`).

## 4. Stand-ins

- ASSET-S1 Moved to [rendering.md](rendering.md) (RENDER-S1 to RENDER-S4).
- ASSET-S2 Levels are composed from the archive's folder names (`models/<level name>/`); how the game streams the pieces is not known.

## 5. Open items

- [ ] ASSET-1 — Moved to [rendering.md](rendering.md) (materials recovered; lighting RENDER-1, reflections RENDER-2, fog RENDER-3).
- [ ] ASSET-2 — Streaming (`Pregen/*.bin`) and the other `.blvl` sections. evidence: [levels.md §2026-10-12T17:20](../../re/levels.md).
- [ ] ASSET-3 — Several smaller file types are not decoded.
- [ ] ASSET-4 — `.cam` record selection details: first-gather values and pose blend rooms are open (see CAMERA-3); the `.cam` fields RotationY/X, Distance and Lookat are editor-only. evidence: [data-tables-config.md §2026-10-13T09:10](../../re/data-tables-config.md).

## 6. Findings summary

- Container, RomFS, FUSE: [container-and-archive.md §2026-10-12T15:32](../../re/container-and-archive.md); pooled layout: [pooled-resources.md §2026-10-12T15:55](../../re/pooled-resources.md).
- Textures: [textures.md §2026-10-12T15:40](../../re/textures.md), [textures.md §2026-10-12T22:40](../../re/textures.md); models: [models.md §2026-10-12T16:10](../../re/models.md).
- Levels, placement, nav grids, handedness: [levels.md §2026-10-12T17:20](../../re/levels.md), [levels.md §2026-10-12T23:05](../../re/levels.md), [levels.md §2026-10-12T23:55](../../re/levels.md), [levels.md §2026-10-13T12:49](../../re/levels.md).
- Text and fonts: [text-localisation-fonts.md §2026-10-13T00:25](../../re/text-localisation-fonts.md); tables and config text: [data-tables-config.md §2026-10-13T03:50](../../re/data-tables-config.md), [data-tables-config.md §2026-10-13T10:15](../../re/data-tables-config.md).

## 7. Code and tests

- `src/assets/` (`fuse.cpp`, `pooled.cpp`, `texture.cpp`, `audio.cpp`, `model.cpp`, `level.cpp`, `nav.cpp`, `text.cpp`, `table.cpp`, `config_text.cpp`, `c_api.cpp`).
- CTest: `assets-core` (synthetic), `assets-real-data` (skips with 77 when the RomFS is absent).
