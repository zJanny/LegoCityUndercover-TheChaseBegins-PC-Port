# Getting started

The [README](../README.md) has the short path from a fresh checkout to a running `chase-game`. This page has the details behind it.

## What this is

- The project builds a PC port of *LEGO City Undercover: The Chase Begins* (Nintendo 3DS). The goal is a port that plays exactly like the original, from start to finish.
- It runs on Linux first. A Windows version is planned for after the Linux version is finished.
- The game is not playable on PC yet. Today the project can extract and decode the game's files, show models, levels and animated characters in development tools, and let you walk the main character around a level and play parts of the first missions. See [Project status](status.md).
- Game data is never stored in this repository. You need your own decrypted `.cci` image.

## What `chase-game` is

`chase-game` is the development runtime. It is not the game. It shows the start screens of the original (logos, language, slot list, intro movie). After them you can walk the player around a street, with parts of mission 1-1 and a car you can get into. See the [development runtime guide](game-runtime.md).

## Check your game file

- The image must be decrypted.
- The tested version is the European one: title ID `00040000000AD600`, product code `CTR-P-AA8P`, version 1.0 (no update). The reference dump prints `078faf7e231381e53d7d7503d4729feab390e3cd1b8e739dc44f9933d8b759ad` for `sha256sum "game.cci"`.
- A different hash does not mean the image is wrong: a trimmed dump of the same build has another hash. The developer notes explain how to compare the build itself: [reference game version](re/game-version.md).
- Other regions are untested. `chase-game` reads only the asset files, not the game program, so another region may work. Some parts are taken from the European version and may not match: for example the language list of the start screens has the nine European languages. Please report what happens.
- An encrypted image is refused by the extraction tools.
- The tools never change your image.

## Packages

The build needs:

- Python 3.11 or newer, for the extraction tools.
- CMake 3.16 or newer and a C++17 compiler.
- SDL2, found through pkg-config.
- libepoxy, for `chase-game` and the model viewer. Without it they are not built.
- FFmpeg (libavformat, libavcodec, libswscale, libswresample, libavutil), for the movies in `chase-game` and the video player. Without it they are not built.

The commands in the README install these on Debian-based, Fedora and Arch-based systems. The build and start were checked on an Arch-based system that already had these libraries. The install lines themselves have not been run on a clean system, and the Debian and Fedora package names are not checked. Other systems need the same libraries under their own package names, with the development files.

Optional, only for the [emulator comparison tool](commands.md#original-game-in-the-emulator-oracle): the Azahar 3DS emulator and Xvfb.

## What the steps do

- **Build.** `-DCHASE_BUILD_TESTS=OFF` skips the test programs, so the build is shorter. To build the tests too, leave it out. See [Building the tools](building.md).
- **Extract.** `tools/extract_romfs.py` writes the game files to `artifacts/romfs`. `chase-game` reads them from there by default. `--fib` and `--names` select another place (see [commands](commands.md)).
- **Executable.** `tools/extract_game.py` extracts the game's program for analysis. You do not need it to run `chase-game`. See [Extracting game data](extracting-game-data.md).
- **More tools.** Unpacking the archive, converting textures, sounds, text and fonts: [Extracting game data](extracting-game-data.md).
- Every tool refuses to write into an output folder that already exists.

## Other programs

- [Model viewer](model-viewer.md), [sound test](sound-test.md), [video player](fmv-player.md), [input laboratory](controls.md).
- Run `chase-game` with `--gameplay` to skip the start screens and go straight to the level. Other options: [development runtime](game-runtime.md) and [commands](commands.md).

## Problems

- **`build/chase-game` is missing after the build.** libepoxy or FFmpeg was not found at configure time. Install them, then run the `cmake` configure and build commands again. The configure output says which one was found.
- **CMake or the compiler is not found.** Install the packages for your system again (README step 2).
- **`chase-game: cannot open archive`.** The program looks for `artifacts/romfs/romfs/lego_city3ds.fib` relative to the folder you start it from. Start it from the project folder, or pass `--fib` and `--names`.
- **The extraction stops with "encrypted NCCH".** The image must be decrypted. The tools do not decrypt.
- **The extraction stops with "output already exists".** Delete the folder or choose another `--output`.
- **A hash check fails.** The image is damaged or a different build. Check it as described above.
- **The window does not open or stays black.** The game needs OpenGL 4.6. Update the graphics driver.
- **Tests report skipped tests.** Some tests need the extracted data and are only added if it existed when `cmake` first ran. See [Building the tools](building.md#check-that-everything-works).
