# Command reference

Commands for the extraction tools, converters and programs, on Linux. For build and test setup, see [Building](building.md).

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

## Continuous integration

The workflow files in `.forgejo/workflows/` and `.github/workflows/` define the same two workflows. CI has no game data, so it never runs the tests labelled `needs-game-data`.

| Workflow | When | What it runs |
|---|---|---|
| `ci.yml` | every push to `main` and every pull request | Build with warnings as errors, the Python tests, then `ctest -LE 'slow\|needs-game-data'` |
| `full.yml` | every night and by hand (`workflow_dispatch`) | The same build and Python tests, then `ctest -LE needs-game-data`, which includes the `slow` tests |

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
- A badly formed name (not `YYYYMMDD-word`, up to three words, a date in the future, or the words `servant` or `audit`) stops with an error.
- `audit` counts the round-end lines (`Lead (<round> end)`, and the old `Lead (round N end)`) since the last line that says `docs audit: done` or `docs audit: declined`. The audit is due after ten.

## Progress report

```sh
python3 tools/progress_report.py                            # short table
python3 tools/progress_report.py --json artifacts/progress.json
python3 tools/progress_report.py --update-readme README.md  # rewrite the progress table in the README
python3 tools/progress_report.py --check README.md          # fails when the README table is out of date
```

## Priority ranking

```sh
python3 tools/priority_report.py                                    # print the ranking
python3 tools/priority_report.py --update docs/plans/priorities.md  # rewrite the file
python3 tools/priority_report.py --check docs/plans/priorities.md   # fails when the file is out of date
```

Each item in the ranking links to its line in the source view of its feature file (`?display=source#L<n>`). The line numbers change when a feature file changes: run `--update` after editing one. `python3 tools/check_doc_links.py --fix` follows moved lines; a link to an item that no longer exists fails `doc-links`.

Needs the built `build/libchase_assets_c.so` and the extracted game data. See [Progress](progress.md).

## C++ tests

The table lists some of the tests. `ctest --test-dir build -N` lists all of them. Run one with `ctest --test-dir build -R <name>`:

| Name | Notes |
|---|---|
| `input-core` | Input core, no SDL, including the `DebugOverlay` binding |
| `qol-settings` | Quality-of-life toggle store, no SDL: master state Original/Improved/Custom, per-feature switches, strict `config/qol.cfg` read and write including `window_mode` and `resolution` (`tests/qol/`) |
| `qol-menu-model` | Drawing-free model of the PC settings menu, no SDL: tabs, rows from the feature table, key and pointer events, master button cycling, toggle rows, the Display tab over the display modes, rejections (`tests/qol/`) |
| `qol-game-tab` | Model of the Game tab, no SDL: SFX and Music volume 0 to 10, Invert camera, a read-only language value and a Language row that requests the Languages page, over a fake options interface, in-sync reads, ranges and rejections (`tests/qol/`) |
| `qol-game-options` | Port side of the Game tab over the Options page state, fake state without game data: edits seen by the original Options page and the reverse, gains, Languages page request, out-of-range rejection (`tests/qol/`) |
| `qol-menu-game` | Game tab of the open settings menu, no SDL: Left, Right and Enter edit the volumes and Invert camera, the Language row ignores Left and Right and requests the Languages page, inert without a game port, and the stores applying changes and telling the port when the menu closes. |
| `qol-game-wiring` | Game tab over the real Options page, options block and save file (fake mixer): edits give the gains, leaving stores the options of slot 1, no change stores nothing, a missing or invalid `--front-save` file stops. |
| `qol-menu-list-page` | Controls list form of the PC settings menu, no SDL: four two-line bars per page, bar and highlight scene, notice and Reset all question scenes, hit areas (`tests/qol/`) |
| `qol-menu-list-page-real-data` | The same scenes drawn with the real layouts: moving the selection, hit areas on the bars and buttons (`tests/qol/`, needs game data) |
| `qol-controls-tab` | Model of the Controls tab, no SDL: waiting for a key, assign or cancel, conflicts reported without change, reset, rejections (`tests/qol/`) |
| `pad-events` | Feeds game-controller button numbers into the input state as `pad` presses and rejects unknown numbers; no SDL |
| `qol-controls-file` | Reads and writes `config/controls.cfg` entries for the Controls tab, including the `pad` source; rejections (`tests/qol/`) |
| `qol-controls-bindings` | The Controls tab joined to the SDL key names and the controls file: the file is written after each change and confirmed reset, the shipped copy is found next to it and never written (`tests/qol/`) |
| `qol-controls-default-copy` | `config/controls.default.cfg` (the shipped bindings for Reset all) is identical to `config/controls.cfg` (`tests/qol/`) |
| `qol-window-apply` | Window mode and size at startup and F11, no SDL: usable display modes, fullscreen at the desktop size by default, stored size kept or clamped, explicit size, toggle written to `config/qol.cfg`, `Fullscreen` binding rejections (`tests/qol/`) |
| `qol-view-layout` | Quality-of-life geometry, no SDL: view rectangle, black bars and aspect for a window size or an area (5:3 to 16:9), corner anchors of the HUD, rejection of windows below 400x240 and of empty areas (`tests/qol/`) |
| `qol-active-settings` | The process-wide QoL settings store, no SDL: default Original with every feature off, Improved, Custom with the switch on and off (`tests/qol/`) |
| `qol-text-check` | Settings menu text checker on a synthetic font: glyphs, widths, box fit, UTF-8 errors (`tests/qol/`) |
| `qol-menu-texts` | The list of texts the settings menu can show: rows with values, Controls names, boxes (`tests/qol/`) |
| `qol-text-check-real-data` | Every settings menu text against the real front-end font: glyphs and fit of the name plate and list bar; skips without the extracted RomFS (`tests/qol/`) |
| `qol-menu-hit` | Settings page scale rule (page centred at min(w/320, h/240)), hit areas of the round and frame, hover on entering, key and wheel mapping (`tests/qol/`) |
| `qol-ui-layout` | Settings menu layout model, no SDL or GL: UI scale from the window height, tab bar, centred row column, scrolling, slider, selector, toggle, button and dropdown rectangles and hit areas, 4.5:1 contrast of the palette, at 720p, 1080p and 4K (`tests/qol/`) |
| `qol-menu-page-real-data` | Settings page drawn from the round menu layout: setup values, scenes of each tab, element names, black bars at three window sizes; skips without the extracted RomFS (`tests/qol/`) |
| `qol-menu-redraw` | The page cache key of the settings menu includes element scales (`tests/qol/`) |
| `qol-menu-redraw-real-data` | Redraw cache of the settings menu: every cached page equals a full redraw, a page seen before is reused, timing budgets at 960x600 and 1920x1080; skips without the extracted RomFS (`tests/qol/`) |
| `qol-menu-window` | Input routing of the settings menu window, no game data: SDL key and pad names, clicks and hover through the page scale, the wheel, inputs bound while the Controls tab waits (`tests/qol/`) |
| `qol-menu-stores` | The stores behind the open settings menu: Original mode writes nothing, master and feature changes reach `config/qol.cfg` and the active store, Display changes reach the window plan, Controls changes reach `config/controls.cfg` and the running input; rejects bad display modes, a missing or malformed controls file and an unwritable `config/qol.cfg` (`tests/qol/`) |
| `qol-front-menu-real-data` | The "PC Settings" button of the slot list and the menu it opens: only on the slot list, its look and place, which events it takes while open; skips without the extracted RomFS (`tests/qol/`) |
| `qol-pause-entry`, `qol-pause-entry-real-data` | The always shown "PC Settings" entry of the pause wheel: last of five entries with the four original ones unchanged, reached by Left from Resume or a tap, chosen by A or a tap without closing the pause screen; with the real texts it opens the settings menu, holds the pause screen while open and returns on Escape; the second skips without the extracted RomFS (`tests/qol/`) |
| `qol-bottom-hook` | Hiding the bottom screen through the master state, no OpenGL context: Original and Custom keep today's top and bottom rectangles and the touch; Improved gives the top picture fitted 5:3 and centred in the window, an empty bottom rectangle, and no touch from clicks (`tests/qol/`) |
| `qol-view-hook` | The widened view hook, no OpenGL context: with the toggle off the 3D view region equals today's top screen for a grid of window sizes; with it on (set through the menu model) the pinned widened regions above the bottom screen, and the rejection of an empty top rectangle (`tests/qol/`) |
| `qol-hud-hook` | The corner HUD placement, no OpenGL context: with the toggle off every part's rectangle equals today's top screen for a grid of window sizes; with it on the pinned corner rectangles of each part, in 16:9, 21:9 and tall windows (`tests/qol/`) |
| `qol-hud-parts-hook` | The corner HUD draw plan, no OpenGL context: with the toggle off no part is drawn separately; with it on the five parts map to the port's HUD parts, get the pinned rectangles and the canvas is cleared before each (`tests/qol/`) |
| `qol-front-panel` | The Improved front end panel, no OpenGL context: no panels in Original and Custom, the fitted top picture and the centred 4:3 panel for windows wider and narrower than 4:3, the window-to-canvas click mapping with clicks outside the panel, the row and A/B/Y click classifier and the darkening (`tests/qol/`) |
| `qol-front-panel-point` | The slot page "PC Settings" click point: stacked screens in Original, the centred panel in Improved (`tests/qol/`) |
| `qol-minimap` | The round minimap, no OpenGL context: Original and Custom draw nothing, Improved draws once at the pinned top-right rectangle under the counters (diameter one fifth of the window height), the disc mask, and no draw while the scanner overlay, a dialog or speech box, the scene change page or the fast-travel map is open (`tests/qol/`) |
| `qol-minimap-real-data` | Needs the game data: the lowest row of the studs and super brick counters in the top canvas equals the counters edge the minimap placement uses (`tests/qol/`) |
| `render-viewport-rect` | View region of the top screen, no OpenGL context: default 400:240, the 16:9 projection scale, rejection of non-positive rectangles and aspects (`tests/render_viewport_rect_test.cpp`) |
| `canvas-clip` | `Canvas::drawScaled` with a destination inside, partly outside, covering and wholly outside the canvas (also flipped) equals the same draw on a larger canvas, and only canvas pixels are visited (`tests/canvas_clip_test.cpp`). |
| `render-overlay-quad` | Placement of an overlay canvas on a window rectangle, no OpenGL context: clip-space corners and texture coordinates against hand-computed values (`tests/render_overlay_quad_test.cpp`). |
| `render-ui-batch` | Batching of the UI draw list, no OpenGL context: vertex counts, areas of plain and rounded fills, borders and lines, scissor runs, nested clips and recorded text commands against hand-computed values (`tests/render_ui_batch_test.cpp`). |
| `qol-display-tab` | Display tab store and rows, no SDL: `window_mode` and `resolution` keys of `config/qol.cfg` with the desktop-size default, window mode and resolution rows, custom size, list cycling, clamp to the largest display mode, no `resolution` line while the desktop size is in use, rejections (`tests/qol/`) |
| `debug-metrics-core` | [Debug overlay](debug-overlay.md) values on synthetic input: frame-time window, `/proc/self/status` fixture, CPU percent, overlay lines |
| `debug-text-core` | Debug overlay font and panel raster |
| `ui-text-core` | Bundled UI font: UTF-8 decoding, glyph coverage of Latin-1, Cyrillic and the ten language names, widths and kerning, glyph atlas, text layout and clipping (no OpenGL) |
| `qol-bundled-font` | Bundled font, its OFL text and stb_truetype exist, their SHA-256 are pinned and `REUSE.toml` records their sources |
| `debug-overlay-off`, `debug-overlay-on`, `debug-overlay-area` | Two offscreen screenshots without and with `--debug-overlay`; the third checks that they differ only in the top-left panel area. Registered only if `artifacts/romfs` existed at configure time |
| `front-overlay-off`, `front-overlay-on`, `front-overlay-area` | The same three checks on the front-end language screen (`--front-script "until language 60; run 1"`). Registered only if `artifacts/romfs` existed at configure time |
| `script-text`, `script-text-real-data` | Script text natives (tip text, dialogue lines, mission title) on synthetic scripts and on `1-1_OnScreenText_Script` of the real level; the second skips with code 77 without `artifacts/romfs` |
| `tip-text-shot`, `tip-text-panel` | An offscreen screenshot of the first tip of mission 1-1 and a check of the tip panel (`tools/check_tip_shot.py`). Registered only if `artifacts/romfs` existed at configure time |
| `police-entrance-shot`, `police-entrance-no-disc` | An offscreen screenshot at the police entrance (script `Police_ExtToInt`) and a check that no pale blue-grey disc lies under the player. Registered only if `artifacts/romfs` existed at configure time |
| `game-option-debug-overlay-value` | `--debug-overlay` takes no value |
| `input-sdl-smoke` | Runs with `SDL_VIDEODRIVER=dummy` |
| `map-buttons`, `map-buttons-real-data` | The map buttons of the lower screen: click areas, touch phases, the characters tap and hold, the fast travel page and its exit on made-up boxes, then the shipped layout's button boxes in play, with the fast travel page open and after its exit against the original's; the second skips with code 77 without `artifacts/romfs` |
| `fence-vault`, `fence-vault-real-data`, `game-fence-vault-1-3` | The fence vault (the B press near a fence): zone, use point, rise of the clip and the sequence with a made-up fence, then the fences of the first level and the clip `vault_fence` (needs the extracted RomFS), then `chase-game` vaulting over the fence of mission 1-3 (needs the extracted RomFS) |
| `ladder-sounds`, `ladder-sounds-real-data` | The sound tracks of animation clips (ladder climb and clamber sounds): made-up clips, then the shipped ladder, jump, idle and walk clips; the second skips with code 77 without `artifacts/romfs` |
| `assets-core` | Asset runtime, synthetic fixtures |
| `shadow-core`, `shadow-real-data` | The shadow constant, focus and shadow camera against values read in the original, the shadow light and the caster and receiver flags; the second checks the shadow light of lvl_01 and the flags of shipped classes and skips with code 77 without `artifacts/romfs` |
| `assets-real-data` | Needs `artifacts/romfs`; skips with code 77 otherwise |
| `layout-text-real-data` | Layout text spacing and glyph scale: the drawn widths of four texts and the columns of the Build-it prompt text, compared with values measured in the original; skips with code 77 without `artifacts/romfs` |
| `layout-scale-real-data` | Layout drawing at a scale: element opacities and scales set by code, text through the node scale, element boxes through references, the shortcut bar scene; skips with code 77 without `artifacts/romfs` |
| `reflection-core`, `reflection-real-data` | Reflection cube maps: which texture unit samples the cube map, the texture coordinates of the other units and the order of the six faces; the second checks every shipped reflection material (each has a cube map on its first unit) and skips with code 77 without `artifacts/romfs` (slow) |
| `pregen-traffic` | The traffic and pedestrian spawn files: made-up files, rejected input and the cell lookup. No game data needed |
| `pregen-derive`, `pregen-derive-real-data` | The fields derived from the spawn files (path, direction, room) and the floor cell of a traffic point (`findTrafficFloorCell`, the lookup 0x1edaac): made-up paths and floors; the second runs them on the shipped files and skips with code 77 without `artifacts/romfs` |
| `traffic-manager` | The rules of the traffic manager (spawn, removal, fade, vehicle limit) on made-up cars. No game data needed |
| `traffic-system` | The traffic manager fed with a made-up level and spawn file: spawn at the points with the height offset, the driver steps, removal beyond the radius, `EnableTrafficAndPedestrians(0)`, `SetTrafficMaxVehicles` and its rejection of a wrong argument count. No game data needed |
| `traffic-system-real-data` | The same on lvl_01: the car heights and directions against values read in the original, ten cars, limit 10, cars spawn and leave around the player; skips with code 77 without `artifacts/romfs` |
| `game-traffic`, `game-traffic-natives` | `chase-game` in lvl_01 from the level start: cars spawn at their points and leave again (300 updates), and `1-4_RobberCaught` switches the traffic off (every car leaves in one update) and sets the vehicle limit (420 updates). Slow; registered only if `artifacts/romfs` existed at configure time |
| `pregen-traffic-real-data` | Decodes the 14 shipped spawn files and checks counts and checksums; skips with code 77 without `artifacts/romfs` |
| `ai-driver-input` | The AI driver of the traffic cars: its states, the state changes after a spawn and the stop rule, on made-up cars and routes. No game data needed |
| `traffic-route`, `traffic-route-real-data` | The routes of the traffic cars (route list, EnableTrafficPath, path follower, junction test) on made-up data, then the junctions and routes of the 7 shipped traffic levels; the second skips with code 77 without `artifacts/romfs` |
| `viewer-real-smoke`, `viewer-cutscene-smoke` | Render one model and one cutscene frame offscreen. Registered only if `artifacts/romfs` existed at configure time |
| `game-real-smoke` | Registered only if `artifacts/romfs` existed at configure time; needs libepoxy |
| `game-mission-1-1`, `game-mission-1-1-start` | Mission 1-1 from the first waypoint to its end in one run (under a minute), with `at X Y Z` teleports between the stops; it checks the order of the scripts and cutscenes, not their update numbers, and the level start (intro movie, cutscene `1_1_a`). Registered only if `artifacts/romfs` existed at configure time |
| `game-script-run`, `game-script-walk`, `game-script-camera` | Same conditions. Feed a fixed stick to the development runtime and check how far the character moved. The first two switch the camera's wall test off (`--no-camera-collision`), the third leaves it on |
| `camera-real-data` | Compares the follow camera with traces recorded from the original game (committed in `tests/data/camera/follow` for open ground and `tests/data/camera/wall` near walls); the wall traces need the RomFS and are skipped without it |
| `camera-sweep-real-data` | Checks the camera's wall test against the sweeps recorded from the original game (committed in `tests/data/camera/sweep.csv`); needs the RomFS and is skipped without it |
| `camera-sweep` | Checks the camera wall test on small made-up walls and boxes (no game data needed) |
| `camera-blocker` | Checks that the camera stays where it was while the character is inside a camera blocker area, with made-up areas (no game data needed) |
| `locomotion-real-data` | Compares the character's movement, run timer and stick-to-heading with rows recorded from the original game (kept in `tests/data/locomotion_traces.inc`); skips with code 77 if `artifacts/romfs` is missing |
| `jump-real-data` | Compares the jump (height curve, forward speed, landing and the stand-still after it) with rows recorded from the original game (kept in `tests/data/jump_traces.inc`); skips with code 77 if `artifacts/romfs` is missing |
| `fall-real-data` | Compares falling, landing, the ground normals and the stand-still after a landing with rows recorded from the original game (kept in `tests/data/fall_traces.inc`); skips with code 77 if `artifacts/romfs` is missing |
| `slide-real-data` | Compares the slide down a steep slope (step-off state 8) with rows recorded from the original game (kept in `tests/data/slide_traces.inc`); skips with code 77 if `artifacts/romfs` is missing |
| `slope-real-data` | Compares the run slow-down on slopes with run rows recorded from the original game (kept in `tests/data/slope_traces.inc`); needs no game data |
| `collision-real-data` | Compares the character's floor height and wall sliding, also against lamp posts and barriers, with samples recorded from the original game (kept in `tests/data/collision_ground_samples.inc` and `tests/data/collision_wall_samples.inc`); skips with code 77 if `artifacts/romfs` is missing |
| `nested-object-collision-real-data` | Checks the collision of the super build-it plate and the built prop of the helipad of mission 1-3 (shapes from the original's collision grid, enabled before and after the build) and the objects that class files nest; skips with code 77 if `artifacts/romfs` is missing |
| `game-helipad-collision` | Builds the helipad of mission 1-3 in `chase-game` and pushes the character into its edge: he must stop at it (slow test, needs game data) |
| `options-page`, `languages-page`, `item-grid`, `item-grid-real-data`, `pause-menu`, `game-pause-menu`, `game-options-reload` | The pause menu, its Options page and its Languages page: turning and choosing, Up and Down changing the sound effects and music volume, Invert camera, Back saving a change, the language list (`languages-page`: rows, A, B and the change without game data), the item grid it is shown in (`item-grid`: tiles, taps, page change, open and close without game data; `item-grid-real-data`: the click areas and storyboard lengths of the layouts and the flags drawn on the Languages page), on made-up values and on the values read from the original (`options-page`, `pause-menu`); a `chase-game` run that opens the pause menu, lowers the music volume on the Options page and saves it with B, and a second run from that file whose level load takes the saved volumes. `item-grid-real-data` and the two `chase-game` tests need the extracted RomFS; `game-pause-menu` is slow |
| `superbuildit-messages` | Checks the messages of the super build-it class (reset, close, the helipad story fix), the save byte, the restore of a built plate and the record in the level state |
| `soundtest-real-smoke`, `soundtest-id-smoke` | Play one sound by file name and one by number on the dummy audio driver. Same conditions |
| `progress-tables` | The tables behind the progress report: which script commands, object types and file types the port handles. No game data needed |
| `progress-readme` | Fails when the progress table in `README.md` is out of date (`tools/progress_report.py --check`); skips with code 77 if `artifacts/romfs` or the built library is missing, or on a branch other than main (`tools/check_on_main.py`) |
| `priorities-current` | Fails when `docs/plans/priorities.md` is out of date (`tools/priority_report.py --check`); skips with code 77 if `artifacts/romfs` or the built library is missing, or on a branch other than main |
| `no-inline-shaders` | Fails when a `.cpp`, `.hpp` or `.h` file under `src/` contains shader code. Shaders live only in `.glsl` files under `src/render/shaders/` (see `CLAUDE.md`) |
| `gl-only-in-render` | Fails when a `.cpp`, `.hpp` or `.h` file under `src/` outside `src/render/` contains an OpenGL call, `<epoxy/gl.h>`, a GL type or a GL constant. Rendering code goes into `src/render/` behind plain-type headers (see `CLAUDE.md`). `gl-only-in-render-rejects` checks that the guard fails on the bad file in `tests/gl_only_in_render/` |
| `qol-separation` | Fails when `qol::` or `chase_qol` appears in a `.cpp`, `.hpp` or `.h` file of `src/` or `tests/` outside the QoL directories (src/qol, tests/qol), when a port file includes a header from src/qol without a hook line, when the number of `qol::` lines per port file differs from the "Hook points" list of `docs/plans/features/qol-settings-menu.md`, or when a file in src/qol is an identical copy of a port file. `qol-separation-accepts` and `qol-separation-rejects-<fixture>` run the same script on the trees in `tests/qol_separation_fixtures/` and check that the rejection names the file |
| `game-cpp-wiring` | Fails when `src/game.cpp` has more than 400 lines. That file only wires features together. New game logic goes into its own file under `src/runtime/` (see `CLAUDE.md`) |
| `no-local-data` | Fails when a tracked text file names data that exists on one machine only (`tools/check_local_data.py`): an `artifacts/` folder other than the ones the documented extraction creates, an emulator folder under `azahar-emu`, a `/home/<name>/` path, the name of an agent's output folder or of a `.mem` trace file, a distro package location outside the install sections, or `--state NAME` / `loadstate NAME` in a doc, test, source file or oracle script (`tools/oracle/*.txt`) where no committed script saves that state. It only warns (exit code 0) about wording such as "this machine" in `docs/re` and user names. Each hit is printed as `file:line` with the fix: write the value into the test or the `docs/re` entry with its provenance. A few files that describe the oracle tool's own output folders are exempt in the tool. Run it alone with `python3 tools/check_local_data.py`. |
| `test-labels` | Fails when a test that needs the game data (its command names the RomFS folder, it skips with code 77, or it shares a fixture with such a test) lacks the label `needs-game-data` (`tools/check_test_labels.py`) |
| `doc-links` | Checks the documentation rules (`tools/check_doc_links.py`). It fails on a broken link or anchor, a dated entry without an anchor, a `docs/re/` or `docs/design/` file over 40 KB, a feature file over 16 KB, a feature index row missing, duplicated, over 300 characters or after a table without a blank line, an item line with fields out of order, an open item with `needs:` or `blocks:` written on one side only (or with both items naming each other in the same field), a new hex address in an open item line or a new item ID or address in a session log line (lines new compared to `HEAD`), a timestamp in the future, an item or stand-in ID defined twice in a feature and its closed file, a feature file without a row in the feature index (or a closed-items file with one), more than 20 entry lines in the session log, `CLAUDE.md` over 23 KiB, a correction in `docs/re/` that is not marked at both ends, or, in `docs/design/` (which holds only the current version of each decision), a `Corrected by` or `Superseded by` line, a `Corrects` link, a `Current state` section or a history file. It also checks the user docs (`docs/*.md` outside the subfolders, and `README.md`) against the code (`tools/user_docs.py`): it fails when a command-line option in `src/` or a `chase-*` program of `CMakeLists.txt` is named in no user doc, when a user doc names an option or `chase-*` program that does not exist, or when a user doc names a `tools/`, `src/`, `tests/` or `docs/` path that does not exist. It warns about a new `:00` timestamp. `--fix` also moves entries with a `Superseded by` line to `docs/re/<topic>.history.md`. Add `--fix` to repair anchors and links, move closed items (`- [x]`) to `<feature>.closed.md`, add the missing side of a `needs:` / `blocks:` pair and move session log lines beyond the newest 20 to `docs/plans/<plan>.progress-archive.md`. |
| `particles-core` | Particle emitters and particles on made-up definitions: start limits, creation with sub-steps, step, bounce, ageing and stop, quad corners, uv and near fade; no game data needed |
| `debris-core` | Brick debris: bursts, flight, bounce, rest, blink and reuse of the records, the distance fade, the wall plane, the prop burst values, the white flash material, and a replay of eleven oracle bricks of a van contact and a lamp post; no game data needed |
| `draw-order`, `draw-order-real-data`, `semi-trans-near-camera` | The scene's draw list: sort keys, the sort, which parts are drawn see-through, the part boxes of every shipped model (needs the game data), and the rule that makes the waypoint columns see-through near the camera |
| `debris-pickup` | Pickup of super debris bricks: the pickup test and skip counter, the magnet and collected states, the add to the super brick count and 10 studs, the oracle's 30 and 31 update cycle, the HUD pending add; no game data needed |
| `game-debris-super-pickup` | (slow) The seated police car smashes Bus_Shelter_01 and its super bricks reach the super brick count and the studs |
| `stud-drop` | Dropped studs: which pool records a drop takes, how the count splits into blue, gold and silver studs, the heart of mode 2, the ten-record limit, start velocity, flight, bounce, landing sound, rest, life, blink, pickup and banking, and the spawn request of `SpawnDir` / `SpawnLocal` / `SpawnCone`. |
| `particles-real-data` | All 298 shipped particle definitions run for 3 s with drawing, records 175 and 53, and the siren-light emitter recorded in the oracle (angles, sizes, uv and flips bit for bit); skips with code 77 if `artifacts/romfs` is missing |
| `studs-core` | Coin pickup rules on made-up coins, no game data needed |
| `studs-real-data` | Replays coin pickups recorded from the original game (committed in `tests/data/studs`); needs no game data |
| `audio-mixer` | Mixer core, no SDL |
| `audio-core` | Sound rules on made-up values (distance volume, panning, level music choice, footstep detection, landing sounds), no game data needed |
| `voice-params` | Volume and rate arithmetic of sound voices (scale, boost, pitch, Doppler), no game data needed |
| `voice-params-real-data` | Plays real sound effects and checks SetSoundVolume, ScaleSoundFrequency, master volume and Doppler on the mixer output, needs the RomFS |
| `audio-real-data` | Checks the sound tables and replays sound calls recorded from the original game (the recorded calls are pinned in the test; made with `tools/export_sound_trace.py`) by running `chase-game --script`; skips with code 77 if `artifacts/romfs` is missing |
| `natives-sound`, `entity-ids-real-data` | The ten sound natives on made-up scripts, and the owner key of an object's sounds (its entity id); the second checks the object flags of the shipped levels and skips with code 77 if `artifacts/romfs` is missing |
| `runtime-core` | Game runtime core (frame timing, pad, stick, character movement, floor height and wall sliding), no SDL |
| `script-dialog-real-data`, `game-script-dialog` | The police station entry script: the question box opens, A and B give their answers, the character is moved to the door. Registered only if `artifacts/romfs` existed at configure time |
| `same-level-entry-real-data`, `game-same-level-entry` | The lift of the police station between its floors: the player lands on the destination point one update after the request, without a loading page. Registered only if `artifacts/romfs` existed at configure time |
| `scene-change-real-data`, `game-scene-change-exit`, `game-scene-change-entry` | Walking out of the police station and back in: where each door leads, the landing points from the original, the order loading page, A, switch. Registered only if `artifacts/romfs` existed at configure time |
| `prop-rocking`, `prop-rocking-real-data` | The script commands SetPropRocking and WaitForPlatform, which props can rock and the swaying of rocking props, with made-up objects; the second checks the generator scene of mission 1-3 (`1-3_Generator_Script` started by name), the girder worker attached to the girder by the level's start script, and the targets of these commands in all levels, and skips with code 77 without `artifacts/romfs` |
| `node-attach` | Objects attached to another object (the script command AttachObject): they keep their place when attached and then move and turn with the parent, with made-up matrices |
| `attach-children` | Placed objects attached to a placed parent: their world is their local matrix times the parent's world (parents first), a repeated AttachObject takes the local matrix again, and a node move of the child itself shifts it in the parent's frame, with made-up matrices (no game data needed) |
| `ai-core` | The AI natives (flags, requested states, moves, AIMove event) with made-up characters and scripts, no game data needed |
| `ai-real-data` | Runs the real police station entry script: the player walks to the door at 0.11 per update, then the scene change follows; skips with code 77 without `artifacts/romfs` |
| `nav-grid` | Nav grid lookups (position to room, grid and cell) on a made-up grid, the nav file name and the loader fix-up, hand-evaluated values |
| `nav-path` | The AIMove path finder and the follower on small made-up grids: same cell, straight line, search around a wall, two linked grids, the walk across the link, a refused gate, a start beside a grid |
| `nav-path-real-data` | The finder and follower on the real grids of lvl_01_cherrytreehills: the police entry case, and 3000 pairs of positions followed inside and across grids; skips with code 77 without `artifacts/romfs` |
| `script-runner` | How level scripts run each update (waits, a script calling or starting another one, conditions, door requests, scripts started by walking into marked places), with made-up scripts, no game data needed |
| `reference-trigger`, `reference-trigger-real-data` | The step that joins the "object triggered" references of build-its, switches, doors and other objects (all references to one script object must be active before it fires; switching them off again; the events and messages it sends), with made-up references; the real-data test checks the references of the eight levels (the Downtown fires and engine parts) and the finished build of the donut van; skips with code 77 without `artifacts/romfs` |
| `switch`, `lever-use`, `lever-use-real-data`, `game-garage-key-1-3`, `game-key-use-1-3` | Switches (levers, key switches, light switches): using one, how it turns on (after its own animation), stays on or switches off again after its time, and how it fires the script object it names, on made-up switches; how the character walks and turns to a lever, plays its animation and when that uses the lever, on made-up values and on the extracted game data (the animations, the place in front of each switch); a `chase-game` run of mission 1-3 at the key switch (A turns the key and the garage door opens); and the real path of mission 1-3 from the construction yard: pick up the key, put it into the keyhole with A, A again turns it and the door opens. `lever-use-real-data` and the two `chase-game` tests need the extracted RomFS; the last one is slow |
| `platform-node-move`, `platform-path`, `platform-node-move-real-data` | Moving platforms: how a trigger starts a platform, how far it travels each update, where its travel ends and how objects attached to it move along, with made-up platforms; the second checks platforms that follow a path (where on the path, how they turn, objects turning with them) with made-up paths; the third checks the crane platform and the lawn mowers of mission 1-3 and the objects the level attaches to them (needs the extracted RomFS) |
| `platform-riding`, `platform-riding-real-data`, `game-ledge-lift-1-3` | Riding a moving platform: the script commands that set a platform's speed, start and end, the reset of a platform, how far a platform carries the character standing on it, and the use zone of a lever turned with the lever, with made-up values, no game data needed; the second checks the ledge lift of mission 1-3 from the level data; the third pulls the switch on the ledge lift of mission 1-3 and checks that Chase rides it to the top. The last two need the extracted RomFS; the last one is slow |
| `script-particles`, `game-generator-1-3` | The script command PlayParticlesGO (which effect, where) with made-up scripts, no game data needed; and the real path of mission 1-3 from the parts crate: carry the parts to the base plate, build the generator switch, pull it with A, and the crane scene runs to its end (`1-3_Generator_Script`), with the crane platform's travel down and a lap of the lawn mower. The second needs the extracted RomFS and is slow |
| `pushable`, `pushable-real-data`, `game-push-cart-1-3`, `game-crane-1-3`, `mission13-end` | Pushable objects and the player pushing them: the first uses made-up objects, no game data needed; the second checks the push cart of mission 1-3 from the level data; the third breaks the cart crate, builds the push cart and pushes it until `1-3_Push_Done` starts; the fourth builds the crane of mission 1-3, pulls the crane switch and runs the mission end script, including Chase's jump from the bridge and fall to the ground (the log lines `player AI move: ...`). The fifth tests the log of a scripted jump with made-up positions. `pushable-real-data`, `game-push-cart-1-3` and `game-crane-1-3` need the extracted RomFS; the last two of them are slow |
| `screen-fade` | The screen fade: timer, opacity of the black quad and which screens it covers; no game data needed |
| `cutscene-camera` | The cutscene camera: track values between keys, the field of view from the camera's lens and the camera frame over time; no game data needed |
| `dialog-bg-fade` | The panel alpha of the dialogue background: +0.2 per update while a tip is shown or BlackoutMap is set, else -0.2; no game data needed |
| `natives-world` | The script natives DestroyProp / KillObject (destroy rules), EffectFadeObject (fade values, hidden state, full table), EnableDistanceFade, AddToSemiTransNearCameraList, SetPartyCharacter and StreamRoomIn (request and wait) with made-up scripts, no game data needed |
| `natives-world-real-data` | The same natives in the real mission 1-2 scripts of lvl_01_cherrytreehills (the police car fade-in, the cage key script waiting for Room40); skips with code 77 without `artifacts/romfs` |
| `use-marker` | The use marker on the ground at usable spots: fade in and out, hold, range of 25 m, the turning quad, the kind-to-ability-to-texture tables and the placement kept per slot, with made-up spots; no game data needed |
| `use-marker-real-data`, `game-use-marker-detective-1-2` | The ability tables of the character rows (Chase, the all-abilities row 167, the locked abilities) and the detective spot `1-2_Dtv_DogSpot_02`; a `chase-game` run in which its police marker shows and goes in detective mode. Need the extracted RomFS |
| `carry-arrows`, `game-key-pickup-1-3` | The green carry arrows (over a carry object that can be picked up, over the place where the carried object belongs) on made-up objects; and a `chase-game` run of mission 1-3 from the construction yard through the detective clue to the key: A too far from the key does nothing, A next to it picks it up and the arrow moves to the keyhole. The second test is slow and needs the extracted RomFS |
| `game-cage-release-1-2` | Mission 1-2: A on the dog cage spot releases the dog; the screen goes black, the cutscene fades in, and after it the mission end script runs with the game picture back (`screen brightness 1`). Slow. Registered only if `artifacts/romfs` existed at configure time |
| `game-stream-room-in` | Runs `1-2_Cage_Key_Target_Script` in `chase-game` and checks that the cutscene starts after Room40 finished loading. Registered only if `artifacts/romfs` existed at configure time |
| `object-rooms` | The rule that moves an object to another room when it leaves its room's box, and which rooms are checked each update, with made-up rooms, no game data needed |
| `dcam` | The script camera: its path curves, start and end waits, the end after the set time and the blocking of `WaitForDCam`, with made-up paths, no game data needed |
| `dcam-real-data` | The camera paths of the first level against what the original showed in the police station entry (positions of the camera, the update on which the camera ends); skips with code 77 if `artifacts/romfs` is missing |
| `save-core` | Save file checks, synthetic data, no SDL |
| `save-real-data` | Completion totals from the game tables; needs `artifacts/romfs`, skips with code 77 otherwise |
| `fmv-real-smoke` | Registered only if FFmpeg and `artifacts/romfs` existed at configure time |
| `frontend-flow` | The order and triggers of the start screens and the StreetPass pages, with a made-up movie, no game data needed |
| `frontend-real-data` | Start-screen texts, line wrapping, exact logo pixels, page pictures and the length of the two movies; skips with code 77 if `artifacts/romfs` is missing |
| `front-game-smoke`, `front-game-handoff`, `frontshot-real-smoke` | Run the start screens with a scripted pad: to the language list, through every screen into the level, and offscreen with `chase-frontshot`. Registered only if `artifacts/romfs` existed at configure time |
| `award-viewer`, `award-viewer-real-data`, `game-award-viewer` | The awards screen of the police station: how the bronze, silver and gold awards are counted, the wheel's items, turning, Back and closing, with made-up values, no game data needed; the second checks the texts, pictures and award totals of the shipped tables and skips with code 77 without `artifacts/romfs`; the third presses A on the award viewer spot of the police station in `chase-game`, turns the wheel and closes it with B (slow, registered only if `artifacts/romfs` existed at configure time) |
| `map-file`, `map-file-real-data` | The level map files of the bottom screen: the 84-byte layout against the pinned values of the eight shipped files, the rejection of a file of another size and the map name rule (the police station uses the first level's map); the second compares the shipped files and the level table and skips with code 77 without `artifacts/romfs` |
| `map-hub` | The hub rule of the bottom-screen map: the fixed centre and angle literals of the original, the generic record rule and the placement against the measurement in the police station lobby |
| `hud-real-data` | The display during play: the stud bar's timing against a recorded pickup (the states are pinned in the test), the icon bounce, the hearts, pictures of the display and the map screen (also with the scanner buttons sliding in and out); skips with code 77 if `artifacts/romfs` is missing |
| `hud-parts` | The HUD part set without game data: set operations and the page layout elements each part owns (`tests/hud_parts_test.cpp`). |
| `hud-parts-real-data` | The HUD drawn part by part: every part alone leaves its own opaque pixels and together they equal the full draw byte for byte (`tests/hud_parts_real_test.cpp`; needs game data, skips with code 77 without). |
| `scanner-bottom` | The lower screen of the sound scanner: the map buttons that slide in and out when the scanner opens and closes, their looks and click sounds, and the back-arrow button (where a touch hits it, that only a fresh touch counts, and that it leaves the scanner like B), with made-up phases, no game data needed |
| `game-script-hud` | Runs the level offscreen with the character standing on a coin and checks that the coin was banked and the display shows the new total. Registered only if `artifacts/romfs` existed at configure time |
| `game-vehicle-room` | Drives the police car of mission 1-1 forward from the spot where it used to vanish and checks that it moves from Room15 to Room21 with the player, so it stays drawn when Room8 unloads. Registered only if `artifacts/romfs` existed at configure time |
| `game-room-streaming` | Walks the character from Room7 into Room8 in the street level and checks the room messages: nine rooms at the start, one room unloaded on entering Room8, one more loaded nine updates later. Registered only if `artifacts/romfs` existed at configure time |

## Merging an agent branch

```sh
python3 tools/merge_agent.py sync                    # fetch, then compare main with its upstream
python3 tools/merge_agent.py prepare BRANCH          # sync, squash the branch into main's staging area, then run the checks
python3 tools/merge_agent.py check                   # run the checks again after fixing conflicts or review edits
python3 tools/merge_agent.py finish BRANCH -m FILE   # checks, commit with the message in FILE, remove worktree and branch
python3 tools/merge_agent.py finish BRANCH -m FILE --release 20261021-audio-A --log linux-port LINE_FILE
python3 tools/merge_agent.py close -m FILE --release 20261021-audio-C --log linux-port LINE_FILE
```

- `sync` compares main with its upstream (`--upstream REMOTE/BRANCH` names another one). When the upstream is ahead and main has no own commits, it fast-forwards main. When both moved, or main has uncommitted changes, it stops with an error, and the user decides.
- `prepare` runs `sync` first and takes the same `--upstream`. `--no-sync` skips it, for a session without a remote.
- `finish --release WHO` drops the ID ranges WHO holds, inside the merge commit; give it once per holder. A name that holds no reservation stops with an error.
- `finish --log PLAN LINE_FILE` appends the round-end line to the session log of the plan's progress file (`docs/plans/<plan>.progress.md`) inside the merge commit. LINE_FILE holds exactly one line `Lead (<round> end): ...` without a timestamp; the tool adds the time from the clock. Any other text stops with an error.
- `finish` removes the worktree of a Claude agent even while the agent's lock is still set. A worktree with any other lock stays: `finish` warns and prints the `git worktree remove` and `git branch -D` commands, and still reports the commit.
- Both are safe to repeat: after a failed check, run `finish` again with the same options.
- `prepare` and `check` record a fingerprint (staged tree and main's HEAD) in the git directory (`merge-agent-verified`, untracked) once build, ctest and Python tests passed. When `finish` starts with the same fingerprint, it skips those three and prints one line saying so; the doc and README checks still run. A failed check deletes the record; a different tree or a missing record runs everything.
- `close` makes the same release and log line as its own commit, for a round whose last branch was dropped. It needs at least one of the two options.

The checks stop on game files, screenshots or other binary files (except bundled fonts `third_party/fonts/*.ttf`), broken doc links, build warnings and failing tests. They also refresh the README progress table. Conflicts in the README progress table or the trend graphic are fixed by making them again; conflicts in a session log archive keep the lines of both sides, and lines that both sides added to a session log, the feature index, the ID reservations or a `docs/re/` topic are both kept. Other conflicts you fix by hand, `docs/design/` ones included. The message file must end with the merging agent's own `Co-Authored-By:` line. The script never pushes.

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
- `chase-hudshot` draws the display of the level (portrait, hearts, stud counter) offscreen and saves it as a BMP (over black) with a mask BMP, or saves the bottom-screen map. Example: `build/chase-hudshot --output /tmp --script "run 90; shot start; studs 0 10; run 40; shot bar; map lvl_01_cherrytreehills 886.764 1111.166 -2.35 -2.35 map"`. Commands: `run N` (updates of 1/30 s), `hearts N`, `studs TYPE TOTAL`, `brickcount TOTAL`, `brick` (a super brick pickup), `brickbank TOTAL`, `shot NAME`, `map LEVEL X Z CAMERAYAW PLAYERYAW NAME`, or the pause menu `pause` (open it), `press START|A|B|LEFT|RIGHT|UP|DOWN` (one update with that press) and `touch X Y N` (touch the lower screen at X, Y for N updates, then let go), `marker X Z` (add a marker at that world position to the map pictures that follow) and `ring SECONDS` (set the time of the markers' ring animation). After `pause`, `shot NAME` saves both screens as one 400x480 picture. Example: `build/chase-hudshot --output /tmp --script "pause; run 50; shot open; press RIGHT; run 20; shot options"`. Options: `--output` (the folder must exist), `--fib`, `--names`.
- `chase-input-lab` looks for `config/controls.cfg` relative to the current folder. CMake copies it into `build/`. See [Controls](controls.md).

## Development runtime

Needs libepoxy and FFmpeg. This is **not the playable game**. It is an early test program: it shows the start screens of the original game and then a level, where the main character walks, runs, jumps and sprints, with parts of the first missions. Large parts of the game are missing. For a plain-language guide, what works and what does not, see [Development runtime](game-runtime.md) and [Project status](status.md).

```sh
build/chase-game
build/chase-game --gameplay
build/chase-game --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl
SDL_VIDEODRIVER=offscreen build/chase-game --gameplay --screenshot out.bmp --frames 2
SDL_VIDEODRIVER=offscreen build/chase-game --front-script "until language 60; run 1" --screenshot out.bmp
SDL_VIDEODRIVER=offscreen build/chase-game --script "0 150 40" --screenshot out.bmp
```

- Bad input stops the program at once with an error that names the option and the bad text, for example `chase-game: --slot: out of range, allowed 0 to 1: '2'`. Numbers must be whole and in range, `--start` needs exactly three numbers, and each `--script` and `--front-script` step must follow its format.
- Without `--gameplay`, `--level`, `--script` or `--start`, the program starts with the screens of the original game (see [Development runtime](game-runtime.md)). `--front-script "..."` runs those screens with a scripted pad (same commands as `chase-frontshot`) and saves the window with `--screenshot`; if the script reaches `done`, the level follows. `--language 1-9` and `--returning` choose the language and a returning player. `--front-save FILE` shows the slots of your own `savegame.dat` in the slot list; A on a used slot continues in the level that slot was last in, with its progress (not together with `--gameplay`, `--script`, `--level`, `--start` or `--save`). Escape quits.
- Options: `--fib`, `--names`, `--config` (as in the viewer), `--gameplay` (start in the level), `--window WIDTHxHEIGHT` (starts windowed at this size instead of the stored window mode and size, see [Window mode and size](game-runtime.md#window-mode-and-size); 400x480 gives the size of the emulator screenshots), `--level`, `--screenshot`, `--frames`, `--script`, `--start`, `--save FILE --slot 0|1` (start from a real save, for example your own `savegame.dat`: the program starts the level that the slot was last in, where the original's Continue starts it, with the missions done, unlocks, level variables, studs and super bricks of the slot; collected items and object states do not yet come from the save. The slot must be in use, an empty slot stops with an error. `--level` starts another level with the progress of the slot, and `--start` / `--facing` place the player yourself. Only the police station was checked against the original; other levels use the level's start point too, which is not checked yet. Without `--save` a run is a new game in slot `--slot` (default 0) with the original's starting progress), `--save-out FILE` (where save requests go; without it they go to the port's save file `$XDG_DATA_HOME/chase/savegame.dat`, normally `~/.local/share/chase/savegame.dat`. Every save request of the game, for example the level scripts' QuickSave or a collected super brick, writes the slot of this run into that file; the other slot and the options of an existing file are kept. This also happens in development runs with `--level` or `--script`, so give `--save-out` to keep your own save untouched. It must not be the `--save` file. The level state of the slot is not refreshed yet), `--call-script NAME` (development aid, may be repeated: starts the level script NAME after the level's own start, for example `1-1_MissionSkip` to switch on the police station door; after a switch to another level it runs again there if that level has the script), `--set-variable NAME=VALUE` (development aid, may be repeated: sets a variable of the level before its start scripts run. NAME must be a variable of the level, a number is required. The police station plays a camera tour on the first visit, `Lobby_Dcam_Intro=0`, and the camera holds the player; `--set-variable Lobby_Dcam_Intro=1` starts after the tour. It is not applied again after a switch to another level), `--super-bricks N` (development aid: sets the super brick count, 0 to 4294967295, at the start of the run; it wins over the count of `--save`. The game saves the count as it is then, like picked-up bricks. It is not applied again after a switch to another level), `--debug-overlay` (start with the [debug overlay](debug-overlay.md) shown; F3 toggles it), `--no-camera-collision` (the camera ignores the level's walls, for comparing screenshots), `--no-start-cutscenes` (development aid: the level's start does not wait for the intro movie and the first cutscene of a new game, so the player can be placed and tested at once; without it `--level` plays them first and then moves the player to the scripted start spot), `--sound-volume 0-10` and `--music-volume 0-10`. After a `--script` run the program prints `sounds:` with one `update:sound-number` pair per sound played. Use `SDL_AUDIODRIVER=dummy` for headless runs.
- The default level of `--gameplay` is the police station. The character is Chase McCain in his police outfit. That choice is a best guess. Run it from the repository folder so the default paths work.
- Move with the D-pad keys (WASD or arrows). Hold `Left Alt` to walk slowly. Without it you run, and after about one second of running you sprint. Press `Q` (the B button, or the right mouse button) to jump. `E` is the X button, which gets into and out of vehicles (see [Development runtime](game-runtime.md)). `Z` and `C` turn the camera (the left and right shoulder buttons). Pushing sideways also turns the camera, as in the original. `Escape` quits.
- What is like the original and what is still a stand-in: [Development runtime](game-runtime.md#what-works) and [what does not work yet](game-runtime.md#what-does-not-work-yet).
- A keyboard cannot tilt the stick halfway, so `Left Alt` stands in for a gentle push. It is a separate control from the 3DS buttons, set in `config/controls.cfg`.
- It prints one line per second: updates per second, the last time step and the circle pad values.
- Traffic lines of the level: `traffic: N cars, limit L, P spawn points, R routes, F floor cells, spawn data loaded` once at the start, `traffic car NAME: spawned at point P (X Y Z), fade F at update U` and `traffic car NAME: removed at update U` for each car that appears or leaves, `traffic car NAME: fade F at update U` when a car enters a fade box, and `traffic: limit L at update U` when a script calls `SetTrafficMaxVehicles`.
- `--screenshot` saves the picture after `--frames` updates (default 2) and exits.
- `--script "X Y COUNT [BUTTONS];X Y COUNT"` ignores the keyboard and holds the stick at X, Y for COUNT updates of 0.033 s each (values like the 3DS circle pad: +X right, +Y up, 145 is full). BUTTONS are optional 3DS button letters (A, B, X, Y, L, R, and S for Start) held during those updates. For example `"0 150 40"` pushes forward for 40 updates, and `"0 0 2;0 0 1 B;0 0 30"` stands, presses jump for one update and waits. A step `touch X Y COUNT` touches the lower screen at pixel X (0 to 319), Y (0 to 239) for COUNT updates, like a click with the mouse (for example `"touch 159 65 3"` clicks the exit button of the fast travel map). A step `at X Y Z [FACING]` (for example `"0 0 30;at 817.9 23.94 1071.9 0;0 0 60"`) puts the character at that world position before the next update, with the facing in degrees if given (a development teleport; it counts no update). While the character sits in a vehicle, the step moves the vehicle there instead (turned to FACING if given, else keeping its orientation from the level), and the character stays in it. The program prints `script teleport to X Y Z at update N` for each one, and one line `script NAME started at update N` or `ended at update N` for every level script and `cutscene NAME` / `movie NAME` lines when a cutscene or movie starts or ends. The program then prints how far the character moved, the screen brightness (`screen brightness 1` normal, 0 black), the stud counter (`studs: counter N, ...`) and the character's height after every update, then exits, saving the picture if `--screenshot` is given.
- `--start "X Y Z"` puts the character at that world position instead of the level's start point. `--facing DEGREES` sets the direction he faces (the camera follows). `--hide-room N` (1 to 99) never shows room N of the street, for comparing a picture with and without a room. Use it to test a ledge, for example `--level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --start "899.5 17.6393 1101" --script "0 150 40"` runs off a 1 m platform.
- `--particle "DEFINITION X Y Z"` (repeatable, 0 to 65535) starts a particle effect of `particles/particledefs.bprt` at that world position when the level starts, to look at the particle drawing. Nothing in the game starts effects yet; for example `--start "886.764 16.589 1111.166" --particle "22 886.764 17 1112"` shows question marks next to the character.

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
- `freetrap SECONDS ADDR[,ADDR...] NAME` catches rare calls without stepping: the game runs normally for that many seconds and stops only at the given code addresses. Each stop's registers and the memory they point to go to `NAME.txt`. Held buttons and the stick do not work during it, so do the walking first with stepped frames. The script stops with an error on a wrong number of words, a time that is not a positive number, an address that is not a word-aligned code address (0x100000 to 0xfffffff), or a name with characters other than letters, digits, `_`, `-` and `.`.

### Working faster

Save a state once, then start from it. States need the `--display xvfb` option, which runs the emulator on a private virtual screen.

```sh
python3 tools/azahar_oracle.py tools/oracle/to-gameplay.txt --display xvfb --renderer opengl --keep   # ends with: savestate gameplay-start
python3 tools/azahar_oracle.py --stop
python3 tools/azahar_oracle.py my-test.txt --display xvfb --renderer opengl --state gameplay-start
```

- `savestate NAME` and `loadstate NAME` are steps in the script file. States are stored in `artifacts/oracle/states/` and survive `--fresh`.
- Only `gameplay-start` (made by `tools/oracle/to-gameplay.txt`) and the states that other committed scripts save can be rebuilt by running a script. A script that loads any other state uses a role (`loadstate @ROLE`, below) for a state that you record yourself at the matching point of the game (play there and save with `savestate NAME` or the emulator's quick save).
- Each saved state remembers whether it was made in New 3DS mode. `loadstate` refuses a state from the other mode and asks you to start a new emulator with `--state NAME`.
- A committed script that needs a state you record yourself loads it by a role: `loadstate @ROLE` (and `--state @ROLE`). The role is capital letters, digits and `_`, starting with a letter. Give the state for each role with `--state-param ROLE=NAME` (repeat it for several roles; `NAME` is a state in `--states`). The roles of a script are in its `loadstate` lines; the game situation each role stands for is listed in [oracle-setup.md](re/oracle-setup.md#ts-20261025-141455).

```sh
python3 tools/azahar_oracle.py tools/oracle/camera-vehicle-enter.txt --display xvfb --renderer opengl \
  --state @PATROL_CAR --state-param PATROL_CAR=my-car-state
```

- The tool checks the roles before it starts the emulator, and stops with an error that names the option or the script line and the text when: a role has no `--state-param`, a role is written wrongly (`@patrol`), a `--state-param` is not `ROLE=NAME` or names a role twice, `NAME` has other characters than letters, digits, `_`, `-` and `.`, the state files `NAME.cst` and `NAME.json` are missing in `--states`, `loadstate` or `savestate` has not exactly one word after it, or `savestate` is given a role (it takes plain names).
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

The script steps `touch` and `drag` touch the 3DS bottom screen (the touch screen of the game's menus). The point goes through the same virtual motion source as a touch pad, so it needs no `--rpc`, no mouse events and no change to the game's memory. The game reads it from its normal touch input.

- `touch X Y [UPDATES]` taps the bottom-screen pixel X Y: X is 0 to 319 and Y is 0 to 239 (the 320 by 240 screen; the fast travel exit button is at 159 65). The point is held for UPDATES game updates (1 to 600, default 6; the same updates that `frames` counts) and then released by the step itself. The game then sees a press, the point, and a release at the first update after the step.
- `touch release` ends a touch at once.
- `drag X1 Y1 X2 Y2 [UPDATES]` touches X1 Y1, moves in a straight line to X2 Y2 with one position per update (UPDATES 2 to 600, default 10) and releases.
- The step stops the script with an error that names the line, the step and the bad text for a wrong word count, a number that is not a whole number, a negative number, a point outside the screen, or a count outside its range. It also stops when the emulator was started before this feature existed: stop it with `--stop` and start it again.
- The touch is a pixel on the bottom screen as the game reads it; menu hit areas are the game's own. Only one finger is supported.
- Details and the measured mapping: `docs/re/oracle-setup.md`.
