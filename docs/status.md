# Project status

**The game is not playable on PC.** This project is at an early stage. It can read the game's files and show many of them, but none of the game itself runs yet.

## What works

- Extracting the executable and the game files from your own decrypted image.
- Unpacking the asset archive.
- Converting textures to PNG and sounds to WAV.
- Reading models, level layouts, navigation grids, skeletons, character animations, cutscene cameras, data tables, menu layouts, level camera settings and the game text.
- Showing models, levels, animated characters and in-engine cutscenes in the [model viewer](model-viewer.md).
- Playing sounds in the [sound test](sound-test.md).
- Playing the cutscene videos with subtitles in the [video player](fmv-player.md).
- Running a level in the [development runtime](commands.md#development-runtime). It only shows the level and moves a test camera. It is not gameplay.
- Testing keyboard and mouse controls in the [input laboratory](controls.md).

## Not recovered yet

- Gameplay, menus and scripts.
- The game's lighting and materials. The viewer shows an approximation.
- Blending between different animations, and the real animation speed.
- Which sound plays for which event.
- Several smaller file types.
- What a save game contains. The save file can be checked, read and written safely, but the progress and settings inside it are not understood yet. No program uses saves yet.

## For developers

The plan and a detailed log of every finding are in [`plans/linux-port.md`](plans/linux-port.md) and [`plans/linux-port.progress.md`](plans/linux-port.progress.md).
