# Project status

**The game is not playable on PC yet.** This project is at an early stage. The goal is a port that plays exactly like the original 3DS game, from start to finish, on Linux first. Windows comes after the Linux version is finished. Extra features come after that. Today the project can read the game's files and show many of them, and one level can be walked through. The rest of the game does not run yet.

## What works

- Extracting the executable and the game files from your own decrypted image.
- Unpacking the asset archive.
- Converting textures to PNG and sounds to WAV.
- Reading models, level layouts, navigation grids, skeletons, character animations, cutscene cameras, data tables, menu layouts, particle effect definitions, level camera settings and the game text.
- Showing models, levels, animated characters and in-engine cutscenes in the [model viewer](model-viewer.md).
- Playing sounds in the [sound test](sound-test.md).
- Playing the cutscene videos with subtitles in the [video player](fmv-player.md).
- Running a level in the [development runtime](commands.md#development-runtime). The main character walks, runs and sprints with the speeds measured in the original game. The level's collision keeps him on the floor and stops him at walls, as in the original. Many objects (lamp posts, crates, barriers, doors) stop him too. He jumps, falls off ledges and lands the way the original does (walking slowly off a ledge differs). The camera follows the character like the original and keeps out of the level's walls and of most of its objects. In the police station and other places with a fixed camera it stays fixed and shows the crossed-out camera picture when you try to turn it (the smooth change between fixed cameras is missing). The level's other people stand in place without moving. The level's coins (studs) spin and can be picked up. They fly to the stud counter on the screen, which goes up (some details are stand-ins, see the [development runtime](game-runtime.md)). The display of the original game is shown during play: the character's picture, the hearts, the stud counter and the map on the bottom screen (the map is a stand-in fitted to the original). The character's footsteps, jump and landing sounds and the coin sound play, with distance and left/right position as in the original. You can walk out of the police station into the street and back in through the front door (the loading page is a stand-in). Parts of mission 1-1 run: its scripts, the waypoints, the build-it, carrying the donut box, and getting into and driving a car (not like the original yet). On the way to the dog cage of mission 1-2 Chase can ride the zipline and climb the bricks. Nothing else of the game works. It is not gameplay.
- Showing the start screens of the original game in the [development runtime](game-runtime.md): loading page, logos, TT Games movie, language list, StreetPass and HOME pages, autosave notice, slot list, saving page, loading screen, intro movie with subtitles and the title card. Some parts are stand-ins. The slots of a save file are shown only with `--front-save`, and the delete page is missing.
- Testing keyboard and mouse controls in the [input laboratory](controls.md).
- Showing how complete a save file is, with `chase-savecheck`. See the [command reference](commands.md#programs).

## Not recovered yet

- Gameplay and menus. The level scripts run and some of them start when the character walks into marked places, but most script commands and many ways to start a script are not worked out yet.
- Reflections, shadows and draw sorting. Lighting and materials follow the game; which lights apply in each room is approximated.
- Blending between different animations, and the real animation speed.
- Collision with people, objects that break or move, the camera being placed again when it is stuck, and the character's other moves (fighting, climbing poles, bricks and rails). Vehicles are only partly done.
- Most music, and the sounds of other people, animals, vehicles and objects. For the character and the coins the sound for each event is known.
- Several smaller file types.
- Most of what a save game contains. The save file can be checked, read and written safely. Only the completion totals, the last level, the missions done, unlocks, level variables, studs, super bricks and the play time are understood. `chase-game` writes the save at the original's save requests and `chase-game --save` starts from a slot; collected items and object states are not saved or loaded yet.

## For developers

A developer tool runs your own game copy in the Azahar emulator, so our code can be compared with the original game. See the [command reference](commands.md#original-game-in-the-emulator-oracle).

The plans with their work logs and one page per feature (with its known gaps) are in [`plans/README.md`](plans/README.md) and [`plans/features/README.md`](plans/features/README.md).
