# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Early-stage Linux x86_64 reconstruction of *LEGO City Undercover: The Chase Begins* (Nintendo 3DS) from the decrypted `.cci` image in the repo root. Right now there are two independent pieces:

1. **Extraction pipeline** (`tools/extract_game.py`, Python 3.11+ stdlib only). It reads the NCSD/CCI image, then the NCCH partition 0, the exheader and the ExeFS, and checks every SHA256. It decompresses `.code` (backwards LZ) and wraps it in a synthetic ELF32 ARM file (`game.arm.elf`) for disassembly in REA.
2. **Native input laboratory** (`src/`, C++17 + SDL2). It tests configurable keyboard/mouse input that maps onto 3DS buttons and the 320x240 lower touch screen. It is **not** the game runtime. Gameplay, rendering, audio, assets and saves have not been recovered yet.

The plan and its progress log live in `docs/plans/linux-port.md` and `docs/plans/linux-port.progress.md`, with steps S1–S4. Read both before you resume port work. Update the progress file with the commands, results and timestamps from each session. The plan does not permit committing, pushing or merging unless the user asks.

## Commands

```sh
# Python extractor tests (synthetic fixtures; no game image needed)
python3 -m unittest discover -s tests -p 'test_*.py' -v
# single Python test
python3 -m unittest discover -s tests -p 'test_*.py' -k test_hash_corruption

# Extract the real image (output dir must NOT already exist)
python3 tools/extract_game.py "LEGO City Undercover - The Chase Begins (Europe) (En,Fr,De,Es,It,Nl,Pt,Da,Ru).cci" --output artifacts/game
# Extract RomFS assets (verifies superblock + IVFC tree; same no-overwrite rule)
python3 tools/extract_romfs.py "LEGO City Undercover - The Chase Begins (Europe) (En,Fr,De,Es,It,Nl,Pt,Da,Ru).cci" --output artifacts/romfs
# Unpack the FUSE asset archive (RefPack-variant codec recovered from 0x12efe0)
python3 tools/extract_fib.py artifacts/romfs/romfs/lego_city3ds.fib --names artifacts/romfs/romfs/lego_city3ds.csv --output artifacts/fib
# Textures (.btga/.bpng/.bdds, PICA200 formats) -> PNG; --census validates headers only
python3 tools/convert_textures.py artifacts/fib/files --output artifacts/textures
# Sounds (.bwav = wrapped CWAV, DSP-ADPCM) -> WAV; --census parses headers only
python3 tools/convert_audio.py artifacts/fib/files --output artifacts/audio
# Pooled b* resource layout check; model parse/stride census; model preview render
python3 tools/tt_resource.py --census artifacts/fib/files
python3 tools/convert_models.py --census artifacts/fib/files
python3 tools/preview_model.py MODEL.bfnmdl --assets artifacts/fib/files --output out.png

# Native build + tests
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
ctest --test-dir build -R input-core          # pure input-core unit test
ctest --test-dir build -R input-sdl-smoke     # SDL integration, runs with SDL_VIDEODRIVER=dummy
ctest --test-dir build -R assets-core         # native asset runtime, synthetic fixtures
ctest --test-dir build -R assets-real-data    # opt-in: needs artifacts/romfs (skips with code 77 otherwise)

# Run the lab (looks for config/controls.cfg relative to cwd; CMake copies it into build/)
cd build && ./chase-input-lab [--config PATH] [--smoke-test]
```

## Architecture notes

- **The input core has no SDL dependency.** `chase_input` (`src/input.{hpp,cpp}`) takes plain `(Source, code, down, repeat)` events. `readBindings` takes a key-name resolver callback, and `main.cpp` passes in `SDL_GetKeyFromName`. Keep SDL out of `input.cpp` so that `input-test` links without SDL.
- Input rules, which `tests/input_test.cpp` and the `--smoke-test` path in `main.cpp` both check:
  - A hold action stays active while any of its bindings is held.
  - A toggle action flips only on a fresh press, and key repeats are ignored.
  - Opposite directions cancel each other in `horizontal()`/`vertical()`.
  - Focus loss resets all held and toggled state.
  - Touch maps only inside the aspect-fitted 320x240 viewport.
  
  When you change behaviour, update both tests.
- **The native asset runtime is `chase_assets`** (`src/assets/`, namespace `chase::assets`). It has no SDL dependency. It ports the verified Python tools: FUSE archive + RefPack, the pooled reader with `PoolCursor`, and PICA texture decode. The Python tools in `tools/` are the reference. When you change a format decoder, change both, and keep `assets-real-test`'s checksums (from `convert_textures.py --checksum`) in agreement.
- `config/controls.cfg` format: `Action key|mouse Code hold|toggle`, with `#` comments. Key names are SDL names with `_` in place of spaces. Mouse codes are `Left`, `Middle`, `Right`, `X1` and `X2`. Action names are 3DS buttons, not recovered gameplay meanings.
- **Extractor safety rules:**
  - Open the image read-only.
  - Bounds-check every offset.
  - Fail on encryption or a hash mismatch.
  - Cap decompressed output at `MAX_CODE`.
  - Never overwrite an existing output directory.

  `manifest.json` records offsets, hashes and ARM segment addresses as evidence. The ELF is a derived analysis wrapper: its entry point is inferred from the text base, and it is not an original binary.
- REA cannot open the `.cci` directly. Analyse `artifacts/<dir>/game.arm.elf` instead, and record REA evidence IDs in the progress file.

## Constraints

- Never modify the `.cci` image. Never stage or commit it, or anything game-derived (`*.cci`, `*.3ds`, `artifacts/`, `build/` are gitignored).
- Record what is unknown instead of inventing game behaviour. Don't size native pointers from ARM addresses. Never describe the lab as a playable port.

## User specified workflows

1. Please commit at logical steps!
2. Only push when the user wishes to!
3. Always keep the .gitignore up to date.
