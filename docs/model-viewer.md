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
build/chase-viewer --level levels/levels/lvl_00_policestation/lvl_00_policestation_main.blvl \
    --room room3 --brightness 2.5 models/lvl_00_policestation/room3.bfnmdl
```

`--level` places every object from the level file that names a model.

## Animate a character

```sh
build/chase-viewer models/char_bear/char_bear.bfnmdl \
    --skeleton models/char_bear/char_bear.bfnskl --anim models/char_bear/walk.bfnanm
```

- The animation plays in a loop on the first model you list.
- `--frame N` shows a single frame instead.
- The real playback speed is not known yet. The viewer plays 30 frames per second.

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
| `--exclude WORD` | Hides parts whose texture name contains WORD, for example `sky`. |
| `--brightness F` | Makes everything brighter, to help you see. |
| `--cull ccw\|cw\|none` | Chooses which faces are hidden. The default, `ccw`, hides ceilings so you can look into rooms. |
| `--yaw DEG`, `--pitch DEG` | Sets the starting camera angle. |
| `--screenshot OUT.bmp` | Draws one frame, saves it and exits. |
| `--fib`, `--names` | Use a different asset archive. The default is in `artifacts/romfs/romfs/`. |

## Without a display

Screenshots also work on a machine without a screen:

```sh
SDL_VIDEODRIVER=offscreen build/chase-viewer --screenshot out.bmp models/char_bear/char_bear.bfnmdl
```
