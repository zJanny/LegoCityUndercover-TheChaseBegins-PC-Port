# Textures and graphics API

Reverse-engineering findings, moved verbatim from the progress log. Entries are chronological; later entries may correct earlier ones. Evidence IDs are shortened with an ellipsis.

<a id="ts-20261012-1540"></a>

## 2026-10-12T15:40+02:00 — Texture container decoded (.btga/.bpng/.bdds)

- Header layout, recovered from shipped files (not yet traced in code):
  - 56 bytes: constants 0x400, 0x10, -14, 0x20, -14
  - u32 data size; u16 width and u16 height; u32 data size again
  - u8 PICA200 format; 2 zero bytes; u8 cubemap flag; u32 mip count; 16 zero bytes
  - Data: the mip chain, largest level first, 8×8 Morton-tiled and stored bottom-up. A cubemap holds six chains.
- Added tools/convert_textures.py (all 14 PICA formats; ETC1/ETC1A4 decode 4×4 blocks in tile order (0,0),(4,0),(0,4),(4,4); u64 LE words; column-major pixel index) and tests/test_convert_textures.py (6 tests).
- `python3 tools/convert_textures.py --census artifacts/fib/files` — 3991/3991 headers valid. Each declared size equals the computed mip-chain size × faces. Counts: btga ETC1 1443, ETC1A4 985, L8 452, LA8 267, RGBA8 107, RGB8 68, A8 11, RGBA8 cube 21; bpng RGBA8 497, ETC1A4 70, LA8 10, A8 9, RGB8 9, ETC1 5, L8 2; bdds ETC1A4 22, ETC1 11, LA8 2.
- `python3 tools/convert_textures.py artifacts/fib/files --output artifacts/textures` — PASS, 3991 PNGs, 0 failures, 80 s.
- Visual checks (viewed PNGs):
  - pc01_lombardstreet postcard: ETC1
  - saveicon "POLICE" sign upright: ETC1, confirms the bottom-up flip
  - ui_shop_codes icon: RGBA8
  - atlas_lvl_00_policestation_main "CITY TV" atlas: ETC1A4 with alpha
  - ui_ls_generic1: RGB8, a 400×240 image in a 512×256 texture (the top-screen size)
  - cubemap_showroom_c: six plausible faces; face order unconfirmed
- `python3 -m unittest discover -s tests -p 'test_*.py' -v` — PASS 26/26.

<a id="ts-20261012-2240"></a>

## 2026-10-12T22:40+02:00 — Texture v orientation corrected (most "black material" cases)

- Corrects the S4b viewer code comment (`src/viewer.cpp`, commit 9e143f0) "GL's first row is v=0, which the game also treats as the bottom": UV v=0 is the TOP row of the decoded (top-down) image.
  - The decoder itself is right (the splash1top text reads upright).
  - char_squirrel: the body UVs (v 0.47–0.99) must hit the orange lower half of SquirrelALPHAT64, and the eye part (v 0.02–0.54) the eye decal in the upper half. With the old convention both were swapped, so the body sampled the black decal area.
  - pickup_chasetoken_imposter: the minifig face is upside down under the old convention and upright under the new one.
- chase-viewer uploads decoded rows unflipped. preview_model samples row v·h.
- Result: char_squirrel renders orange with black eyes. lvl_00_policestation room3 shows the textured floor, emblem, plants and lightmap shading where large areas were black before. The scene is still dim; the combiner/lighting setup is unrecovered.
- Graphics API, from strings: the game links Nintendo DMPGL (a dmp_* reserved-uniform table at 0x35b288, 0x10-byte entries {name, id, count, GL type}). The dmp_TexEnv names are referenced only from that table, and no static PICA TEV command headers (regs 0xc0–0xf7) exist in code.bin, so the combiner setup must be traced through the game's uniform-location usage. Not done.
- ctest PASS 7/7.

<a id="ts-20261013-150853"></a>

## 2026-10-13T15:08:53+02:00 — convert_textures.py decodes through C++; --checksum removed

- tools/convert_textures.py now writes the PNGs from `chase::assets::decodeTexture` through tools/chase_native.py. The output of all 3991 textures is byte-identical to the earlier Python decoder. The `--checksum` option was removed because it would compare the C++ decoder with itself.
- The FNV-1a reference values in tests/assets_real_test.cpp stay valid. Provenance: `python3 tools/convert_textures.py --checksum <file>` (the Python decoder) at 9d04c91. To reproduce, check out that commit (for example `git worktree add /tmp/old 9d04c91`) and run the command there.
