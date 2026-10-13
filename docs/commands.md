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
| `viewer-real-smoke`, `viewer-cutscene-smoke` | Render one model and one cutscene frame offscreen. Registered only if `artifacts/romfs` existed at configure time |
| `game-real-smoke` | Registered only if `artifacts/romfs` existed at configure time; needs libepoxy |
| `game-script-run`, `game-script-walk` | Same conditions. Feed a fixed stick to the development runtime and check how far the character moved |
| `camera-real-data` | Compares the follow camera with traces recorded from the original game (`artifacts/oracle/cam-out`, folder set with `-DCHASE_CAMERA_TRACE_DIR=...`); skips with code 77 if the traces are missing |
| `locomotion-real-data` | Compares the character's movement with traces recorded from the original game (`artifacts/oracle/loco-out`, folder set with `-DCHASE_LOCO_TRACE_DIR=...`); skips with code 77 if the traces or `artifacts/romfs` are missing |
| `collision-real-data` | Compares the character's floor height and wall sliding with traces recorded from the original game (`artifacts/oracle/loco-out` and `artifacts/oracle/coll-out`, folders set with `-DCHASE_LOCO_TRACE_DIR=...` and `-DCHASE_COLLISION_TRACE_DIR=...`); skips with code 77 if the traces or `artifacts/romfs` are missing |
| `soundtest-real-smoke`, `soundtest-id-smoke` | Play one sound by file name and one by number on the dummy audio driver. Same conditions |
| `audio-mixer` | Mixer core, no SDL |
| `runtime-core` | Game runtime core (frame timing, pad, stick, character movement, floor height and wall sliding), no SDL |
| `save-core` | Save file checks, synthetic data, no SDL |
| `save-real-data` | Completion totals from the game tables; needs `artifacts/romfs`, skips with code 77 otherwise |
| `fmv-real-smoke` | Registered only if FFmpeg and `artifacts/romfs` existed at configure time |

## Programs

```sh
build/chase-tabledump tables/tableslevels.bxls
build/chase-savecheck [--romfs artifacts/romfs/romfs] savegame.dat
build/chase-soundtest music/act_bass_break.bwav
build/chase-soundtest --id 5
SDL_VIDEODRIVER=offscreen build/chase-fmv --lang us --screenshot out.bmp --time 3 artifacts/romfs/romfs/cutscenes/01_gameintro.moflex
cd build && ./chase-input-lab [--config PATH] [--smoke-test]
```

- `chase-tabledump` prints data tables as TSV. Columns have no names in the file.
- `chase-savecheck` prints how much of each of the two save slots is complete, as items collected, per mille and percent. A slot with no progress shows as empty. It reads the totals from the game tables in the extracted game data (`--romfs`).
- `chase-soundtest` plays decoded sounds, by file name or by sound number (`--id`). Use `SDL_AUDIODRIVER=dummy` for headless runs. See [Sound test](sound-test.md).
- `chase-fmv` needs the FFmpeg libraries. See [Video player](fmv-player.md).
- `chase-input-lab` looks for `config/controls.cfg` relative to the current folder. CMake copies it into `build/`. See [Controls](controls.md).

## Development runtime

Needs libepoxy. This is **not the playable game**. It shows a level with the main character standing at the start point from the game data. The character walks, runs and sprints when you press the D-pad keys. Speed, turning and the animation changes follow what was measured in the original game. Everything else about the game is missing.

```sh
build/chase-game
build/chase-game --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl
SDL_VIDEODRIVER=offscreen build/chase-game --screenshot out.bmp --frames 2
SDL_VIDEODRIVER=offscreen build/chase-game --script "0 150 40" --screenshot out.bmp
```

- Options: `--fib`, `--names`, `--config` (as in the viewer), `--level`, `--screenshot`, `--frames`, `--script`.
- The default level is the police station. The character is Chase McCain in his police outfit. That choice is a best guess. Run it from the repository folder so the default paths work.
- Move with the D-pad keys (WASD or arrows). Hold `E` to walk slowly. Without it you run, and after about one second of running you sprint. `Z` and `C` turn the camera (the left and right shoulder buttons). Pushing sideways also turns the camera, as in the original. `Escape` quits.
- What moves like the original: the speeds, the walk, run and sprint switch, the delay before sprinting, and the way the character turns towards the direction you push.
- What is still a stand-in:
  - The camera follows the character like the original in open ground: distance, height, smoothing and turning match what was measured. The original also pushes the camera away from walls and from the character. That part is missing, so the camera can sit inside buildings. Rooms that use several camera zones are not handled.
  - The collision is only the level's fixed geometry. The character stays on the floor, climbs steps and kerbs by the original's rules, and slides along walls. Objects, other characters and vehicles do not block him, and he cannot fall or jump. Where the original lets him fall, he stays at his height.
  - The animations play at an assumed speed and switch without blending.
  - A keyboard cannot tilt the stick halfway, so `E` stands in for a gentle push.
- It prints one line per second: updates per second, the last time step and the circle pad values.
- `--screenshot` saves the picture after `--frames` updates (default 2) and exits.
- `--script "X Y COUNT;X Y COUNT"` ignores the keyboard and holds the stick at X, Y for COUNT updates of 0.033 s each (values like the 3DS circle pad: +X right, +Y up, 145 is full). For example `"0 150 40"` pushes forward for 40 updates. The program then prints how far the character moved and exits, saving the picture if `--screenshot` is given.

## Model viewer

Needs libepoxy. Use `SDL_VIDEODRIVER=offscreen` with `--screenshot` for headless checks. See [Model viewer](model-viewer.md).

```sh
build/chase-viewer --screenshot out.bmp models/lvl_00_policestation/room1.bfnmdl --exclude sky
build/chase-viewer --screenshot out.bmp models/char_bear/char_bear.bfnmdl --skeleton models/char_bear/char_bear.bfnskl --anim models/char_bear/walk.bfnanm --frame 12
build/chase-viewer --screenshot out.bmp --cutscene csdata/models/1_1_a --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --exclude sky --frame 200
```

- The second command poses the first model. `--frame N` fixes the frame.
- The third command builds the cast from `csdata/models/<id>/manifest.bstxt` and the camera from its ExportCam tracks.

## Original game in the emulator (oracle)

Needs the Azahar 3DS emulator in `PATH`, and Xvfb for `--display xvfb`. It runs your own game copy without a window, so developers can compare our code with the original game. It uses the only `.cci` file in the repository folder. Pass `--image FILE` if there are several.

```sh
python3 tools/azahar_oracle.py tools/oracle/boot.txt
```

- The script file lists steps: advance game frames, hold buttons, take screenshots, print timing values. The list of steps is at the top of `tools/azahar_oracle.py`.
- Screenshots go to `artifacts/oracle/out/`.
- `tools/oracle/camera-trace.txt` records the camera (`memtrace` step). Convert each `.mem` file with `tools/export_camera_trace.py` (it takes the pad state of that part as options) to get the `.csv` files the camera test reads.
- `tools/oracle/loco-*.txt` measure how the player moves. Each one starts with `loadstate gameplay-start` (a saved state that `tools/oracle/to-gameplay.txt` creates). Run them with `--display xvfb --renderer opengl --rpc --rpc-input`.
- The emulator uses its own settings and save folder in `artifacts/oracle/home/`. Your normal Azahar settings and saves are not used or changed. `--fresh` deletes that folder first.
- Stepping frames is slow: 600 frames take about 4 minutes. Waiting is faster: `wait 25` reaches the language screen in about 35 seconds.

### Working faster

Save a state once, then start from it. States need the `--display xvfb` option, which runs the emulator on a private virtual screen.

```sh
python3 tools/azahar_oracle.py tools/oracle/to-gameplay.txt --display xvfb --renderer opengl --keep   # ends with: savestate gameplay-start
python3 tools/azahar_oracle.py --stop
python3 tools/azahar_oracle.py my-test.txt --display xvfb --renderer opengl --state gameplay-start
```

- `savestate NAME` and `loadstate NAME` are steps in the script file. States are stored in `artifacts/oracle/states/` and survive `--fresh`.
- Loading takes about 15 seconds. Right after a start the emulator may refuse for a few seconds. The tool retries by itself.
- `--keep` leaves the emulator running after the script. `--attach` connects to it later (no game image needed). `--stop` closes it. Frame numbers start at 0 on every attach.

```sh
python3 tools/azahar_oracle.py my-test.txt --display xvfb --keep --port 24690
python3 tools/azahar_oracle.py more.txt --attach --port 24690
python3 tools/azahar_oracle.py --stop
```

- Do not trust timing values from a run with `--frame-limit 0` or from stepped frames. Details are in `docs/re/oracle.md`.

### Measuring while the game runs normally (`--rpc`)

Add `--rpc` to read the game's memory without stopping it. This is faster, and timing values stay correct.

```sh
python3 tools/azahar_oracle.py measure.txt --display xvfb --state gameplay-start --rpc
```

- Screenshots take well under a second instead of about 11 seconds.
- `sample SECONDS INTERVAL NAME` lets the game run freely and writes the game clock, the screen counters and the player position to `NAME.csv` at the given interval.
- `--rpc-input` presses buttons without stopping the game on every frame. It changes one instruction of the game in memory while the tool runs. The tool restores it before it leaves a kept emulator.
- Only one emulator at a time can use `--rpc`, because it always uses the network port 45987. The tool stops with a message if that port is taken.
- Without `--rpc` the tool works as before.
