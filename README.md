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
- individual asset formats (the `.fib` archive is unpacked and textures convert to PNG, but `.bfnmdl`, `.bfnanm`, `.bwav`, `.blvl`, `.moflex` and the rest are not decoded)
- the save format
