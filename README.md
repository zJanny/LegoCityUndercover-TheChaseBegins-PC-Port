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

This starts the development runtime. It is not the game: you can only walk the player around the police station. See [the development runtime guide](docs/game-runtime.md).

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
