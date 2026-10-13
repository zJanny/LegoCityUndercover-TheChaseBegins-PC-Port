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
- Running a level in the [development runtime](commands.md#development-runtime). The main character walks, runs and sprints with the speeds measured in the original game. The level's collision keeps him on the floor and stops him at walls, as in the original. Many objects (lamp posts, crates, barriers, doors) stop him too. He jumps, falls off ledges and lands the way the original does (walking slowly off a ledge differs). The camera follows the character like the original and keeps out of the level's walls and of most of its objects. The level's other people stand in place without moving. The level's coins (studs) spin and can be picked up, and a stud counter goes up (some details are stand-ins, see the [development runtime](game-runtime.md)). The character's footsteps, jump and landing sounds and the coin sound play, with distance and left/right position as in the original. Nothing else of the game works. It is not gameplay.
- Showing the start screens of the original game in the [development runtime](game-runtime.md): loading page, logos, TT Games movie, language list, StreetPass and HOME pages, autosave notice, slot list, saving page, loading screen, intro movie with subtitles and the title card. Some parts are stand-ins, and saved games are not shown.
- Testing keyboard and mouse controls in the [input laboratory](controls.md).
- Showing how complete a save file is, with `chase-savecheck`. See the [command reference](commands.md#programs).

## Not recovered yet

- Gameplay, menus and scripts.
- The game's lighting and materials. The viewer shows an approximation.
- Blending between different animations, and the real animation speed.
- Collision with parked cars and people, objects that break or move, the camera being placed again when it is stuck, and the character's other moves (fighting, climbing, vehicles).
- Music, and the sounds of other people, animals, vehicles and objects. For the character and the coins the sound for each event is known.
- Several smaller file types.
- Most of what a save game contains. The save file can be checked, read and written safely, but only the completion totals are understood. No game program uses saves.

## For developers

A developer tool runs your own game copy in the Azahar emulator, so our code can be compared with the original game. See the [command reference](commands.md#original-game-in-the-emulator-oracle).

The plan and a detailed log of every finding are in [`plans/linux-port.md`](plans/linux-port.md) and [`plans/linux-port.progress.md`](plans/linux-port.progress.md).
