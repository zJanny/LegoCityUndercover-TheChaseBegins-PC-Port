# Model viewer

`chase-viewer` shows models, levels and animated characters from your extracted game data. It is a tool for checking decoded assets. **It is not the game.** There is no gameplay, and the game's lighting and materials are not recovered, so many surfaces look darker than in the game.

The viewer needs the extracted RomFS in `artifacts/romfs` (see [Extracting game data](extracting-game-data.md)). Paths such as `models/char_bear/char_bear.bfnmdl` are names inside the game's asset archive.

## Show a model

```sh
build/chase-viewer models/lvl_00_policestation/room1.bfnmdl --exclude sky
```

You can list several models. They are drawn together.

## Show a level

```sh
build/chase-viewer --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --exclude sky
```

- `--level` places every object from the level file that names a model.
- Without model names, the viewer also loads the level's own models from `models/<level name>/`. This follows the archive's folder names; how the game itself streams these pieces is not known yet.
- `--room room3` shows a single room and only its objects.
- `--nav` draws the level's navigation grid on top: cyan cells are open floor, orange cells are edges and obstacles.
- `--eye X,Y,Z` puts the camera at a position, for example `--eye 800,60,1060 --yaw 180 --pitch -25` for a street view in Cherry Tree Hills.

## Animate a character

```sh
build/chase-viewer models/char_bear/char_bear.bfnmdl \
    --skeleton models/char_bear/char_bear.bfnskl --anim models/char_bear/walk.bfnanm
```

- The animation plays in a loop on the first model you list. Like the game, the viewer blends smoothly between frames.
- `--frame N` shows a single frame instead. Values such as `2.5` show an in-between pose.
- The game's real playback speed is not known yet. The viewer plays 30 frames per second; change it with `--fps`.

## Play a cutscene

```sh
build/chase-viewer --cutscene csdata/models/1_1_a \
    --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --exclude sky
```

- `--cutscene` loads the cast listed in the cutscene folder and follows its camera.
- Add the level the cutscene takes place in with `--level`. Otherwise the characters stand in empty space.
- The cutscene loops. `--frame N` shows a single frame, and `--fps` changes the speed as for animations.
- While a cutscene plays, the camera controls do nothing.
- Some characters have shorter animations than the camera. The viewer starts them all together and holds their last pose. How the game lines them up is not known yet.
- There is no sound or subtitles yet.

## Controls

The viewer uses the same `config/controls.cfg` as the [input laboratory](controls.md):

- **D-pad:** move.
- **L / R:** lower / raise.
- **X / Y:** turn.
- **Left-drag:** look around.
- **Escape:** quit.

## Options

| Option | What it does |
|---|---|
| `--room NAME` | Shows only the level objects in one room. |
| `--nav` | Draws the navigation grid of the level. |
| `--cutscene FOLDER` | Plays the cutscene in `csdata/models/<id>`. |
| `--exclude WORD` | Hides parts whose texture name contains WORD, for example `sky`. |
| `--brightness F` | Makes everything brighter, to help you see. |
| `--cull ccw\|cw\|none` | Chooses which faces are hidden. The default, `ccw`, hides ceilings so you can look into rooms. |
| `--yaw DEG`, `--pitch DEG` | Sets the starting camera angle. |
| `--eye X,Y,Z` | Sets the starting camera position. |
| `--screenshot OUT.bmp` | Draws one frame, saves it and exits. |
| `--fib`, `--names` | Use a different asset archive. The default is in `artifacts/romfs/romfs/`. |

## Without a display

Screenshots also work on a machine without a screen:

```sh
SDL_VIDEODRIVER=offscreen build/chase-viewer --screenshot out.bmp models/char_bear/char_bear.bfnmdl
```
