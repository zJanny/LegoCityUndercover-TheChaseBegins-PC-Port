# Development runtime

The development runtime (`chase-game`) is the first part of the future game, and it is an early test program. **It is not the game yet.** It shows a level and lets you walk the main character around it, with behaviour measured in the original. There are no missions, enemies, menus or story yet.

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
- The coins (studs) of the level spin where the level puts them. Walk close to one and it is picked up. After one second the stud counter goes up (silver 10, gold 100, blue 1000, purple 10000). The program prints the counter at the end of a `--script` run.

## What does not work yet

- The camera only reacts to the level's fixed walls. Lamp posts, cars and other objects do not push it.
- When the camera is stuck behind a wall for a longer time, the original places it again. That is not done here.
- Objects, other characters and vehicles do not block the character.
- Animals and the eight line-up characters of the police station are not shown. Their look is not worked out yet.
- Walking slowly off a ledge looks different from the original, which first plays a short step.
- Jumping onto something higher, or under a ceiling, may not match the original.
- The animations do not blend. Their speed is an estimate.
- Studs are stand-ins in some details. A picked-up stud disappears at once instead of flying to the counter. The counter is not shown on the screen, only printed. There is no pickup sound. Studs that a mission or a script switches on stay hidden. The spin starts at its own time and the picture is drawn without the original's blending.
- The pickup distance (1.5 units) was measured in the original, not taken from the game's code.

## Other levels

Choose another level with `--level` and the level's path:

```sh
build/chase-game --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl
```

The path is the level's place inside the game archive, as in the example. Each level has a `<name>_main.blvl` file.

For the other options, such as `--screenshot` and `--frames`, see the [command reference](commands.md#development-runtime).
