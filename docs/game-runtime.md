# Development runtime

The development runtime (`chase-game`) is the first part of the future game, and it is an early test program. **It is not the game yet.** It starts with the screens of the original game, from the logos to a new game, and then lets you walk the main character around a level, with behaviour measured in the original. There are no missions, enemies, menus or story yet.

You need the game files first. See [Extracting game data](extracting-game-data.md) and [Building the tools](building.md). The program needs libepoxy and FFmpeg.

## Start it

Run this from the project folder, the one that contains `README.md`:

```sh
build/chase-game
```

A window opens and shows the start of the game, as it looks on the 3DS: the top screen above the bottom screen. Press Escape to quit.

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

The buttons are the keys of [Controls](controls.md): A, B, and the up and down keys. `--language 1-9` starts with another language (the list numbers it in the order of the language screen: 1 English, 2 French, 3 Italian, 4 German, 5 Spanish, 6 Dutch, 7 Danish, 8 Portuguese, 9 Russian). `--returning` starts as a player who has played before: the language screen is skipped and the movies can be skipped.

What is not like the original yet:

- Several parts are stand-ins: how long the loading pages last, the spinning badge on the loading screen (a flat picture here), the picture behind the title card, the tip on the loading screen (always the same one), and the 3D scene that the original plays on the top screen under the title card (here the top screen stays black).
- Every slot is an empty "New Game" slot. Saved games are not shown or loaded.
- The StreetPass pages act as if StreetPass were available, as it was in the comparison runs.
- The level after "New Game" is the street of Cherry Tree Hills, with the character at the position the original starts him. Nothing from the story happens there.

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
- Many objects of the level block the character and the camera like the original: lamp posts, traffic lights, benches, bins, crates, barriers, doors and signs. The character is stopped by them and slides along them.
- The level's other people stand where the level puts them, with their own heads and clothes. They do not move, talk or react.
- The character jumps, falls off ledges and lands as in the original, including the short pause after landing. Running uphill slows the character down. At some ledges the character first slides down a steep edge before it falls. Two details of that slide are not exact yet: how it ends and the short pause after landing from it.
- The character makes the sounds of the original: footsteps while walking, running and sprinting, a sound when jumping, and landing sounds that depend on how far the character fell. Picking up a stud plays its coin sound. Sounds come from the place where they happen and are quieter and panned to the side as in the original. See [Sound](#sound).
- Doors that open by themselves swing open when the character comes close, wait while he stands there, and close again, with the same timing as the original. This works for the hinged doors and the sliding doors of the police station. Doors that the story opens (for example the front doors in the street) stay shut, because the story is not in the program yet.
- The coins (studs) of the level spin where the level puts them. Walk close to one and it is picked up. After one second the stud counter goes up (silver 10, gold 100, blue 1000, purple 10000). The program prints the counter at the end of a `--script` run.

## Sound

- The program plays the sounds of the character and the studs through your normal audio output. If no audio device can be opened, it prints a note and runs without sound.
- A sound is louder when it is close and silent when it is far away. It is panned left or right depending on where it is as seen from the camera.
- The volume options are `--sound-volume 0-10` (default 10) and `--music-volume 0-10` (default 8). They are the game's option values, which the game divides by 10.
- There is no music at the start of the police station or the street. The original starts none there either. Music that the original starts later (during missions, fights or in vehicles) is not played, because those parts are not worked out yet.
- Not played yet: the sounds of other people, animals, vehicles and objects of the level, the idle sounds of the character, and water or sand landing sounds.

## What does not work yet

- Only some kinds of objects block the character and the camera so far. Parked cars, people, animals and some special objects do not. Objects that the original switches off at the start (mission items, build-it parts and their results) are switched off by the same rules as in the original, but nothing switches them on later.
- Objects stay as they are at the start of the level. Nothing breaks or moves, and doors open only when the character comes close. An open door keeps the same blocking box as a closed one here. One measurement in the original differed, and the reason is not known yet.
- When the camera is stuck behind a wall for a longer time, the original places it again. That is not done here.
- Other characters and vehicles do not block the character.
- Animals and the eight line-up characters of the police station are not shown. Their look is not worked out yet.
- Walking slowly off a ledge looks different from the original, which first plays a short step.
- Jumping onto something higher, or under a ceiling, may not match the original.
- The animations do not blend. Their speed is an estimate.
- Studs are stand-ins in some details. A picked-up stud disappears at once instead of flying to the counter. The counter is not shown on the screen, only printed. Studs that a mission or a script switches on stay hidden. The spin starts at its own time and the picture is drawn without the original's blending.
- The pickup distance (1.5 units) was measured in the original, not taken from the game's code.

## Other levels

Choose another level with `--level` and the level's path:

```sh
build/chase-game --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl
```

The path is the level's place inside the game archive, as in the example. Each level has a `<name>_main.blvl` file.

For the other options, such as `--screenshot` and `--frames`, see the [command reference](commands.md#development-runtime). Giving `--level`, `--script` or `--start` also skips the front-end screens.
