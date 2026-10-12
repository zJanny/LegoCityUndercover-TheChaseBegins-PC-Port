# Command reference

Commands for the extraction tools, converters and programs. For build and test setup, see [Building](building.md).

Let `CCI` be the image file name in the repository root:

`LEGO City Undercover - The Chase Begins (Europe) (En,Fr,De,Es,It,Nl,Pt,Da,Ru).cci`

## Extraction and conversion

The output folder must not exist yet.

```sh
python3 tools/extract_game.py "$CCI" --output artifacts/game
python3 tools/extract_romfs.py "$CCI" --output artifacts/romfs
python3 tools/extract_fib.py artifacts/romfs/romfs/lego_city3ds.fib --names artifacts/romfs/romfs/lego_city3ds.csv --output artifacts/fib
python3 tools/convert_textures.py artifacts/fib/files --output artifacts/textures
python3 tools/convert_audio.py artifacts/fib/files --output artifacts/audio
```

- `convert_textures.py --census` and `convert_audio.py --census` check headers only.
- `python3 tools/tt_resource.py --census artifacts/fib/files` checks the pooled resource layout.
- `python3 tools/convert_models.py --census artifacts/fib/files` checks model parsing and strides.
- `python3 tools/preview_model.py MODEL.bfnmdl --assets artifacts/fib/files --output out.png` renders a model preview.

See [Extracting game data](extracting-game-data.md) for details.

## Tests

```sh
python3 -m unittest discover -s tests -p 'test_*.py' -v
python3 -m unittest discover -s tests -p 'test_*.py' -k test_hash_corruption
ctest --test-dir build --output-on-failure
```

Single C++ tests, with `ctest --test-dir build -R <name>`:

| Name | Notes |
|---|---|
| `input-core` | Input core, no SDL |
| `input-sdl-smoke` | Runs with `SDL_VIDEODRIVER=dummy` |
| `assets-core` | Asset runtime, synthetic fixtures |
| `assets-real-data` | Needs `artifacts/romfs`; skips with code 77 otherwise |
| `viewer-real-smoke` | Registered only if `artifacts/romfs` existed at configure time |
| `audio-mixer` | Mixer core, no SDL |
| `save-core` | Save file checks, synthetic data, no SDL |
| `fmv-real-smoke` | Registered only if FFmpeg and `artifacts/romfs` existed at configure time |

## Programs

```sh
build/chase-tabledump tables/tableslevels.bxls
build/chase-soundtest music/act_bass_break.bwav
SDL_VIDEODRIVER=offscreen build/chase-fmv --lang us --screenshot out.bmp --time 3 artifacts/romfs/romfs/cutscenes/01_gameintro.moflex
cd build && ./chase-input-lab [--config PATH] [--smoke-test]
```

- `chase-tabledump` prints data tables as TSV. Columns have no names in the file.
- `chase-soundtest` plays decoded sounds. Use `SDL_AUDIODRIVER=dummy` for headless runs. See [Sound test](sound-test.md).
- `chase-fmv` needs the FFmpeg libraries. See [Video player](fmv-player.md).
- `chase-input-lab` looks for `config/controls.cfg` relative to the current folder. CMake copies it into `build/`. See [Controls](controls.md).

## Model viewer

Needs libepoxy. Use `SDL_VIDEODRIVER=offscreen` with `--screenshot` for headless checks. See [Model viewer](model-viewer.md).

```sh
build/chase-viewer --screenshot out.bmp models/lvl_00_policestation/room1.bfnmdl --exclude sky
build/chase-viewer --screenshot out.bmp models/char_bear/char_bear.bfnmdl --skeleton models/char_bear/char_bear.bfnskl --anim models/char_bear/walk.bfnanm --frame 12
build/chase-viewer --screenshot out.bmp --cutscene csdata/models/1_1_a --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --exclude sky --frame 200
```

- The second command poses the first model. `--frame N` fixes the frame.
- The third command builds the cast from `csdata/models/<id>/manifest.bstxt` and the camera from its ExportCam tracks.
