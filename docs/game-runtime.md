# Development runtime

The development runtime (`chase-game`) is the first part of the future game, and it is an early test program. **It is not the game yet.** It starts with the screens of the original game, from the logos to a new game, and then lets you walk the main character around a level, with behaviour measured in the original. There are no missions, enemies, menus or story yet.

You need the game files first. See [Extracting game data](extracting-game-data.md) and [Building the tools](building.md). The program needs libepoxy and FFmpeg.

## Start it

Run this from the project folder, the one that contains `README.md`:

```sh
build/chase-game
```

A window opens and shows the start of the game, as it looks on the 3DS: the top screen above the bottom screen. Press Escape to quit. With `--window WIDTHxHEIGHT` you choose the window size, for example `--window 800x960`.

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
- Every slot is an empty "New Game" slot. Saved games are not shown in the slot list yet. To start from your own save, run `chase-game --save savegame.dat --slot 0` (see [commands](commands.md)): it starts the level of that slot like Continue in the original.
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
- F3 shows or hides the [debug overlay](debug-overlay.md) (frame rate, memory, CPU and a few level values). It is off by default.

## What works

- The speeds, the walk, run and sprint change, and the turning follow the original game.
- The camera follows behind the character, as in the original.
- In some places the camera is fixed, as in the original: the police station and every area the level marks with a fixed camera. There it does not turn when you press the camera buttons, and a crossed-out camera picture shows at the bottom left of the top screen. Details such as the smooth change between fixed cameras are not done yet.
- The camera keeps out of the level's walls. Near a wall it moves closer to the character and slides to the side, as measured in the original.
- The floor and walls come from the level. The character climbs steps and kerbs, and slides along walls.
- Many objects of the level block the character and the camera like the original: lamp posts, traffic lights, benches, bins, crates, barriers, doors and signs. The character is stopped by them and slides along them.
- The level's other people stand where the level puts them, with their own heads and clothes. They do not move, talk or react.
- The character jumps, falls off ledges and lands as in the original, including the short pause after landing. Running uphill slows the character down. At some ledges the character first slides down a steep edge before it falls. Two details of that slide are not exact yet: how it ends and the short pause after landing from it.
- The character makes the sounds of the original: footsteps while walking, running and sprinting, a sound when jumping, and landing sounds that depend on how far the character fell. Picking up a stud plays its coin sound. Sounds come from the place where they happen and are quieter and panned to the side as in the original. See [Sound](#sound).
- In the level, the top screen shows the 3D view with the display of the original game on top of it, and the bottom screen shows the map. See [The display during play](#the-display-during-play).
- Doors that open by themselves swing open when the character comes close, wait while he stands there, and close again, with the same timing as the original. This works for the hinged doors and the sliding doors of the police station. Doors that the story opens (for example the front doors in the street) stay shut, because the story is not in the program yet. Doors make their opening and closing sounds.
- The street level loads and unloads its rooms while you walk, by the rule found in the original: the room you stand in, its neighbours and up to three rooms ahead of the camera. Rooms that are not loaded are not drawn. The room you stand in is found from the room models' size, a stand-in. The loading time (9 updates) is the one measured for one room. Characters and coins of unloaded rooms are still drawn. Other levels show all their rooms. The program prints `room streaming:`, `room A -> B` and `rooms loaded:` lines.
- The level's own scripts run while you play, with the same timing rules as the original (a script that waits continues later, a script can wait for another one). Scripts start when the level starts, when the character walks into or out of some marked places of the level, and when a script switches an object. Events of the story and many other kinds of starts do not start scripts yet; the program prints `script trigger event without check` for those once. A script can open and close doors.
- The question box of the police station door works: walk into the door area (after the first mission is switched on) and the game stops with the box "If you are on a mission...". A (OK) moves the character to the station door, B (Cancel) closes it. The box is drawn from measurements of one picture, so sizes may differ slightly. Screen fades from scripts darken the picture over half a second.
- You can walk out of the police station and back in. Leaving: walk to the lobby exit. A page with "Continue (A)" appears. Press A and you stand in the street where the original puts you. Entering: walk into the door area (after the first mission is switched on), answer the box with A, and press A again on the page. You stand in the lobby. The page is a stand-in: the picture on the top screen is a plain background, not the original's key art. The camera after the change is not the original's yet. The street level needs `--call-script 1-1_MissionSkip` for the door, unless you start with `--save` from a save that is past the first mission.
- Scripts can walk the player: after you answer the police station box with A, the character walks into the station by itself, at a speed measured in the original, in a straight line (a stand-in: the original follows a path). Other people do not walk yet. The AI commands of the scripts (enable, ignore player, invulnerable, pursuit and so on) are accepted and remembered but have no visible effect.
- Waypoints (the light columns of the missions) work on foot: walk into an active one and the player stops for about half a second, then the mission script it names starts (for example the donut shop of mission 1-1 starts `1-1_VanBreakdown`) and the waypoint switches off. The dev log prints `waypoint started script NAME at update N`. Waypoints for vehicles and the use-an-object waypoints do not work yet.
- Most script commands are not worked out yet. The first time a script uses one of them, the program prints `script function without handler` and the command's name, and the script goes on as if the command had finished.
- The coins (studs) of the level spin where the level puts them. Walk close to one and it is picked up. It flies up to the stud counter. After one second the counter goes up (silver 10, gold 100, blue 1000, purple 10000), shows the new total for a few seconds and slides away again. The program also prints the counter at the end of a `--script` run.

## The display during play

- Top left of the top screen: the picture of the main character in its hexagon, and next to it four hearts. The heart at the end of the row beats slowly.
- The stud counter slides in below the hearts each time a stud is added. It shows the new total and a spinning coin in the colour of the last stud. The coin jumps for a moment when the total changes. After about two seconds the counter slides out again.
- The bottom screen shows the map of the level, turned so that the camera looks up. The arrow in the middle is the main character. The button at the top left is the characters button; it does nothing yet.
- Not like the original yet: the hearts do not change because nothing can hurt the character yet. The map is not placed with the original's own data, but fitted to pictures of the original (it differs by a few pixels), and it shows no mission marks or collectibles. The display slides in a moment after the level starts; when the original does that is not known. Only the first level has a map picture that was checked.

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
- Studs are stand-ins in some details. Studs that a mission or a script switches on stay hidden. The spin starts at its own time and the pictures of the studs, also the flying ones, are drawn without the original's blending. Studs that a script adds to the counter later (the original shows the old total first) are not done.
- The pickup distance (1.5 units) was measured in the original, not taken from the game's code.

## Other levels

Choose another level with `--level` and the level's path:

```sh
build/chase-game --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl
```

The path is the level's place inside the game archive, as in the example. Each level has a `<name>_main.blvl` file.

For the other options, such as `--screenshot` and `--frames`, see the [command reference](commands.md#development-runtime). Giving `--level`, `--script` or `--start` also skips the front-end screens.
