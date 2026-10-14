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

- `convert_textures.py --census` and `convert_audio.py --census` decode every file and write nothing.

See [Extracting game data](extracting-game-data.md) for details.

## Tests

```sh
python3 -m unittest discover -s tests -p 'test_*.py' -v
python3 -m unittest discover -s tests -p 'test_*.py' -k test_hash_corruption
ctest --test-dir build --output-on-failure
```

## Documentation links

Run this before you commit any Markdown change. It checks every link between the Markdown files and every `#anchor`. `--fix` adds the anchor line to each dated entry in `docs/re/` and points every `§<timestamp>` link at its entry. Running it twice changes nothing.

```sh
python3 tools/check_doc_links.py          # list broken links and missing anchors
python3 tools/check_doc_links.py --fix    # repair what can be repaired mechanically
```

## Progress report

```sh
python3 tools/progress_report.py                            # short table
python3 tools/progress_report.py --json artifacts/progress.json
python3 tools/progress_report.py --update-readme README.md  # rewrite the progress table in the README
python3 tools/progress_report.py --check README.md          # fails when the README table is out of date
```

Needs the built `build/libchase_assets_c.so` and the extracted game data. See [Progress](progress.md).

## Merging an agent branch

```sh
python3 tools/merge_agent.py prepare BRANCH          # squash the branch into main's staging area, then run the checks
python3 tools/merge_agent.py check                   # run the checks again after fixing conflicts or review edits
python3 tools/merge_agent.py finish BRANCH -m FILE   # checks, commit with the message in FILE, remove worktree and branch
```

The checks stop on game files, screenshots or other binary files, broken doc links, build warnings and failing tests. They also refresh the README progress table. The message file must end with the merging agent's own `Co-Authored-By:` line. The script never pushes.

Single C++ tests, with `ctest --test-dir build -R <name>`:

| Name | Notes |
|---|---|
| `input-core` | Input core, no SDL, including the `DebugOverlay` binding |
| `debug-metrics-core` | [Debug overlay](debug-overlay.md) values on synthetic input: frame-time window, `/proc/self/status` fixture, CPU percent, overlay lines |
| `debug-text-core` | Debug overlay font and panel raster |
| `debug-overlay-off`, `debug-overlay-on`, `debug-overlay-area` | Two offscreen screenshots without and with `--debug-overlay`; the third checks that they differ only in the top-left panel area. Registered only if `artifacts/romfs` existed at configure time |
| `script-text`, `script-text-real-data` | Script text natives (tip text, dialogue lines, mission title) on synthetic scripts and on `1-1_OnScreenText_Script` of the real level; the second skips with code 77 without `artifacts/romfs` |
| `tip-text-shot`, `tip-text-panel` | An offscreen screenshot of the first tip of mission 1-1 and a check of the tip panel (`tools/check_tip_shot.py`). Registered only if `artifacts/romfs` existed at configure time |
| `police-entrance-shot`, `police-entrance-no-disc` | An offscreen screenshot at the police entrance (script `Police_ExtToInt`) and a check that no pale blue-grey disc lies under the player. Registered only if `artifacts/romfs` existed at configure time |
| `game-option-debug-overlay-value` | `--debug-overlay` takes no value |
| `input-sdl-smoke` | Runs with `SDL_VIDEODRIVER=dummy` |
| `assets-core` | Asset runtime, synthetic fixtures |
| `assets-real-data` | Needs `artifacts/romfs`; skips with code 77 otherwise |
| `viewer-real-smoke`, `viewer-cutscene-smoke` | Render one model and one cutscene frame offscreen. Registered only if `artifacts/romfs` existed at configure time |
| `game-real-smoke` | Registered only if `artifacts/romfs` existed at configure time; needs libepoxy |
| `game-script-run`, `game-script-walk`, `game-script-camera` | Same conditions. Feed a fixed stick to the development runtime and check how far the character moved. The first two switch the camera's wall test off (`--no-camera-collision`), the third leaves it on |
| `camera-real-data` | Compares the follow camera with traces recorded from the original game: in open ground (`artifacts/oracle/cam-out`, folder set with `-DCHASE_CAMERA_TRACE_DIR=...`) and near walls (`artifacts/oracle/camwall-out`, `-DCHASE_CAMERA_WALL_TRACE_DIR=...`, needs the RomFS); skips with code 77 if the traces are missing |
| `camera-sweep-real-data` | Checks the camera's wall test against the sweeps recorded from the original game (same folders); skips with code 77 if they are missing |
| `camera-sweep` | Checks the camera wall test on small made-up walls and boxes (no game data needed) |
| `locomotion-real-data` | Compares the character's movement with traces recorded from the original game (`artifacts/oracle/loco-out`, folder set with `-DCHASE_LOCO_TRACE_DIR=...`); skips with code 77 if the traces or `artifacts/romfs` are missing |
| `jump-real-data` | Compares the jump (height curve, forward speed, landing and the stand-still after it) with traces recorded from the original game (`artifacts/oracle/jump-traces`, folder set with `-DCHASE_JUMP_TRACE_DIR=...`); skips with code 77 if the traces or `artifacts/romfs` are missing |
| `fall-real-data` | Compares falling, landing and the stand-still after a landing with traces recorded from the original game (`artifacts/oracle/fall-out`, folder set with `-DCHASE_FALL_TRACE_DIR=...`); skips with code 77 if the traces or `artifacts/romfs` are missing |
| `collision-real-data` | Compares the character's floor height and wall sliding with traces recorded from the original game and the slides against lamp posts and barriers (`artifacts/oracle/loco-out`, `artifacts/oracle/coll-out` and `artifacts/oracle/objcoll-out`, folders set with `-DCHASE_LOCO_TRACE_DIR=...`, `-DCHASE_COLLISION_TRACE_DIR=...` and `-DCHASE_OBJECT_COLLISION_TRACE_DIR=...`); skips with code 77 if the traces or `artifacts/romfs` are missing |
| `soundtest-real-smoke`, `soundtest-id-smoke` | Play one sound by file name and one by number on the dummy audio driver. Same conditions |
| `progress-tables` | The tables behind the progress report: which script commands, object types and file types the port handles. No game data needed |
| `progress-readme` | Fails when the progress table in `README.md` is out of date (`tools/progress_report.py --check`); skips with code 77 if `artifacts/romfs` or the built library is missing |
| `no-inline-shaders` | Fails when a `.cpp`, `.hpp` or `.h` file under `src/` contains shader code. Shaders live only in `.glsl` files under `src/render/shaders/` (see `CLAUDE.md`) |
| `gl-only-in-render` | Fails when a `.cpp`, `.hpp` or `.h` file under `src/` outside `src/render/` contains an OpenGL call, `<epoxy/gl.h>`, a GL type or a GL constant. Rendering code goes into `src/render/` behind plain-type headers (see `CLAUDE.md`). `gl-only-in-render-rejects` checks that the guard fails on the bad file in `tests/gl_only_in_render/` |
| `game-cpp-wiring` | Fails when `src/game.cpp` has more than 400 lines. That file only wires features together. New game logic goes into its own file under `src/runtime/` (see `CLAUDE.md`) |
| `doc-links` | Checks the documentation rules (`tools/check_doc_links.py`). It fails on a broken link or anchor, a dated entry without an anchor, a `docs/re/` file over 40 KB, a feature file over 150 lines, the feature index over 40 lines, an item or stand-in over 400 characters, `CLAUDE.md` over 20 KiB, or a correction that is not marked at both ends. Add `--fix` to repair anchors and links. |
| `studs-core` | Coin pickup rules on made-up coins, no game data needed |
| `studs-real-data` | Replays coin pickups recorded from the original game (`artifacts/oracle/studs-traces`, folder set with `-DCHASE_STUDS_TRACE_DIR=...`); skips with code 77 if the traces or `artifacts/romfs` are missing |
| `audio-mixer` | Mixer core, no SDL |
| `audio-core` | Sound rules on made-up values (distance volume, panning, level music choice, footstep detection, landing sounds), no game data needed |
| `audio-real-data` | Checks the sound tables and replays sound calls recorded from the original game (`artifacts/oracle/audio-traces`, folder set with `-DCHASE_AUDIO_TRACE_DIR=...`, made with `tools/export_sound_trace.py`) by running `chase-game --script`; skips with code 77 if the traces or `artifacts/romfs` are missing |
| `runtime-core` | Game runtime core (frame timing, pad, stick, character movement, floor height and wall sliding), no SDL |
| `script-dialog-real-data`, `game-script-dialog` | The police station entry script: the question box opens, A and B give their answers, the character is moved to the door. Registered only if `artifacts/romfs` existed at configure time |
| `scene-change-real-data`, `game-scene-change-exit`, `game-scene-change-entry` | Walking out of the police station and back in: where each door leads, the landing points from the original, the order loading page, A, switch. Registered only if `artifacts/romfs` existed at configure time |
| `script-runner` | How level scripts run each update (waits, a script calling or starting another one, conditions, door requests, scripts started by walking into marked places), with made-up scripts, no game data needed |
| `dcam` | The script camera: its path curves, start and end waits, the end after the set time and the blocking of `WaitForDCam`, with made-up paths, no game data needed |
| `dcam-real-data` | The camera paths of the first level against what the original showed in the police station entry (positions of the camera, the update on which the camera ends); skips with code 77 if `artifacts/romfs` is missing |
| `save-core` | Save file checks, synthetic data, no SDL |
| `save-real-data` | Completion totals from the game tables; needs `artifacts/romfs`, skips with code 77 otherwise |
| `fmv-real-smoke` | Registered only if FFmpeg and `artifacts/romfs` existed at configure time |
| `frontend-flow` | The order and triggers of the start screens and the StreetPass pages, with a made-up movie, no game data needed |
| `frontend-real-data` | Start-screen texts, line wrapping, exact logo pixels, page pictures and the length of the two movies; skips with code 77 if `artifacts/romfs` is missing |
| `front-game-smoke`, `front-game-handoff`, `frontshot-real-smoke` | Run the start screens with a scripted pad: to the language list, through every screen into the level, and offscreen with `chase-frontshot`. Registered only if `artifacts/romfs` existed at configure time |
| `hud-real-data` | The display during play: the stud bar's timing against a recorded pickup (`artifacts/oracle/hud-traces`, folder set with `-DCHASE_HUD_TRACE_DIR=...`), the icon bounce, the hearts, pictures of the display and the map screen; skips with code 77 if `artifacts/romfs` is missing and without the trace compares only the numbers pinned in the test |
| `game-script-hud` | Runs the level offscreen with the character standing on a coin and checks that the coin was banked and the display shows the new total. Registered only if `artifacts/romfs` existed at configure time |
| `game-room-streaming` | Walks the character from Room7 into Room8 in the street level and checks the room messages: nine rooms at the start, one room unloaded on entering Room8, one more loaded nine updates later. Registered only if `artifacts/romfs` existed at configure time |

## Programs

Every option of these programs is checked. A bad number, an out-of-range value, a missing value or an unknown option stops with an error like `chase-viewer: --frame: not a number: '1x'` and exit code 1. Nothing is read before the options are checked.

```sh
build/chase-tabledump tables/tableslevels.bxls
build/chase-savecheck [--romfs artifacts/romfs/romfs] savegame.dat
build/chase-soundtest music/act_bass_break.bwav
build/chase-soundtest --id 5
SDL_VIDEODRIVER=offscreen build/chase-fmv --lang us --screenshot out.bmp --time 3 artifacts/romfs/romfs/cutscenes/01_gameintro.moflex
cd build && ./chase-input-lab [--config PATH] [--smoke-test]
```

- `chase-tabledump` prints data tables as TSV. Columns have no names in the file.
- `chase-materials FIB CSV` prints how many parts of all models use each material type. With model paths after the two files it prints the texture-environment stages of each part, for example `build/chase-materials artifacts/romfs/romfs/lego_city3ds.fib artifacts/romfs/romfs/lego_city3ds.csv models/lvl_00_policestation/room3.bfnmdl`.
- `chase-savecheck` prints how much of each of the two save slots is complete, as items collected, per mille and percent. A slot with no progress shows as empty. It reads the totals from the game tables in the extracted game data (`--romfs`).
- `chase-soundtest` plays decoded sounds, by file name or by sound number (`--id`). Use `SDL_AUDIODRIVER=dummy` for headless runs. See [Sound test](sound-test.md).
- `chase-fmv` needs the FFmpeg libraries. See [Video player](fmv-player.md).
- `chase-frontshot` runs the start screens offscreen with a scripted pad and saves pictures of both screens (400x480, like the emulator's screenshots). Example: `build/chase-frontshot --output /tmp --script "until language 60; run 5; shot language; press A; run 5; shot streetpass"`. Commands: `run SECONDS`, `press A|B|X|Y|START|UP|DOWN`, `until SCREEN [SECONDS]` and `shot NAME`. A bad script stops with an error before anything runs. Screens: boot, logos, fusion, language, prompt, autosave, slot, saving, loading, intro, card, done. Options: `--output`, `--language 1-9`, `--returning`, `--fib`, `--names`, `--movies`.
- `chase-hudshot` draws the display of the level (portrait, hearts, stud counter) offscreen and saves it as a BMP (over black) with a mask BMP, or saves the bottom-screen map. Example: `build/chase-hudshot --output /tmp --script "run 90; shot start; studs 0 10; run 40; shot bar; map lvl_01_cherrytreehills 886.764 1111.166 -2.35 -2.35 map"`. Commands: `run N` (updates of 1/30 s), `hearts N`, `studs TYPE TOTAL`, `shot NAME`, `map LEVEL X Z CAMERAYAW PLAYERYAW NAME`. Options: `--output`, `--fib`, `--names`.
- `chase-input-lab` looks for `config/controls.cfg` relative to the current folder. CMake copies it into `build/`. See [Controls](controls.md).

## Development runtime

Needs libepoxy. This is **not the playable game**. It shows a level with the main character standing at the start point from the game data. The character walks, runs and sprints when you press the D-pad keys. Speed, turning and the animation changes follow what was measured in the original game. Everything else about the game is missing. For a plain-language guide, see [Development runtime](game-runtime.md).

```sh
build/chase-game
build/chase-game --gameplay
build/chase-game --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl
SDL_VIDEODRIVER=offscreen build/chase-game --gameplay --screenshot out.bmp --frames 2
SDL_VIDEODRIVER=offscreen build/chase-game --front-script "until language 60; run 1" --screenshot out.bmp
SDL_VIDEODRIVER=offscreen build/chase-game --script "0 150 40" --screenshot out.bmp
```

- Bad input stops the program at once with an error that names the option and the bad text, for example `chase-game: --slot: out of range, allowed 0 to 1: '2'`. Numbers must be whole and in range, `--start` needs exactly three numbers, and each `--script` and `--front-script` step must follow its format.
- Without `--gameplay`, `--level`, `--script` or `--start`, the program starts with the screens of the original game (see [Development runtime](game-runtime.md)). `--front-script "..."` runs those screens with a scripted pad (same commands as `chase-frontshot`) and saves the window with `--screenshot`; if the script reaches `done`, the level follows. `--language 1-9` and `--returning` choose the language and a returning player. Escape quits.
- Options: `--fib`, `--names`, `--config` (as in the viewer), `--gameplay` (start in the level), `--window WIDTHxHEIGHT` (window size, default 960x600; 400x480 gives the size of the emulator screenshots), `--level`, `--screenshot`, `--frames`, `--script`, `--start`, `--save FILE --slot 0|1` (start from a real save, for example your own `savegame.dat`: the program starts the level that the slot was last in, where the original's Continue starts it, with the missions done, unlocks and level variables of the slot; collected items and object states do not yet come from the save. The slot must be in use, an empty slot stops with an error. `--level` starts another level with the progress of the slot, and `--start` / `--facing` place the player yourself. Only the police station was checked against the original; other levels use the level's start point too, which is not checked yet), `--call-script NAME` (development aid: starts the level script NAME after the level's own start, for example `1-1_MissionSkip` to switch on the police station door; after a switch to another level it runs again there if that level has the script), `--debug-overlay` (start with the [debug overlay](debug-overlay.md) shown; F3 toggles it), `--no-camera-collision` (the camera ignores the level's walls, for comparing screenshots), `--sound-volume 0-10` and `--music-volume 0-10`. After a `--script` run the program prints `sounds:` with one `update:sound-number` pair per sound played. Use `SDL_AUDIODRIVER=dummy` for headless runs.
- The default level of `--gameplay` is the police station. The character is Chase McCain in his police outfit. That choice is a best guess. Run it from the repository folder so the default paths work.
- Move with the D-pad keys (WASD or arrows). Hold `Left Alt` to walk slowly. Without it you run, and after about one second of running you sprint. Press `Q` (the B button, or the right mouse button) to jump. `E` is the X button, which does nothing yet. `Z` and `C` turn the camera (the left and right shoulder buttons). Pushing sideways also turns the camera, as in the original. `Escape` quits.
- What moves like the original: the speeds, the walk, run and sprint switch, the delay before sprinting, and the way the character turns towards the direction you push.
- What is still a stand-in:
  - The camera follows the character like the original: distance, height, smoothing and turning match what was measured. It keeps out of the level's walls and of most of its objects (lamp posts, doors, signs) the way the original does. The original's re-placing of a camera that stays stuck is missing. Rooms that use several camera zones are not handled.
  - The collision is the level's fixed geometry plus most placed objects (lamp posts, crates, barriers, doors, signs), as they stand at the start of the level. The character stays on the floor, climbs steps and kerbs by the original's rules, and slides along walls and objects. Other characters, vehicles and some special objects do not block him. A list of objects that the original switches off at the start was read from the original and is used instead of its scripts.
  - He falls off ledges with the original's gravity and top speed. He lands hard after a fall of 12 units or more. After a landing he stands still for a short time, as in the original. When he walks off a ledge the original plays a short step-off first. That step is missing here. The stand-still time is measured, not read from the game's data.
  - The jump follows the height curve and the forward speed measured in the original (about 2.1 units high, forward speed 0.8 of the ground speed, no double jump). Jumping onto something higher or under a ceiling is not handled like the original, and the jump heights use measured scale factors.
  - The level shows its fixed objects, such as props, doors and parked vehicles, standing still. The level's other people (not animals) stand in place in a neutral pose with their own heads and clothes. Mission markers are not shown, and nothing reacts to you except the coins. Coins (studs) spin in place and are picked up when you come within 1.5 units; the stud counter goes up one second later (silver 10, gold 100, blue 1000, purple 10000). This is a stand-in in some details: a picked-up coin disappears at once, there is no counter on the screen, and coins that a script switches on stay hidden. See [Development runtime](game-runtime.md).
  - The animations play at an assumed speed and switch without blending.
  - A keyboard cannot tilt the stick halfway, so `Left Alt` stands in for a gentle push. It is a separate control from the 3DS buttons, set in `config/controls.cfg`.
- It prints one line per second: updates per second, the last time step and the circle pad values.
- `--screenshot` saves the picture after `--frames` updates (default 2) and exits.
- `--script "X Y COUNT [BUTTONS];X Y COUNT"` ignores the keyboard and holds the stick at X, Y for COUNT updates of 0.033 s each (values like the 3DS circle pad: +X right, +Y up, 145 is full). BUTTONS are optional 3DS button letters (A, B, X, Y, L, R) held during those updates. For example `"0 150 40"` pushes forward for 40 updates, and `"0 0 2;0 0 1 B;0 0 30"` stands, presses jump for one update and waits. The program then prints how far the character moved, the stud counter (`studs: counter N, ...`) and the character's height after every update, then exits, saving the picture if `--screenshot` is given.
- `--start "X Y Z"` puts the character at that world position instead of the level's start point. `--facing DEGREES` sets the direction he faces (the camera follows). Use it to test a ledge, for example `--level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --start "899.5 17.6393 1101" --script "0 150 40"` runs off a 1 m platform.

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
- `tools/oracle/camwall-circle.txt` and `tools/oracle/camwall-east.txt` record the camera near walls. They need the extra option `--extra 0x330` when converting.
- `tools/oracle/jump-*.txt` measure jumps (the B button). `tools/oracle/fall-*.txt` measure falling and landing. They use the `teleport X Y Z` step, which places the player at a position (a measuring aid). Start each step list with `stick 0 0`, because the last stick value stays set after `loadstate`.
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
- Each saved state remembers whether it was made in New 3DS mode. `loadstate` refuses a state from the other mode and asks you to start a new emulator with `--state NAME`.
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
