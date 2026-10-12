# Project status

**The game is not playable on PC.** This project is at an early stage. It can read the game's files and show many of them, but none of the game itself runs yet.

## What works

- Extracting the executable and the game files from your own decrypted image.
- Unpacking the asset archive.
- Converting textures to PNG and sounds to WAV.
- Reading models, level layouts, navigation grids, skeletons, character animations and the game text.
- Showing models, levels and animated characters in the [model viewer](model-viewer.md).
- Playing sounds in the [sound test](sound-test.md).
- Testing keyboard and mouse controls in the [input laboratory](controls.md).

## Not recovered yet

- Gameplay, menus and scripts.
- The game's lighting and materials. The viewer shows an approximation.
- Blending between different animations, and the real animation speed.
- Which sound plays for which event.
- Cutscene videos (`.moflex`) and several smaller file types.
- Save games and settings.

## For developers

The plan and a detailed log of every finding are in [`plans/linux-port.md`](plans/linux-port.md) and [`plans/linux-port.progress.md`](plans/linux-port.progress.md).
