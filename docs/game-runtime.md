# Development runtime

The development runtime (`chase-game`) is the first part of the future game, and it is an early test program. **It is not the game yet.** It starts with the screens of the original game, from the logos to a new game. Then it lets you walk the main character around a level, with behaviour measured in the original. Only parts of the first three missions (1-1 to 1-3) run. There are no enemies, and of the menus only the pause menu wheel works.

Back to the [documentation list](../README.md#more-documentation).

You need the game files first. See [Extracting game data](extracting-game-data.md) and [Building the tools](building.md). The program needs libepoxy and FFmpeg.

## Start it

Run this from the project folder, the one that contains `README.md`:

```sh
build/chase-game
```

A window opens and shows the start of the game, as it looks on the 3DS: the top screen above the bottom screen. To quit, close the window or use Quit game in the [PC settings menu](controls.md#pc-settings-menu); Escape never closes the game ([Escape](controls.md#escape)). With `--window WIDTHxHEIGHT` you choose the window size, for example `--window 800x960`.

To skip the screens and go straight to a level, use `--gameplay` (police station) or `--level` and the level's path (see [Other levels](#other-levels)).

## The screens before the level

The program shows these screens in the original order:

1. A short loading page ("Preparing Game Card", "Preparing save data").
2. The company logos, then the TT Games movie.
3. The language list. Push up or down, then press A.
4. The StreetPass question. A is Yes, B is No. Then the HOME Button notice (press A).
5. The autosave notice (press A).
6. The slot list. Push up or down, then press A on a slot.
7. "Saving Game", a loading screen with a tip, and the intro movie with its subtitles.
8. The "Doughnut Disaster" title card. Press A, or wait. Then the level starts.

The buttons are the keys of [Controls](controls.md): A, B, and the up and down keys. `--language 1-9` starts with another language (the game's language numbers: 1 English, 2 French, 3 Italian, 4 German, 5 Spanish, 6 Dutch, 7 Danish, 8 Portuguese, 9 Russian; the language screen lists Danish before Dutch and Russian before Portuguese). `--returning` starts as a player who has played before: the language screen is skipped and a button press skips the TT Games movie (the intro movie cannot be skipped).

What is not like the original yet:

- Several parts are stand-ins:
  - how long the loading pages last
  - the spinning badge on the loading screen (a flat picture here)
  - the picture behind the title card
  - the tip on the loading screen (always the same one)
  - the 3D scene that the original plays on the top screen under the title card (here the top screen stays black)
- Without `--front-save`, every slot is an empty "New Game" slot. With `--front-save savegame.dat` the slot list shows the slots of your save. A on a used slot continues in the level that slot was last in (see [commands](commands.md)). The delete page is not done. To skip the screens and start from a save slot directly, run `chase-game --save savegame.dat --slot 0`.
- The game saves when the original saves (save points of the story, a collected super brick). It writes into `~/.local/share/chase/savegame.dat` (`$XDG_DATA_HOME/chase/savegame.dat` when that is set), or into the file of `--save-out`. The slot chosen on the slot list is the one written. Start again from it with `chase-game --save ~/.local/share/chase/savegame.dat --slot 0` or show it in the slot list with `--front-save`. The save holds the progress, the play time, the studs and the super bricks; what was built or collected inside a level is not saved.
- The StreetPass pages act as if StreetPass were available, as it was in the comparison runs.
- The level after "New Game" is the street of Cherry Tree Hills, with the character at the position the original starts him. Mission 1-1 starts there: the cutscene plays, then the first light column (at the donut shop) is switched on.

## Keys

| Action | Keys |
|---|---|
| Walk, run and sprint | Arrow keys, or W, A, S and D |
| Walk slowly | Hold Left Alt |
| Jump | Q, or the right mouse button |
| Punch (the Y button) | R, a short press |
| Turn the camera | Z (left) and C (right). Pushing left or right also turns the camera. |
| Pause menu | Return, or Keypad Enter with PC controls on; Escape in both |
| Quit | Quit game in the PC settings menu, or close the window |

- The character runs when you push the keys. After about one second of running, the character sprints.
- The keys come from `config/controls.cfg`. See [Controls](controls.md).
- F3 shows or hides the [debug overlay](debug-overlay.md) (frame rate, memory, CPU and a few level values). It is off by default.
- In a level the game updates 30 times per second, every 1/30 s, like the original in the driven police car. Other screens still follow the display rate.

## What works

The list of what `chase-game` reproduces is on its own page: [what works](game-runtime-works.md).

## Window mode and size

- `chase-game` starts in borderless full screen at the desktop size. With `--window WIDTHxHEIGHT` it starts in a window of that size instead, whatever the stored mode.
- **F11** switches between borderless full screen and a window. The choice is stored as `window_mode` (`windowed` or `fullscreen`) in `config/qol.cfg`, next to the controls file (`--config`). The file need not exist.
- `view_distance <factor>` in `config/qol.cfg` is the View distance of the [PC settings menu](controls.md#pc-settings-menu).
  - The value is one of:
    - `1.25`, `1.5`, `1.75` and so on in quarter steps up to `4.0`
    - `5.0`, `6.0` and so on in whole steps up to `20.0`
    - `1.0` is also accepted
  - Without the line the factor is x1.0, the original.
  - With `master Improved` or `master Custom` these distances are multiplied by the factor:
    - the far plane of every camera
    - the fog start and end
    - the pedestrians' spawn and fade distances
  - Above x1.0 the game loads more rooms. It loads every room whose box comes within 260 m times the factor of the camera. These come besides the rooms the game loads itself. They finish loading 9 updates after they come in range, also at the level start.
  - `master Original` always uses x1.0.
  - Traffic is not scaled (open item).
  - Any other value stops the program with an error naming the line.
- `resolution WIDTHxHEIGHT` in `config/qol.cfg` sets the size of the window (not used in full screen). Without the line the window has the desktop size. A size larger than the largest display mode of the monitor is limited to it; another size that is not a display mode is used as written. A malformed line stops the program with an error.
- The window cannot be made smaller than 400x240. The window has this minimum from its creation. A `--window` size below it stops the program with an error naming the option (the option parses widths from 200).
- The [PC settings menu](controls.md#pc-settings-menu) writes `config/qol.cfg` after each change of its Display and Quality of life tabs. The window follows at once (window mode and size). After a display change it asks "Keep these settings?"; without an answer within 15 seconds, or on Escape, the old window mode and size come back and are written again. A menu or file error stops the program with a message. A resolution chosen in the menu also resizes a window started with `--window`.
- `config/qol.cfg` is read at startup. The widened view works like this:
  - With `master Improved` (or `widened_view on` under `master Custom`) the 3D view is widened over the whole window. The bottom screen is hidden with it.
  - Only the gameplay camera is widened. Cutscene, director and scanner views and movies keep the 5:3 picture with bars at the sides.
  - The HUD parts sit at the corners of the widened view. While a cutscene, director or scanner camera shows the 5:3 picture, they sit at the corners of that picture.
    - Hearts and portrait are top left, studs and super bricks top right, camera lock bottom left.
    - The head icon and the scanner overlay stay on the usual top-screen rectangle.
  - The widened view also loads the rooms that only its wider sides reach. These lie inside the 16:9 view, outside the 5:3 view, up to the far plane.
  - While the view is widened the bottom screen is not drawn and a click there is no touch. The top picture is fitted 5:3 and centred in the window, with bars where needed.
  - While the view is widened a round minimap sits at the right edge of the window. It is under the studs and counters of the corner HUD, with a small margin.
    - It is the original map canvas cropped to a disc, at the original zoom, rotating with the camera.
    - Its diameter is one fifth of the window height.
    - It is not drawn in these cases:
      - in cutscenes and movies
      - while the scanner overlay shows
      - in a dialog box or speech box
      - on the scene change page
      - while the fast-travel map is open
    - The pause menu replaces the whole picture.
  - The pages of the bottom screen (pause menu, map, scanner, build-it, award viewer) have no input then.
  - In the front end while the view is widened the top picture fills the window. Each page with bottom-screen content is drawn as a 320x240 panel centred over the darkened picture, as large as fits keeping 4:3. These pages are the "Preparing" notice, language list, question and notice pages, autosave notice, slot page, "Saving Game", loading screen and mission card.
  - The panel appears and disappears with its page. The logos, the movies and the black moments between pages show no panel and an undimmed picture.
  - The loading screen's panel has a small spinning save icon at the lower right of the tip box. The "PC Settings" bar of the slot page is clicked inside the panel.
  - While a movie plays (the logo movie and the intro movie, in the front end and in a level) there is no panel. The movie fills the window at full brightness. A subtitle is the original subtitle bubble drawn over the lower part of the movie.
  - A left click on a row of the language list or the slot list selects that row and presses A. A click on an A, B or Y picture of the page (the question and notice pages, the shortcut bar) presses that button. A click anywhere else on such a page does nothing.
  - Pages without rows or buttons (logos, movies, loading, mission card) do not handle the click in the panel. It passes on as the bound mouse button (A by default). So it skips the fusion movie (not on a first run) and the mission card like the A key.
  - `master Custom` with `widened_view off` keeps the bottom screen. The widened view is the one setting that hides it, in every preset (switching PC controls on or off never changes it). A missing file keeps the original view.
  - `pc_controls on` under `master Custom` (and `master Improved`) makes the running game read the keys from `config/controls.pc.cfg`, see [PC controls](controls.md#pc-controls). The file is created from the built-in set when it is missing.
  - With PC controls on, mouse X also turns the follow camera in play (see [PC controls](controls.md#pc-controls)). The cursor is captured while the game runs. It is released in the pause screen, in the PC settings menu and over the lower screen.
  - While the player sits in a vehicle (and no pause screen or award viewer is open) the keys come from `config/controls.pc.vehicle.cfg` instead. The runtime prints `PC vehicle controls on` and `PC vehicle controls off` at the switch.

## The display during play

- Top left of the top screen: the picture of the main character in its hexagon, and next to it four hearts. The heart at the end of the row beats slowly.
- The stud counter slides in below the hearts each time a stud is added. It shows the new total and a spinning coin in the colour of the last stud. The coin jumps for a moment when the total changes. After about two seconds the counter slides out again.
- The bottom screen shows the map of the level. In the levels the map is centred on the main character and turned so that the camera looks up; the arrow in the middle is the main character. In the police station the map keeps one fixed centre and angle, as in the original. It does not move or turn while you walk or turn the camera. The map buttons slide in as in the original: the characters button at the top left and, from mission 1-3 on, the scan button at the bottom left. A short click on the characters button does nothing. Holding it for more than half a second opens the characters panel without its portraits. The other buttons do nothing.
- Not like the original: the hearts do not change because nothing can hurt the character yet. The map of each level is placed with the numbers of that level's map file, as in the original. The picture of the first level equals the original's. The other levels were not compared. In the police station the map of the first level is shown around the original's fixed centre and angle; the arrow of the character there is still turned as in the other levels, which may differ from the original. The map shows the marker of the active waypoint. When the waypoint is far away, the marker stays at the edge of the map and an arrow points to it. Mission targets (for example the police car of mission 1-1 once you reach its waypoint) show a small blue dot on the map. When none of a mission's targets is near enough to be on the map, a blue dot at their middle fades in over 1 second. It stays at the edge with a blue arrow. The target dots fade out. The blue dot fades away again when a target comes near. Enemy targets would use red and carry targets green, but nothing in the port adds those. Other mission marks and collectibles are not shown, and the map does not zoom. The display slides in a moment after the level starts; when the original does that is not known.

## Sound

- The program plays the sounds of the character and the studs through your normal audio output. If no audio device can be opened, it prints a note and runs without sound.
- A sound is louder when it is close and silent when it is far away. It is panned left or right depending on where it is as seen from the camera.
- The volume options are `--sound-volume 0-10` (default 10) and `--music-volume 0-10` (default 8). They are the game's option values, which the game divides by 10. Like the original, each level load then uses the volumes the save file holds for the slot that was saved last. So a run with `--save`, or with a save file that already has a saved slot, uses the saved volumes instead. A new game keeps the two options as the volumes of its slot.
- There is no music at the start of the police station or the street. The original starts none there either. Music that mission scripts start (the story script commands that choose a music track, or stop it) is played. Music that the original starts in fights or in vehicles is not played, because those parts are not worked out yet.
- Not played yet: the sounds of other people, animals, vehicles and objects of the level, the idle sounds of the character, and water or sand landing sounds.

## What does not work yet

The list of what is missing or only approximated is on its own page: [what does not work yet](game-runtime-missing.md).

## Other levels

Choose another level with `--level` and the level's path:

```sh
build/chase-game --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl
```

The path is the level's place inside the game archive, as in the example. Each level has a `<name>_main.blvl` file.

A new game in Cherry Tree Hills starts as in the original:

- The intro movie plays (once), then the first cutscene of mission 1-1 with the donut shop, then the first tip.
- Start (Return, or Keypad Enter with PC controls on; Escape in both) opens the pause menu of the original. The upper screen shows the collectibles panel (without its numbers). The lower screen shows the wheel with Resume, Options, Credits, Exit and PC Settings, Back and Select. The wheel turns and chooses as in the original, also by clicking or swiping on the lower screen ([controls](controls.md#pause-menu)). Resume, B and Start go back to the game; Escape is B on every page of the pause screen. Options opens the options wheel of the original: the sound effects and music volume, Invert camera (it turns the camera the other way with L and R) and Language. A change of a volume is heard at once; B after a change saves the options into the save file (without the original's "Checking save data" check) and goes back to the pause wheel. The Language row opens the original's grid of nine flag tiles with no flag chosen. A click on a flag chooses it. A second click or A stores that language in the save file and goes back to the options wheel. B goes back without a change. The texts and pictures stay English. PC Settings opens the [PC settings menu](controls.md#pc-settings-menu) over the whole window. The game and the pause screen wait behind it (its Game tab holds only Quit game here). The pause wheel is as it was after Escape. Credits and Exit do nothing. The short black screen of the original before the menu appears is not there.
- In the police station, A on the award viewer's use marker opens the awards screen of the original ([controls](controls.md#award-viewer)): the lower screen has the wheel with Overview and the seven areas and the Back button. The upper screen counts the bronze, silver and gold awards of the save slot for the chosen area. The award trophies of the upper screen are not drawn. The screen opens and closes without the original's fades. The program prints `award viewer opened at update N, bronze A/B, silver C/D, gold E/F` and `award viewer closed at update N`.
- Cutscenes fade the top screen to black before they start (when the script asks for it) and at their end. Then they fade the game picture back in. The fades come from the original's code: 0.5 s each, black quad over the top screen only. The short black pause before the picture returns is a stand-in measured in the original.
- The player is held during the cutscene. Add `--no-start-cutscenes` to skip the movie and the first cutscene while testing.

For the other options, such as `--screenshot` and `--frames`, see the [command reference](commands.md#development-runtime). Giving `--gameplay`, `--level`, `--script`, `--start`, `--particle` or `--save` also skips the front-end screens.

In-engine cutscenes that the level scripts start (for example the donut shop scene of mission 1-1) play in the level: the picture is the cutscene camera, the HUD is hidden and the player waits until it ends. A cutscene fades in from black at its start. It does so also when a script blacked out the screen before it (as after the dog is released in mission 1-2). The cutscene camera follows the original's code: its moves between keys, its hard cuts and its field of view (from the camera's lens). Movies that a script starts play on the top screen. The faces of the actors and skipping are not done.
