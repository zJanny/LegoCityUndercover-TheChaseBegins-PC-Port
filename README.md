# The Chase Begins — PC port (work in progress)

This project is building a PC port of *LEGO City Undercover: The Chase Begins* (Nintendo 3DS). The goal is a port that plays exactly like the original, from start to finish. It runs on Linux first. A Windows version is planned for after the Linux version is finished. **The game is not playable on PC yet.** Today the project can extract and decode the game's files, show models, levels and animated characters in development tools, and let you walk the main character around one level.

Game data is never stored in this repository. You need your own decrypted `.cci` image.

## Quick start

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
python3 tools/extract_romfs.py path/to/game.cci --output artifacts/romfs
build/chase-game
```

This starts the development runtime. It is not the game: it shows the start screens of the original (logos, language, slot list, intro movie), and after them you can only walk the player around one street. See [the development runtime guide](docs/game-runtime.md).

## Progress

<!-- progress:start -->
| Measure | Now | What it means |
|---|---|---|
| Script commands | 19 of 239 (8.0%) | Different commands the level scripts call that the port runs. They cover 59.1% of all calls. 1 more run with a stand-in result. |
| Object types | 104 of 506 (20.6%) | Different object types in the levels that the port treats with recovered rules (collision, doors, studs). They cover 52.4% of all placed objects. |
| Game files read | 99.6% of files, 99.9% of data | Files in the game archive (12836) that the port can decode, by count and by size. |
| Open work | 125 open items, 67 stand-ins | Known gaps and temporary guesses in 23 feature areas. It has no total to reach; watch the trend. |
| Checked against the original | 53 tests, 195 recorded traces | Automatic tests, and recordings of the original game that the tests compare with. |
| Game code documented | 811 of about 7575 (10.7%) | Functions of the game program that the notes or the source point to. The total is an estimate. |

Updated 2026-10-14, commit d9c8522. How the numbers are measured: [docs/progress.md](docs/progress.md).
<!-- progress:end -->

## Documentation

- [Building the tools](docs/building.md)
- [Command reference](docs/commands.md)
- [Extracting game data](docs/extracting-game-data.md)
- [Model viewer](docs/model-viewer.md)
- [Development runtime](docs/game-runtime.md)
- [Sound test](docs/sound-test.md)
- [Video player](docs/fmv-player.md)
- [Controls and the input laboratory](docs/controls.md)
- [Project status](docs/status.md)
- [How progress is measured](docs/progress.md)
