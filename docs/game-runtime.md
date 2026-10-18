# Development runtime

The development runtime (`chase-game`) is the first part of the future game, and it is an early test program. **It is not the game yet.** It starts with the screens of the original game, from the logos to a new game, and then lets you walk the main character around a level, with behaviour measured in the original. Only parts of mission 1-1 run. There are no other missions, no enemies and no menus yet.

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
- Without `--front-save`, every slot is an empty "New Game" slot. With `--front-save savegame.dat` the slot list shows the slots of your save, and A on a used slot continues in the level that slot was last in (see [commands](commands.md)). The delete page is not done. To skip the screens and start from a save slot directly, run `chase-game --save savegame.dat --slot 0`.
- The StreetPass pages act as if StreetPass were available, as it was in the comparison runs.
- The level after "New Game" is the street of Cherry Tree Hills, with the character at the position the original starts him. Mission 1-1 starts there: the cutscene plays, then the first light column (at the donut shop) is switched on.

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
- In a level the game updates 30 times per second, every 1/30 s, like the original in the driven police car. Other screens still follow the display rate.

## What works

- The speeds, the walk, run and sprint change, and the turning follow the original game.
- The camera follows behind the character, as in the original.
- The sky is drawn: the level's sky dome (clouds and the skyline gradient) follows the camera and rises a little when the character is high above the sea, as in the original. Where the dome ends below the horizon the plain background colour shows. The depth the original gives the sky is a stand-in (the sky is always behind everything), and a level with two sky objects uses the first.
- In some places the camera is fixed, as in the original: the police station and every area the level marks with a fixed camera. There it does not turn when you press the camera buttons, and a crossed-out camera picture shows at the bottom left of the top screen. Details such as the smooth change between fixed cameras are not done yet.
- While a script runs a camera sequence (a camera that flies along a path), the character cannot be controlled: the stick and the buttons do nothing, and other characters stop too unless the script keeps them running. When the sequence ends you can move again at once. In mission 1-1 this is why X does not let you out of the police car for the first 9 seconds. Other effects of the sequence (hidden display, stopped clocks) are not done yet.
- The camera keeps out of the level's walls. Near a wall it moves closer to the character and slides to the side, as measured in the original.
- The floor and walls come from the level. The character climbs steps and kerbs, and slides along walls.
- Many objects of the level block the character and the camera like the original: lamp posts, traffic lights, benches, bins, crates, barriers, doors and signs. The character is stopped by them and slides along them.
- The level's other people stand where the level puts them, with their own heads and clothes. They do not walk, talk or react. They play their idle animations, and the animations the mission scripts ask for (for example the van driver stuck in the mailbox in mission 1-1). A script that waits for such an animation waits until it has played, as in the original. Animations that scripts ask of the main character or of objects such as doors are not played yet.
- The character jumps, falls off ledges and lands as in the original, including the short pause after landing. Running uphill slows the character down. At some ledges the character first slides down a steep edge before it falls. Two details of that slide are not exact yet: how it ends and the short pause after landing from it.
- The character makes the sounds of the original: footsteps while walking, running and sprinting, a sound when jumping, and landing sounds that depend on how far the character fell. Picking up a stud plays its coin sound. Sounds come from the place where they happen and are quieter and panned to the side as in the original. See [Sound](#sound).
- In the level, the top screen shows the 3D view with the display of the original game on top of it, and the bottom screen shows the map. See [The display during play](#the-display-during-play).
- Doors that open by themselves swing open when the character comes close, wait while he stands there, and close again, with the same timing as the original. This works for the hinged doors and the sliding doors of the police station. Doors that the story opens (for example the front doors in the street) stay shut, because the story is not in the program yet. Doors make their opening and closing sounds.
- The street level loads and unloads its rooms while you walk, by the rule found in the original: the room you stand in, its neighbours and up to three rooms ahead of the camera. Rooms that are not loaded are not drawn. The room you stand in is found from the room models' size, a stand-in. The loading time (9 updates) is the one measured for one room. Characters and coins of unloaded rooms are still drawn. Other levels show all their rooms. The program prints `room streaming:`, `room A -> B` and `rooms loaded:` lines.
- The level's own scripts run while you play, with the same timing rules as the original (a script that waits continues later, a script can wait for another one). Scripts start when the level starts, when the character walks into or out of some marked places of the level, and when a script switches an object. Events of the story and many other kinds of starts do not start scripts yet; the program prints `script trigger event without check` for those once. A script can open and close doors.
- The question box of the police station door works: walk into the door area (after the first mission is switched on) and the game stops with the box "If you are on a mission...". A (OK) moves the character to the station door, B (Cancel) closes it. The box is drawn from measurements of one picture, so sizes may differ slightly. Screen fades from scripts darken the picture over half a second.
- You can walk out of the police station and back in. Leaving: walk to the lobby exit. A page with "Continue (A)" appears. Press A and you stand in the street where the original puts you. Entering: walk into the door area (after the first mission is switched on), answer the box with A, and press A again on the page. You stand in the lobby. The page is a stand-in: the picture on the top screen is a plain background, not the original's key art. The camera after the change is not the original's yet. The street level needs `--call-script 1-1_MissionSkip` for the door, unless you start with `--save` from a save that is past the first mission.
- Scripts can walk the player: after you answer the police station box with A, the character walks into the station by itself, at a speed measured in the original, in a straight line (a stand-in: the original follows a path). Other people do not walk yet. The AI commands of the scripts (enable, ignore player, invulnerable, pursuit and so on) are accepted and remembered but have no visible effect.
- Waypoints (the light columns of the missions) work on foot: walk into an active one and the player stops for about half a second, then the mission script it names starts (for example the donut shop of mission 1-1 starts `1-1_VanBreakdown`) and the waypoint switches off. The dev log prints `waypoint started script NAME at update N`. Waypoints for vehicles work too: they fire when the vehicle you sit in (or jump into) reaches them. A vehicle waypoint that stops you (for example the shop of the donut van) brakes the vehicle until its script starts. A normal waypoint does nothing while you are in a vehicle and shows a hint text instead. The use-an-object waypoints do not work yet. A small waypoint shows its police badge turning and hopping inside the light column, as in the original; the column and the badge do not fade in and out yet.
- Build-its (piles of bricks the story switches on) can be built: stand next to them and hold A. One brick is built per press and it takes the same number of updates as the original; after the last brick the finished object appears and the story script that waits for it starts (for the donut van of mission 1-1 the script switches the broken van off and the driveable van on; driving it is not in the program yet). The six bricks lie scattered around the pile and each flies to its place while it is built, then the finished object rises out of the pile. The character plays the building animation while you hold A, and a sound plays when each brick and the whole build is done. The bricks do not hop while you stand next to them yet (no hop sound), the camera does not frame the build, and the studs and the saved state of a finished build-it are missing.
- Scripts can remove a prop (`DestroyProp`), fade an object in or out (`EffectFadeObject`) and wait for a room to load (`StreamRoomIn`). Far objects do not fade out with distance and models near the camera do not turn see-through yet. `SetPartyCharacter` only prints `party character ... swap not ported` when a script asks for another character than the one you play.
- Most script commands are not worked out yet. The first time a script uses one of them, the program prints `script function without handler` and the command's name, and the script goes on as if the command had finished.
- The coins (studs) of the level spin where the level puts them. Walk close to one and it is picked up. It flies up to the stud counter. After one second the counter goes up (silver 10, gold 100, blue 1000, purple 10000), shows the new total for a few seconds and slides away again. The program also prints the counter at the end of a `--script` run.

## The display during play

- Top left of the top screen: the picture of the main character in its hexagon, and next to it four hearts. The heart at the end of the row beats slowly.
- The stud counter slides in below the hearts each time a stud is added. It shows the new total and a spinning coin in the colour of the last stud. The coin jumps for a moment when the total changes. After about two seconds the counter slides out again.
- The bottom screen shows the map of the level, turned so that the camera looks up. The arrow in the middle is the main character. The button at the top left is the characters button; it does nothing yet.
- Not like the original yet: the hearts do not change because nothing can hurt the character yet. The map is placed with the original's numbers for the first level (the picture equals the original's), and it shows the marker of the active waypoint. When the waypoint is far away, the marker stays at the edge of the map and an arrow points to it. Other mission marks and collectibles are not shown, and the map does not zoom yet. The display slides in a moment after the level starts; when the original does that is not known. Only the first level has a map picture that was checked.

## Sound

- The program plays the sounds of the character and the studs through your normal audio output. If no audio device can be opened, it prints a note and runs without sound.
- A sound is louder when it is close and silent when it is far away. It is panned left or right depending on where it is as seen from the camera.
- The volume options are `--sound-volume 0-10` (default 10) and `--music-volume 0-10` (default 8). They are the game's option values, which the game divides by 10.
- There is no music at the start of the police station or the street. The original starts none there either. Music that mission scripts start (the story script commands that choose a music track, or stop it) is played, but it switches at once without the original's fade between tracks and does not loop. Music that the original starts in fights or in vehicles is not played, because those parts are not worked out yet.
- Not played yet: the sounds of other people, animals, vehicles and objects of the level, the idle sounds of the character, and water or sand landing sounds.

## What does not work yet

- Carrying works for the donut box of mission 1-1: stand next to it and press A to pick it up (the story script starts), press A far from the target to put it down (it goes back to its start after 5 seconds), press A inside the delivery area to place it into the target (the story script that ends the mission starts). The character turns to the box (or the target), bends and lifts it with the original's animations, and holds it in his hands while walking at the original's carrying speed. The moments where the box leaves the counter and the hands are taken from the animations; the pickup takes as long as its animation (measured); how long the put-down takes is still a copy of a measured value. There are no sounds. A put-down box appears under the hands (the original also drops it from the hands to the floor below) with the heading it had in the hands.
- Getting into a vehicle works: stand within 2 units of an unlocked vehicle and press X (the E key by default). The character jumps in the way the original does (about 0.7 s, an arc to the seat) and is then hidden. Driving works: A accelerates, B brakes and reverses, the stick or the left and right D-pad steers (see [controls](controls.md#driving-a-vehicle)). The vehicle drives like the original: its engine, its single forward gear and reverse, its steering and its tyres are the original's, and the physics run in the original's fixed steps of 1/30 s. It falls with the original's gravity and rests on four sprung wheels, so it settles at the original's height, tilts with slopes, bounces over kerbs, leans back when it speeds up and dips when it brakes. Braking hard in a turn locks the rear wheels like the hand brake. It collides with the street the way the original does: its body cannot sink into the street or kerbs, it lands with the original's bounce, and the edges of the street stop it. When it stands still for a moment it goes to sleep like the original (after about 0.75 s). It also hits the level's solid blocks for vehicles (for example the one about 75 m ahead of the parked police car of mission 1-1) the way the original does: it stops there, bounces back a little and keeps pushing while you hold A. It collides with other vehicles the way the original does: a parked vehicle you drive into is pushed away and stops again by itself. It smashes lamp posts, phone boxes, bus shelters and other breakable props that it drives into: they disappear at once, without the bricks and dust of the original (not done yet), and the vehicle does not slow down, as in the original. Other objects (barriers and props of other kinds) are not checked yet, so the vehicle drives through them. The character can walk through a vehicle that has moved from its parking place. The drawn wheels turn, the front wheels steer and all wheels move with the springs. Wheels of vehicles that have not been driven yet stay at their class-file pose, and they never come off in the distance. The camera is the original's vehicle camera: it sits about 15 units behind the vehicle and swings round to stay behind it, the stick does not turn it, and the L and R buttons still do. Next to an unlocked vehicle a round X button appears above the character (it fades in and out, and it is hidden when the camera is closer than 6 units to it). While you sit in the vehicle the HUD shows the vehicle's picture and blue hearts with the vehicle's health. A hard hit (above 20 m/s) costs a heart. When the last heart is gone, or when you keep pushing the vehicle against something for 3 seconds without it moving, the vehicle breaks: its body disappears, the wheels fly off and bounce, and you cannot jump out. After 2 seconds the vehicle is back, whole, with full hearts, where it stood a moment before the break, and you still sit in it. The bricks, smoke and sounds of the break, and the fade of the screen at the return, are not done yet. The sounds and the horn are missing. Press X again to jump out next to the vehicle; it brakes to a stop by itself. In mission 1-1 the police car and the donut van stay locked until the story unlocks them, and the story script that waits for the police car to be entered starts when you get in (it holds the controls for about 9 seconds). The development options `--call-script 1-1_VanBreakdown --call-script 1-1_Get_Car` unlock the police car for testing. `--call-script 1-1_SO_VanFixed` makes the donut van drivable and starts `1-1_Van_Waypoint_Enter` (CTest `game-van-escort` drives it to the shop waypoint, where the story script jumps the character out).
- Only some kinds of objects block the character and the camera so far. People, animals and some special objects do not. Objects that the original switches off at the start (mission items, build-it parts and their results) are switched off by the same rules as in the original, but nothing switches them on later.
- Objects stay as they are at the start of the level. Nothing breaks or moves, and doors open only when the character comes close. An open door keeps the same blocking box as a closed one here. One measurement in the original differed, and the reason is not known yet.
- When the camera is stuck behind a wall for a longer time, the original places it again. That is not done here.
- Other characters do not block the character. Parked vehicles do: the character stops at them.
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

A new game in Cherry Tree Hills starts as in the original:

- The intro movie plays (once), then the first cutscene of mission 1-1 with the donut shop, then the first tip.
- Cutscenes fade the top screen to black before they start (when the script asks for it) and at their end, then fade the game picture back in. The fade times were measured in the original, not read from its code.
- The player is held during the cutscene. Add `--no-start-cutscenes` to skip the movie and the first cutscene while testing.

For the other options, such as `--screenshot` and `--frames`, see the [command reference](commands.md#development-runtime). Giving `--gameplay`, `--level`, `--script`, `--start` or `--save` also skips the front-end screens.

In-engine cutscenes that the level scripts start (for example the donut shop scene of mission 1-1) play in the level: the picture is the cutscene camera, the HUD is hidden and the player waits until it ends. Movies that a script starts play on the top screen. The faces of the actors and skipping are not done yet.
