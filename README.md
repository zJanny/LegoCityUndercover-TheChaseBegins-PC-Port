# The Chase Begins — Linux reconstruction (work in progress)

This is the start of a Linux x86_64 reconstruction of *LEGO City Undercover: The Chase Begins* (Nintendo 3DS). **The game is not playable on PC.** The repository contains:

- `tools/extract_game.py` takes a decrypted `.cci` image you supply. It reads the image without modifying it and writes the executable plus an analysis manifest.
- `chase-input-lab` is a native SDL2 tool for developing and testing keyboard/mouse controls. It is not the game.

Game data is never stored in this repository. Supply your own decrypted image.

## Extract the executable

Requires Python 3.11 or newer.

```sh
python3 tools/extract_game.py path/to/game.cci --output artifacts/game
```

The output directory must not exist yet. The tool refuses to overwrite.

The tool fails with an error on encrypted, truncated or hash-mismatched images. On success it writes the following files:

| File | Contents |
|---|---|
| `manifest.json` | offsets, SHA256 hashes, ARM segment addresses |
| `exheader.bin` | extended header |
| `exefs/*.bin` | ExeFS members |
| `code.bin` | decompressed ARM code |
| `game.arm.elf` | a synthetic ELF wrapper for disassemblers; not an original or runnable binary |

## Extract the RomFS (game assets)

```sh
python3 tools/extract_romfs.py path/to/game.cci --output artifacts/romfs
```

Before writing anything, the tool verifies the RomFS superblock hash from the NCCH header and the full IVFC hash tree. It then writes `romfs/` and `romfs-manifest.json`, which records each file's image offset, size and SHA256. Like the executable extractor, it refuses an existing output directory and rejects unsafe or duplicate names, cycles and out-of-bounds entries.

## Unpack the asset archive

Almost every asset except the `.moflex` cutscene videos is inside `romfs/lego_city3ds.fib`, a TT Fusion `FUSE1.00` archive. The format and its compression (a RefPack variant) were recovered from the game's ARM code.

```sh
python3 tools/extract_fib.py artifacts/romfs/romfs/lego_city3ds.fib \
    --names artifacts/romfs/romfs/lego_city3ds.csv --output artifacts/fib
```

The tool writes `files/<path>` and `fib-manifest.json`. It cross-checks every CSV entry against the archive directory, and any entry missing from the CSV goes to `files/unnamed/<crc>.bin`. Decoding is bounds-checked, and every entry must decode to exactly its declared size. Codec 3 is reported as unsupported because no shipped entry uses it.

## Convert textures to PNG

`.btga`, `.bpng` and `.bdds` share one 56-byte header followed by PICA200 Morton-tiled pixel data. Supported formats: RGBA8, RGB8, RGBA5551, RGB565, RGBA4, LA8, HILO8, L8, A8, LA4, L4, A4, ETC1 and ETC1A4.

```sh
python3 tools/convert_textures.py --census artifacts/fib/files        # validate headers only
python3 tools/convert_textures.py artifacts/fib/files --output artifacts/textures
```

Only the largest mip level is written. Cubemaps are written as a vertical strip of six faces in stored order; which face is which has not been confirmed yet.

## Convert sounds to WAV

`.bwav` files wrap standard Nintendo CWAV data, and every shipped sound is DSP-ADPCM, mostly at 12 kHz. Music files hold one mono CWAV per channel, and the tool merges them into a single WAV.

```sh
python3 tools/convert_audio.py --census artifacts/fib/files            # parse headers only
python3 tools/convert_audio.py artifacts/fib/files --output artifacts/audio
```

`audio-manifest.json` keeps each sound's sample rate, sample count and loop points.

## Inspect models

All `b*` resources share one pooled block layout (`tools/tt_resource.py`). `tools/convert_models.py` parses `.bfnmdl` models by replaying the game's deserializer block by block. It checks that every block is consumed and that the vertex stride derived from the material matches the stored one.

```sh
python3 tools/tt_resource.py --census artifacts/fib/files                 # pool layout check, all b* files
python3 tools/convert_models.py --census artifacts/fib/files              # all models, stride check
python3 tools/convert_models.py --vertices 3 path/to/model.bfnmdl         # raw vertex dump
python3 tools/preview_model.py path/to/model.bfnmdl --assets artifacts/fib/files --output out.png [--exclude sky]
```

`preview_model.py` is a small software renderer for checking the decoders visually. Node and mesh transforms, skinning and animation are not recovered yet.

## Native asset runtime

`chase_assets` (`src/assets/`) is the start of the native port's asset layer, written in C++17 with no SDL dependency. It reads the FUSE archive, including its RefPack chunks, parses pooled resources and decodes textures to RGBA. `ctest` runs it against synthetic fixtures (`assets-core`).

If you have extracted the RomFS to `artifacts/romfs`, `assets-real-data` also decodes every archive entry and texture. It cross-checks texture checksums against the Python decoder. Without the extracted data, the test is reported as skipped. Set `-DCHASE_ROMFS_DIR=...` to point it elsewhere.

## Development model viewer

`chase-viewer` loads `.bfnmdl` models straight from the archive and draws them with their textures. With `--level` it also places every object from a `.blvl` level file that names a mesh. It uses OpenGL 3.3 through libepoxy and is only built if libepoxy is found. It is a tool for checking decoded assets. It is **not** the game: there is no gameplay, no animation, no lighting or material combiners, and no characters.

```sh
build/chase-viewer --level levels/levels/lvl_00_policestation/lvl_00_policestation_main.blvl \
    --room room3 --brightness 2.5 models/lvl_00_policestation/room3.bfnmdl
```

- `--room` limits placements to one room.
- `--cull ccw|cw|none` sets the face winding. The default is `ccw`, which culls ceilings so you can look into interiors.
- `--brightness` only helps inspection, because the game's lighting setup is not recovered.
- `--exclude WORD` skips parts whose texture name contains WORD (e.g. `sky`).

The viewer uses the same `config/controls.cfg` bindings as the lab:
- **D-pad:** move.
- **L / R:** lower / raise.
- **X / Y:** turn.
- **Left-drag:** look around.
- **Escape:** quit.

Other options:
- **`--screenshot OUT.bmp`:** renders one frame, then exits. With `SDL_VIDEODRIVER=offscreen` this works without a display.
- **`--fib` / `--names`:** default to `artifacts/romfs/romfs/`.

## Sound test

`chase-soundtest` decodes `.bwav` sounds from the archive and plays them through the native mixer (`chase_audio`, `src/audio/`) and SDL audio. It honours loop points: a looping sound plays for 10 s unless you pass `--seconds`.

```sh
build/chase-soundtest music/act_bass_break.bwav sounds/lego_heart_collected_01.bwav
```

How the game routes sounds to events is not recovered yet.

## Build and run the input laboratory

Requires CMake ≥ 3.16, a C++17 compiler and SDL2 (found through pkg-config).

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
cd build && ./chase-input-lab            # or: --config PATH, --smoke-test
```

The window shows every 3DS button's state, the D-pad axes and a lower-screen touch area. Press Escape to quit.

## Controls

Edit `config/controls.cfg`. Each line has the form `Action key|mouse Code hold|toggle`.

- **Actions:** Up, Down, Left, Right, A, B, X, Y, L, R, Start, Select.
- **Key names:** SDL names, with `_` in place of spaces (for example `Left_Shift`).
- **Mouse buttons:** `Left`, `Middle`, `Right`, `X1`, `X2`.

How bindings behave:

- **Several bindings for one action:** they combine.
- **`hold`:** the action is active while the input is held.
- **`toggle`:** each fresh press latches the action on or off.
- **Left and Right, or Up and Down, together:** opposite directions cancel.
- **Window loses focus:** everything is released.
- **Left mouse button:** acts as the 3DS touch screen inside the lower-screen area.

Default bindings:

| Button | Default |
|---|---|
| D-pad | WASD or the arrow keys |
| A | Space or left mouse |
| B | Q or right mouse |
| X | E |
| Y | R |
| L | Z |
| R | C |
| Start | Return |
| Select | Tab |

## Python tests

```sh
python3 -m unittest discover -s tests -p 'test_*.py' -v
```

## Status

The plan and the evidence log are in `docs/plans/linux-port.md` and `docs/plans/linux-port.progress.md`. Not yet recovered:

- gameplay logic
- rendering
- audio
- individual asset formats (the `.fib` archive is unpacked and textures and sounds convert to PNG and WAV, and model geometry decodes, but transforms, skeletons (`.bfnskl`), animation (`.bfnanm`), `.blvl`, `.moflex` and the rest are not decoded)
- the save format
