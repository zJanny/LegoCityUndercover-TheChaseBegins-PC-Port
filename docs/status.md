# Project status

**The game is not playable on PC.** This project is at an early stage. It can read the game's files and show many of them, but none of the game itself runs yet.

## What works

- Extracting the executable and the game files from your own decrypted image.
- Unpacking the asset archive.
- Converting textures to PNG and sounds to WAV.
- Reading models, level layouts, navigation grids, skeletons, character animations, cutscene cameras, data tables, menu layouts, particle effect definitions, level camera settings and the game text.
- Showing models, levels, animated characters and in-engine cutscenes in the [model viewer](model-viewer.md).
- Playing sounds in the [sound test](sound-test.md).
- Playing the cutscene videos with subtitles in the [video player](fmv-player.md).
- Running a level in the [development runtime](commands.md#development-runtime). The main character walks, runs and sprints with the speeds measured in the original game. The level's fixed collision keeps him on the floor and stops him at walls, as in the original. He jumps, falls off ledges and lands the way the original does (walking slowly off a ledge differs). The camera follows the character like the original and keeps out of the level's fixed walls (objects do not push it). Nothing else of the game works. It is not gameplay.
- Testing keyboard and mouse controls in the [input laboratory](controls.md).
- Showing how complete a save file is, with `chase-savecheck`. See the [command reference](commands.md#programs).

## Not recovered yet

- Gameplay, menus and scripts.
- The game's lighting and materials. The viewer shows an approximation.
- Blending between different animations, and the real animation speed.
- Objects pushing the camera, the camera being placed again when it is stuck, objects blocking the character and the character's other moves (fighting, climbing, vehicles).
- Which sound plays for which event.
- Several smaller file types.
- Most of what a save game contains. The save file can be checked, read and written safely, but only the completion totals are understood. No game program uses saves.

## For developers

A developer tool runs your own game copy in the Azahar emulator, so our code can be compared with the original game. See the [command reference](commands.md#original-game-in-the-emulator-oracle).

The plan and a detailed log of every finding are in [`plans/linux-port.md`](plans/linux-port.md) and [`plans/linux-port.progress.md`](plans/linux-port.progress.md).
