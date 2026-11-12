# Command reference

Every command of the extraction tools, converters and programs, on Linux, with its options. For build and test setup, see [Building](building.md).

Back to the [documentation list](../README.md#more-documentation).

Let `CCI` be the path of your image file, for example `path/to/game.cci`.

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
ctest --test-dir build -j"$(nproc)" --output-on-failure            # all tests, about one and a half minutes on a recent 8-core machine
ctest --test-dir build -j"$(nproc)" -LE slow --output-on-failure   # without the slow tests, about half a minute on the same machine
```

- Tests with the label `slow` run for more than about 5 seconds each. They are listed at the end of `CMakeLists.txt`. Run them before every commit that ends a piece of work.
- Build with `-DCMAKE_BUILD_TYPE=RelWithDebInfo` (the default). A `Debug` build runs the game tests about 5 times slower.
- Tests with the label `needs-game-data` need your extracted game data (`artifacts/romfs`). Most of them skip when it is missing. They are listed at the end of `CMakeLists.txt`. The test `test-labels` fails when a test that needs the data lacks the label.
- Tests with the label `needs-display` open an OpenGL 4.6 window through SDL. They skip with exit code 77 when there is no display or no GL 4.6 context.

## Continuous integration

The workflow files in `.forgejo/workflows/` and `.github/workflows/` define the same four workflows. CI has no game data, so it never runs the tests labelled `needs-game-data`.

| Workflow | When | What it runs |
|---|---|---|
| `ci.yml` | every push to `main` and every pull request | Build with warnings as errors, the Python tests, then `ctest -LE 'slow\|needs-game-data'` |
| `full.yml` | every night and by hand (`workflow_dispatch`) | The same build and Python tests, then `ctest -LE needs-game-data`, which includes the `slow` tests |
| `dco.yml` (workflow `DCO`, job `dco`) | every pull request | `python3 tools/check_dco.py`: fails when a commit of the pull request has no `Signed-off-by:` line |
| `generated.yml` (workflow `Generated files`, job `regenerate`) | every push to `main` | `python3 tools/regenerate_generated_files.py`, then commits the changed generated files to `main` as the GitHub Actions bot |

Reproduce a CI run on your machine:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_COMPILE_WARNING_AS_ERROR=ON
cmake --build build -j"$(nproc)"
python3 -m unittest discover -s tests -p 'test_*.py'
SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy ctest --test-dir build -j"$(nproc)" -LE 'slow|needs-game-data' --output-on-failure   # ci.yml
SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy ctest --test-dir build -j"$(nproc)" -LE needs-game-data --output-on-failure          # full.yml
```

- `-DCMAKE_COMPILE_WARNING_AS_ERROR=ON` needs CMake 3.24 or newer.
- On a machine with game data, the same commands also work and skip the data tests. Run the plain `ctest` (see above) to run all of them.

## Documentation links

Run this before you commit any Markdown change. It checks every link between the Markdown files and every `#anchor`. `--fix` adds the anchor line to each dated entry in `docs/re/` and `docs/design/` and points every `§<timestamp>` link at its entry. Running it twice changes nothing.

```sh
python3 tools/check_doc_links.py          # list broken links and missing anchors
python3 tools/check_doc_links.py --fix    # repair what can be repaired mechanically
python3 tools/check_doc_links.py --next VEH 3          # next free item IDs (VEH-S for stand-ins)
python3 tools/check_doc_links.py --reserve VEH 10 20261021-audio-A   # reserve a range for a parallel agent
python3 tools/check_doc_links.py --release 20261021-audio-A          # drop that agent's reservations
```

- An unknown prefix (a typo) stops `--next` and `--reserve` with an error; add `--new-prefix` only for a new feature.
- `--release` with a name that holds no reservation stops with an error.
- At a merge, `merge_agent.py finish --release` does the release inside the merge commit (see "Merging an agent branch").

## Round names

Rounds of the lead workflow have names, not numbers: the start date and a topic word, such as `20261021-audio`. Agents are `20261021-audio-A`, `20261021-audio-B` and so on.

```sh
python3 tools/round_log.py name 20261021-audio   # is the name well formed and still free?
python3 tools/round_log.py audit                 # is the user docs audit due?
```

- `name` exits with 1 when a progress log, its archive, the ID reservations or a local round folder already uses the name. Add a second topic word then, such as `20261021-audio-sfx`.
- A badly formed name stops with an error. A name is badly formed when it breaks one of these rules.
  - It is not `YYYYMMDD-word`.
  - It has more than three words.
  - It has a date in the future.
  - It uses the words `servant` or `audit`.
- `audit` counts the round-end lines (`Lead (<round> end)`, and the old `Lead (round N end)`) since the last line that says `docs audit: done` or `docs audit: declined`. The audit is due after ten.

## Progress report

```sh
python3 tools/progress_report.py                            # short table
python3 tools/progress_report.py --json artifacts/progress.json
python3 tools/progress_report.py --update-readme README.md  # rewrite the progress table in the README
python3 tools/progress_report.py --check README.md          # fails when the README table is out of date
```

## Sign-off check and generated files

```sh
python3 tools/check_dco.py --base BASE_COMMIT --head HEAD_COMMIT
python3 tools/regenerate_generated_files.py             # rewrite the generated files that differ
python3 tools/regenerate_generated_files.py --check     # exit 1 when one differs, write nothing
python3 tools/regenerate_generated_files.py --skip-marked   # do nothing when HEAD is a commit made by the workflow
python3 tools/generated_file_census.py --refresh        # measure the game data and rewrite tests/data/generated_file_census.csv
python3 tools/generated_file_census.py --check          # fails when the table differs from a measurement
```

- `check_dco.py` checks every commit that is in `HEAD_COMMIT` and not in `BASE_COMMIT`. A commit needs a line `Signed-off-by: Name <email>`; a GitHub noreply address is fine. Merge commits and commits of bots (author name ending in `[bot]`) are not checked. To fix a failure, run `git rebase --signoff main` (or `git commit --amend -s` for the last commit) and force-push your own branch. A missing, unknown or malformed `--base` or `--head` stops with an error naming the option.
- The workflow `Generated files` rebuilds the README progress section, `docs/plans/priorities.md` and `docs/progress-trend.svg` from tracked files only. The numbers that come from game data are read from `tests/data/generated_file_census.csv` (columns `section,key,value,provenance`). It commits the files that differ with the marker `[generated-files]`; a push whose HEAD carries the marker does nothing.
- A ruleset or branch protection on `main` must let the GitHub Actions bot push (a bypass entry), or the `regenerate` job cannot commit.
- The line `Updated <date>, commit <hash>.` of the README section is not compared, so a new commit alone changes nothing.
- `progress_report.py` and `priority_report.py` read the table when the game data is absent. `--census TABLE` names another table; `--from-census` uses the table even when the game data is present.
- The table is refreshed at each merge on a machine with game data. `--refresh` and `--check` need `artifacts/romfs`, `artifacts/game` and the built `build/libchase_assets_c.so`; `--check` skips with code 77 without them.

## Priority ranking

```sh
python3 tools/priority_report.py                                    # print the ranking
python3 tools/priority_report.py --update docs/plans/priorities.md  # rewrite the file
python3 tools/priority_report.py --check docs/plans/priorities.md   # fails when the file is out of date
```

Each item in the ranking links to its line in the source view of its feature file (`?display=source#L<n>`). The line numbers change when a feature file changes: run `--update` after editing one. `python3 tools/check_doc_links.py --fix` follows moved lines; a link to an item that no longer exists fails `doc-links`.

Needs the built `build/libchase_assets_c.so` and the extracted game data. See [Progress](progress.md).

## C++ tests

The C++ tests are listed by area, one page per area, each with a table of test names. Run one test with `ctest --test-dir build -R <name>`. `ctest --test-dir build -N` lists all of them.

- [Input and game scene tests](commands-tests.md): input, settings menu and whole-scene tests
- [Display tests](commands-tests-display.md): HUD, menus, pause screens, drawing
- [Simulation tests](commands-tests-simulation.md): traffic, vehicles, characters, scripts
- [Library and data tests](commands-tests-data.md): assets, audio, tools and documentation checks

## Merging an agent branch

```sh
python3 tools/merge_agent.py sync                    # fetch, then compare main with its upstream
python3 tools/merge_agent.py prepare BRANCH          # sync, squash the branch into main's staging area, then run the checks
python3 tools/merge_agent.py check                   # run the checks again after fixing conflicts or review edits
python3 tools/merge_agent.py finish BRANCH -m FILE   # checks, commit with the message in FILE, remove worktree and branch
python3 tools/merge_agent.py finish BRANCH -m FILE --release 20261021-audio-A --log linux-port LINE_FILE
python3 tools/merge_agent.py close -m FILE --release 20261021-audio-C --log linux-port LINE_FILE
python3 tools/merge_agent.py check-pr 42             # check pull request 42 of the GitHub remote
```

- `sync` compares main with its upstream (`--upstream REMOTE/BRANCH` names another one). When the upstream is ahead and main has no own commits, it fast-forwards main. When both moved, or main has uncommitted changes, it stops with an error, and the user decides.
- `prepare` runs `sync` first and takes the same `--upstream`. `--no-sync` skips it, for a session without a remote.
- `finish --release WHO` drops the ID ranges WHO holds, inside the merge commit; give it once per holder. A name that holds no reservation stops with an error.
- `finish --log PLAN LINE_FILE` appends the round-end line to the session log of the plan's progress file (`docs/plans/<plan>.progress.md`) inside the merge commit. LINE_FILE holds exactly one line `Lead (<round> end): ...` without a timestamp; the tool adds the time from the clock. Any other text stops with an error.
- `finish` removes the worktree of a Claude agent even while the agent's lock is still set. A worktree with any other lock stays: `finish` warns and prints the `git worktree remove` and `git branch -D` commands, and still reports the commit.
- Both are safe to repeat: after a failed check, run `finish` again with the same options.
- `prepare` and `check` record a fingerprint (staged tree and main's HEAD) in the git directory (`merge-agent-verified`, untracked) once build, ctest and Python tests passed. When `finish` starts with the same fingerprint, it skips those three and prints one line saying so; the doc and README checks still run. A failed check deletes the record; a different tree or a missing record runs everything.
- `check-pr N` checks the GitHub pull request N. It fetches `refs/pull/N/head` from the git remote whose URL points to github.com (it prefers `zJanny/LegoCityUndercover-TheChaseBegins-PC-Port`; with none it stops and prints the command that adds the remote). Before any code of the pull request runs, it checks the full tree of the pull request with the rules of `tools/forbidden_files.py`. Then it builds warning-free and runs all CTests (also `needs-game-data`, through an `artifacts` link) and the Python tests in a temporary standalone checkout under `$TMPDIR` (several GB for the build). It prints `check-pr N: PASS` or `check-pr N: FAIL` with the reasons and always removes the checkout. It never pushes or merges. Exit codes: 0 PASS, 1 FAIL or error, 2 bad arguments. Read the diff first.
- `prepare`, `check` and `finish` refresh `tests/data/generated_file_census.csv` when game data is present.
- `close` makes the same release and log line as its own commit, for a round whose last branch was dropped. It needs at least one of the two options.

The checks stop on game files, screenshots or other binary files (except bundled fonts `third_party/fonts/*.ttf`), broken doc links, build warnings and failing tests. They also refresh the README progress table. Conflicts in the README progress table or the trend graphic are fixed by making them again. Conflicts in a session log archive keep the lines of both sides. Lines that both sides added to a session log, the feature index, the ID reservations or a `docs/re/` topic are both kept. Other conflicts you fix by hand, `docs/design/` ones included. The message file must end with the merging agent's own `Co-Authored-By:` line. The script never pushes.

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
- `chase-layoutdump` prints UI layouts. `build/chase-layoutdump --list` lists every `.bxaml` in the archive with its size, element count, storyboards and references. `build/chase-layoutdump --tracks sprites/blend/pausetop/pausetop.bxaml` prints the element tree and the storyboards with their tracks. Options: `--fib`, `--names`.
- `chase-materials FIB CSV` prints how many parts of all models use each material type. With model paths after the two files it prints the texture-environment stages of each part, for example `build/chase-materials artifacts/romfs/romfs/lego_city3ds.fib artifacts/romfs/romfs/lego_city3ds.csv models/lvl_00_policestation/room3.bfnmdl`.
- `chase-savecheck` prints how much of each of the two save slots is complete, as items collected, per mille and percent. A slot with no progress shows as empty. It reads the totals from the game tables in the extracted game data (`--romfs`).
- `chase-soundtest` plays decoded sounds, by file name or by sound number (`--id`). Use `SDL_AUDIODRIVER=dummy` for headless runs. See [Sound test](sound-test.md).
- `chase-fmv` needs the FFmpeg libraries. See [Video player](fmv-player.md).
- `chase-frontshot` runs the start screens offscreen with a scripted pad and saves pictures of both screens (400x480, like the emulator's screenshots). Example: `build/chase-frontshot --output /tmp --script "until language 60; run 5; shot language; press A; run 5; shot streetpass"`. Commands: `run SECONDS`, `press A|B|X|Y|START|UP|DOWN`, `until SCREEN [SECONDS]` and `shot NAME`. A bad script stops with an error before anything runs. The `--output` folder must exist. Screens: boot, logos, fusion, language, prompt, autosave, slot, saving, loading, intro, card, done. Options: `--output`, `--language 1-9`, `--returning`, `--save FILE` (show the slots of that save file in the slot list: time, percent and location of a used slot, Delete shortcut), `--fib`, `--names`, `--movies`.
- `chase-hudshot` draws the display of the level (portrait, hearts, stud counter) offscreen. It saves it as a BMP (over black) with a mask BMP, or saves the bottom-screen map.
  - Example: `build/chase-hudshot --output /tmp --script "run 90; shot start; studs 0 10; run 40; shot bar; map lvl_01_cherrytreehills 886.764 1111.166 -2.35 -2.35 map"`.
  - Commands: `run N` (updates of 1/30 s), `hearts N`, `studs TYPE TOTAL`, `brickcount TOTAL`, `brick` (a super brick pickup), `brickbank TOTAL`, `shot NAME` and `map LEVEL X Z CAMERAYAW PLAYERYAW NAME`.
  - Pause menu commands: `pause` (open it), `press START|A|B|LEFT|RIGHT|UP|DOWN` (one update with that press) and `touch X Y N` (touch the lower screen at X, Y for N updates, then let go).
  - Map commands: `marker X Z` adds a marker at that world position to the map pictures that follow. `ring SECONDS` sets the time of the markers' ring animation.
  - After `pause`, `shot NAME` saves both screens as one 400x480 picture. Example: `build/chase-hudshot --output /tmp --script "pause; run 50; shot open; press RIGHT; run 20; shot options"`.
  - Options: `--output` (the folder must exist), `--fib`, `--names`.
- `chase-input-lab` looks for `config/controls.cfg` relative to the current folder. CMake copies it into `build/`. See [Controls](controls.md).

## Development runtime

Needs libepoxy and FFmpeg. This is **not the playable game**. It is an early test program. It shows the start screens of the original game and then a level. There the main character walks, runs, jumps and sprints, with parts of the first missions. Large parts of the game are missing. For a plain-language guide to what works and what does not, see [Development runtime](game-runtime.md) and [Project status](status.md).

```sh
build/chase-game
build/chase-game --gameplay
build/chase-game --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl
SDL_VIDEODRIVER=offscreen build/chase-game --gameplay --screenshot out.bmp --frames 2
SDL_VIDEODRIVER=offscreen build/chase-game --front-script "until language 60; run 1" --screenshot out.bmp
SDL_VIDEODRIVER=offscreen build/chase-game --script "0 150 40" --screenshot out.bmp
```

- Bad input stops the program at once with an error that names the option and the bad text, for example `chase-game: --slot: out of range, allowed 0 to 1: '2'`. Numbers must be whole and in range, `--start` needs exactly three numbers, and each `--script` and `--front-script` step must follow its format.
- Without `--gameplay`, `--level`, `--script` or `--start`, the program starts with the screens of the original game (see [Development runtime](game-runtime.md)).
  - `--front-script "..."` runs those screens with a scripted pad (same commands as `chase-frontshot`) and saves the window with `--screenshot`. If the script reaches `done`, the level follows.
  - `--language 1-9` and `--returning` choose the language and a returning player.
  - `--front-save FILE` shows the slots of your own `savegame.dat` in the slot list. A on a used slot continues in the level that slot was last in, with its progress. It does not work together with `--gameplay`, `--script`, `--level`, `--start` or `--save`.
- Options of `chase-game`:
  - `--fib`, `--names` and `--config` (as in the viewer).
  - `--gameplay` starts in the level.
  - `--window WIDTHxHEIGHT` starts windowed at this size instead of the stored window mode and size. Below 400x240 it stops with an error (see [Window mode and size](game-runtime.md#window-mode-and-size)). 400x480 gives the size of the emulator screenshots.
  - `--level`, `--screenshot`, `--frames`, `--script` and `--start` (described below).
  - `--save FILE --slot 0|1` starts from a real save, for example your own `savegame.dat`.
    - The program starts the level that the slot was last in, where the original's Continue starts it. It uses the missions done, unlocks, level variables, studs and super bricks of the slot.
    - Collected items and object states do not come from the save.
    - The slot must be in use. An empty slot stops with an error.
    - `--level` starts another level with the progress of the slot. `--start` and `--facing` place the player yourself.
    - Only the police station was checked against the original. Other levels use the level's start point too, which is not checked.
    - Without `--save` a run is a new game in slot `--slot` (default 0) with the original's starting progress.
  - `--save-out FILE` is where save requests go. Without it they go to the port's save file `$XDG_DATA_HOME/chase/savegame.dat`, normally `~/.local/share/chase/savegame.dat`.
    - Every save request of the game writes the slot of this run into that file. Examples are the level scripts' QuickSave or a collected super brick. The other slot and the options of an existing file are kept.
    - This also happens in development runs with `--level` or `--script`. Give `--save-out` to keep your own save untouched.
    - It must not be the `--save` file. The level state of the slot is not refreshed.
  - `--call-script NAME` (development aid, may be repeated) starts the level script NAME after the level's own start. For example `1-1_MissionSkip` switches on the police station door. After a switch to another level it runs again there if that level has the script.
  - `--set-variable NAME=VALUE` (development aid, may be repeated) sets a variable of the level before its start scripts run.
    - NAME must be a variable of the level. A number is required.
    - The police station plays a camera tour on the first visit (`Lobby_Dcam_Intro=0`), and the camera holds the player. `--set-variable Lobby_Dcam_Intro=1` starts after the tour.
    - It is not applied again after a switch to another level.
  - `--super-bricks N` (development aid) sets the super brick count, 0 to 4294967295, at the start of the run. It wins over the count of `--save`. The game saves the count as it is then, like picked-up bricks. It is not applied again after a switch to another level.
  - `--extras MASK` (development aid) sets the extras mask of the original, 0 to 4294967295, at the start of the run. It is 0 after a load, as in the original.
    - Bit 19 (`--extras 524288`) turns on the cop whistle. Press the D-pad Down key while standing free with empty hands. The clip plays and the traffic stops for a few seconds.
    - The mask also answers the level scripts' IsRedBrickExtraActive. The menu that sets it in the original is not recovered.
  - `--debug-overlay` starts with the [debug overlay](debug-overlay.md) shown. F3 toggles it.
  - `--no-camera-collision` makes the camera ignore the level's walls, for comparing screenshots.
  - `--no-start-cutscenes` (development aid) makes the level's start skip the intro movie and the first cutscene of a new game. You can then place and test the player at once. Without it `--level` plays them first and then moves the player to the scripted start spot.
  - `--sound-volume 0-10` and `--music-volume 0-10`.
  - After a `--script` run the program prints `sounds:` with one `update:sound-number` pair per sound played. Use `SDL_AUDIODRIVER=dummy` for headless runs.
- The default level of `--gameplay` is the police station. The character is Chase McCain in his police outfit. That choice is a best guess. Run it from the repository folder so the default paths work.
- Move with the D-pad keys (WASD or arrows). Hold `Left Alt` to walk slowly. Without it you run, and after about one second of running you sprint. Press `Q` (the B button, or the right mouse button) to jump. `E` is the X button, which gets into and out of vehicles (see [Development runtime](game-runtime.md)). `Z` and `C` turn the camera (the left and right shoulder buttons). Pushing sideways also turns the camera, as in the original. Escape acts as Start (it opens the pause menu); to quit, close the window or use Quit game in the [PC settings menu](controls.md#pc-settings-menu).
- What is like the original and what is still a stand-in: [Development runtime](game-runtime-works.md) and [what does not work yet](game-runtime-missing.md).
- A keyboard cannot tilt the stick halfway, so `Left Alt` stands in for a gentle push. It is a separate control from the 3DS buttons, set in `config/controls.cfg`.
- It prints one line per second: updates per second, the last time step and the circle pad values.
- Traffic lines of the level:
  - `traffic: N cars, limit L, P spawn points, R routes, F floor cells, spawn data loaded` once at the start.
  - `traffic car NAME: spawned at point P (X Y Z), fade F at update U` and `traffic car NAME: removed at update U` for each car that appears or leaves.
  - `traffic car NAME: body joins the physics world at update U` and `... body leaves the physics world at update U` when the car's physics body is added to or taken out of the physics world.
  - `traffic car NAME: hit, driver requests state 4 at update U` when a car with a physics body hits it.
  - `traffic car NAME: fade F at update U` when a car enters a fade box.
  - `traffic car NAME: stopped at junction J at update U` and `traffic car NAME: goes on at junction J at update U` when a car stops at a light and goes on.
  - `traffic car NAME: lane of junction J at update U` and `traffic car NAME: left the junction lane at update U` when it drives through a junction.
  - `traffic: limit L at update U` when a script calls `SetTrafficMaxVehicles`.
  - `traffic: path NAME enabled|disabled at update U` when a script calls `EnableTrafficPath`.
- Spawner lines of the level:
  - `ai spawners: N, M characters held` once at the start.
  - `ai spawner NAME: state N STATE at update U` for each state change.
  - `ai spawner NAME: spawned CHARACTER (slot I, S/M) at update U` for each character sent out.
  - Before it, `ai start state N CHARACTER: ORDER [to TARGET] at update U` with the first order the character got: `idle`, `move`, `super jump`, `mill about` or `no state`. `(not ported)` follows an order of the original that is not done yet. `(target not found)` follows when the target object is missing.
  - `ai spawner NAME: TriggerObject TARGET at update U` when it starts the next step.
- `--screenshot` saves the picture after `--frames` updates (default 2) and exits.
- `--script "X Y COUNT [BUTTONS];X Y COUNT"` ignores the keyboard and holds the stick at X, Y for COUNT updates of 0.033 s each. The values are like the 3DS circle pad: +X right, +Y up, 145 is full.
  - BUTTONS are optional 3DS button letters (A, B, X, Y, L, R, and S for Start) held during those updates.
  - For example `"0 150 40"` pushes forward for 40 updates. `"0 0 2;0 0 1 B;0 0 30"` stands, presses jump for one update and waits.
  - A step `touch X Y COUNT` touches the lower screen at pixel X (0 to 319), Y (0 to 239) for COUNT updates. It acts like a click with the mouse. For example `"touch 159 65 3"` clicks the exit button of the fast travel map.
  - A step `at X Y Z [FACING]` puts the character at that world position before the next update, with the facing in degrees if given. It is a development teleport and counts no update. For example `"0 0 30;at 817.9 23.94 1071.9 0;0 0 60"`.
    - While the character sits in a vehicle, the step moves the vehicle there instead. The vehicle is turned to FACING if given, else it keeps its orientation from the level. The character stays in it.
    - The program prints `script teleport to X Y Z at update N` for each one.
  - A step `left-vehicle CHARACTER VEHICLE` tells the game that the level character CHARACTER has left the vehicle VEHICLE. For example `"0 0 5;left-vehicle 1-4_Booster_Strange_Chaser 1-4_Stolen_Vehicle"`.
    - It raises by hand the event that a character's jump out of a vehicle raises. It counts no update. It also releases the character from its AI spawner.
    - The character can then be arrested with A at once. The program prints `arrest: character NAME left vehicle V, arrestable at update N`.
    - An unknown character name stops the program with an error.
  - The program prints one line `script NAME started at update N` or `ended at update N` for every level script. It prints `cutscene NAME` and `movie NAME` lines when a cutscene or movie starts or ends.
  - After every update the program prints how far the character moved and the screen brightness (`screen brightness 1` is normal, 0 is black). It also prints the stud counter (`studs: counter N, ...`) and the character's height. Then it exits, saving the picture if `--screenshot` is given.
- `--start "X Y Z"` puts the character at that world position instead of the level's start point. `--facing DEGREES` sets the direction he faces (the camera follows). `--hide-room N` (1 to 99) never shows room N of the street, for comparing a picture with and without a room. Use it to test a ledge, for example `--level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --start "899.5 17.6393 1101" --script "0 150 40"` runs off a 1 m platform.
- `--particle "DEFINITION X Y Z"` (repeatable, 0 to 65535) starts a particle effect of `particles/particledefs.bprt` at that world position when the level starts. Use it to look at the particle drawing. Nothing in the game starts effects. For example `--start "886.764 16.589 1111.166" --particle "22 886.764 17 1112"` shows question marks next to the character.

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

It runs your own game copy without a window, so developers can compare our code with the original game. It uses the only `.cci` file in the repository folder. Pass `--image FILE` if there are several.

Requirements (only for this tool):

- The Azahar 3DS emulator as `azahar` in `PATH`. The tool was written and tested with version 2126.2; other versions are untested.
- For `--rpc` and `--rpc-input`: an Azahar build with the RPC server compiled in (CMake option `ENABLE_SCRIPTING`). Many packaged builds (AppImage, Flatpak, distribution packages) may not have it. Without it the tool stops after about a minute with a message that names the missing RPC server.
- A build with Cemuhook UDP support, which the virtual motion and touch input use.
- Xvfb, for `--display xvfb` (needed for save states).
- The libraries `libX11.so.6` and `libXtst.so.6`, which the tool uses to press the emulator hotkeys for save states.
- Working OpenGL (for example Mesa) inside Xvfb, for `--renderer opengl`.
- Free local UDP ports: `--port` (default 24690), the motion port (`--port` plus 1000) and, for `--rpc`, the fixed port 45987. The tool picks free X displays among `:90` to `:199`.

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
- `freetrap SECONDS ADDR[,ADDR...] NAME` catches rare calls without stepping: the game runs normally for that many seconds and stops only at the given code addresses. Each stop's registers and the memory they point to go to `NAME.txt`. Held buttons and the stick do not work during it, so do the walking first with stepped frames. The script stops with an error in these cases:
  - A wrong number of words.
  - A time that is not a positive number.
  - An address that is not a word-aligned code address (0x100000 to 0xfffffff).
  - A name with characters other than letters, digits, `_`, `-` and `.`.

### Working faster

Save a state once, then start from it. States need the `--display xvfb` option, which runs the emulator on a private virtual screen.

```sh
python3 tools/azahar_oracle.py tools/oracle/to-gameplay.txt --display xvfb --renderer opengl --keep   # ends with: savestate gameplay-start
python3 tools/azahar_oracle.py --stop
python3 tools/azahar_oracle.py my-test.txt --display xvfb --renderer opengl --state gameplay-start
```

- `savestate NAME` and `loadstate NAME` are steps in the script file. States are stored in `artifacts/oracle/states/` and survive `--fresh`.
- Only `gameplay-start` (made by `tools/oracle/to-gameplay.txt`) and the states that other committed scripts save can be rebuilt by running a script. A script that loads any other state uses a role (`loadstate @ROLE`, below). A role stands for a state that you record yourself at the matching point of the game: play there and save with `savestate NAME` or the emulator's quick save.
- Each saved state remembers whether it was made in New 3DS mode. `loadstate` refuses a state from the other mode and asks you to start a new emulator with `--state NAME`.
- A committed script that needs a state you record yourself loads it by a role: `loadstate @ROLE` (and `--state @ROLE`). The role is capital letters, digits and `_`, starting with a letter. Give the state for each role with `--state-param ROLE=NAME` (repeat it for several roles; `NAME` is a state in `--states`). The roles of a script are in its `loadstate` lines; the game situation each role stands for is listed in [oracle-setup.md](re/oracle-setup.md#ts-20261025-141455).

```sh
python3 tools/azahar_oracle.py tools/oracle/camera-vehicle-enter.txt --display xvfb --renderer opengl \
  --state @PATROL_CAR --state-param PATROL_CAR=my-car-state
```

- The tool checks the roles before it starts the emulator. It stops with an error that names the option or the script line and the text when:
  - a role has no `--state-param`.
  - a role is written wrongly (`@patrol`).
  - a `--state-param` is not `ROLE=NAME` or names a role twice.
  - `NAME` has other characters than letters, digits, `_`, `-` and `.`.
  - the state files `NAME.cst` and `NAME.json` are missing in `--states`.
  - `loadstate` or `savestate` has not exactly one word after it.
  - `savestate` is given a role (it takes plain names).
- `tools/check_local_data.py` (CTest `no-local-data`) also reads `tools/oracle/*.txt`: `loadstate` there may name only a role or a state that a committed script saves.
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
- Only one emulator at a time can use `--rpc`, because it always uses the UDP port 45987. The tool stops with a message if that port is taken. Developers who share one machine cannot use `--rpc` at the same time.
- Without `--rpc` the tool works as before.

### Motion input

The game reads the 3DS motion sensors (the sound scanner needs them). Every emulator session gets a virtual motion source: a small server (`tools/oracle_motion.py`) that the tool starts and stops with the emulator. It holds the console level and still.

- `--motion-port PORT` sets its UDP port (1024 to 65535, default: `--port` plus 1000). Give a different one to each emulator that runs at the same time.
- The script step `tilt AX AY AZ [GP GY GR]` changes what it reports: acceleration in g and, optionally, rotation in degrees per second. The level pose is `tilt 0 0 -1`. Wrong numbers or a wrong count stop the script with an error.
- Details and axes: `docs/re/oracle-setup.md`.

### Touching the bottom screen

The script steps `touch` and `drag` touch the 3DS bottom screen (the touch screen of the game's menus). The point goes through the same virtual motion source as a touch pad. It needs no `--rpc`, no mouse events and no change to the game's memory. The game reads it from its normal touch input.

- `touch X Y [UPDATES]` taps the bottom-screen pixel X Y: X is 0 to 319 and Y is 0 to 239 (the 320 by 240 screen; the fast travel exit button is at 159 65). The point is held for UPDATES game updates (1 to 600, default 6; the same updates that `frames` counts) and then released by the step itself. The game then sees a press, the point, and a release at the first update after the step.
- `touch release` ends a touch at once.
- `drag X1 Y1 X2 Y2 [UPDATES]` touches X1 Y1, moves in a straight line to X2 Y2 and releases. It gives one position per update (UPDATES 2 to 600, default 10).
- The step stops the script with an error that names the line, the step and the bad text. This happens for a wrong word count or a number that is not whole or is negative. It also happens for a point outside the screen or a count outside its range.
- It also stops when the running emulator was started before touch steps existed. Stop it with `--stop` and start it again.
- The touch is a pixel on the bottom screen as the game reads it; menu hit areas are the game's own. Only one finger is supported.
- Details and the measured mapping: `docs/re/oracle-setup.md`.
