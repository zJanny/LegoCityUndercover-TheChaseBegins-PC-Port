# Development runtime

The development runtime (`chase-game`) is an early test program. **It is not the game.** It shows a level and lets you walk the main character around it. There are no missions, enemies, menus or story.

You need the game files first. See [Extracting game data](extracting-game-data.md) and [Building the tools](building.md). The program needs libepoxy.

## Start it

Run this from the project folder, the one that contains `README.md`:

```sh
build/chase-game
```

A window opens with the police station. Press Escape to quit.

## Keys

| Action | Keys |
|---|---|
| Walk, run and sprint | Arrow keys, or W, A, S and D |
| Walk slowly | Hold Left Alt |
| Jump | Q, or the right mouse button |
| Turn the camera | Z (left) and C (right). Pushing left or right also turns the camera. |
| Quit | Escape |

- The character runs when you push the keys. After about one second of running, the character sprints.
- The keys come from `config/controls.cfg`. See [Controls](controls.md).

## What works

- The speeds, the walk, run and sprint change, and the turning follow the original game.
- The camera follows behind the character, as in the original.
- The camera keeps out of the level's walls. Near a wall it moves closer to the character and slides to the side, as measured in the original.
- The floor and walls come from the level. The character climbs steps and kerbs, and slides along walls.
- The level's other people stand where the level puts them, with their own heads and clothes. They do not move, talk or react.
- The character jumps, falls off ledges and lands as in the original, including the short pause after landing.

## What does not work yet

- The camera only reacts to the level's fixed walls. Lamp posts, cars and other objects do not push it.
- When the camera is stuck behind a wall for a longer time, the original places it again. That is not done here.
- Objects, other characters and vehicles do not block the character.
- Animals and the eight line-up characters of the police station are not shown. Their look is not worked out yet.
- Walking slowly off a ledge looks different from the original, which first plays a short step.
- Jumping onto something higher, or under a ceiling, may not match the original.
- The animations do not blend. Their speed is an estimate.

## Other levels

Choose another level with `--level` and the level's path:

```sh
build/chase-game --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl
```

The path is the level's place inside the game archive, as in the example. Each level has a `<name>_main.blvl` file.

For the other options, such as `--screenshot` and `--frames`, see the [command reference](commands.md#development-runtime).
