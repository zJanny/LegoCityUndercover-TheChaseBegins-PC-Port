# Getting started

The [README](../README.md) has the short path from a fresh checkout to a running `chase-game`. This page has the details behind it.

## What this is

- The project builds a PC port of *LEGO City Undercover: The Chase Begins* (Nintendo 3DS). The goal is a port that plays exactly like the original, from start to finish.
- It runs on Linux first. The Windows version is work in progress: it builds and starts, with the gaps listed in [Project status](status.md#on-windows).
- The game is not playable on PC yet. Today the project can extract and decode the game's files. Development tools show models, levels and animated characters. You can walk the main character around a level and play parts of the first missions. See [Project status](status.md).
- Game data is never stored in this repository. You need your own decrypted `.cci` image.

## What `chase-game` is

`chase-game` is the development runtime. It is not the game. It shows the start screens of the original (logos, language, slot list, intro movie). After them you can walk the player around a street of the city and play through parts of the first missions. See the [development runtime guide](game-runtime.md) and [Project status](status.md) for what works.

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

The commands in the README install these on Debian-based, Fedora and Arch-based systems. The build and start were checked on one Linux system (Arch-based) that already had these libraries. The install lines themselves have not been run on a clean system, and the Debian and Fedora package names are not checked. Other systems need the same libraries under their own package names, with the development files. On Windows the MSYS2 packages provide them; see [On Windows](#on-windows).

Optional, only for the [emulator comparison tool](commands.md#original-game-in-the-emulator-oracle): the Azahar 3DS emulator (version 2126.2 tested; `--rpc` needs a build with the RPC server), Xvfb, `libX11` and `libXtst`. The [requirements list](commands.md#original-game-in-the-emulator-oracle) has the details.

## What the steps do

- **Build.** `-DCHASE_BUILD_TESTS=OFF` skips the test programs, so the build is shorter. To build the tests too, leave it out. See [Building the tools](building.md).
- **Extract.** `tools/extract_romfs.py` writes the game files to `artifacts/romfs`. `chase-game` reads them from there by default. `--fib` and `--names` select another place (see [commands](commands.md)).
- **Executable.** `tools/extract_game.py` extracts the game's program for analysis. You do not need it to run `chase-game`. See [Extracting game data](extracting-game-data.md).
- **More tools.** Unpacking the archive, converting textures, sounds, text and fonts: [Extracting game data](extracting-game-data.md).
- Every tool refuses to write into an output folder that already exists.

## On Windows

These are the steps of the README's "Quick start on Windows", with details. They were checked on Windows 11 (x86_64). The Windows version is work in progress; its gaps are in [Project status](status.md#on-windows).

You use two windows:

- **Git Bash**, from Git for Windows, for Git and the Python tools.
- **MSYS2 UCRT64**, a shell of [MSYS2](https://www.msys2.org/), for building, testing and starting the programs. MSYS2 provides the GCC compiler and the libraries (UCRT64 is its 64-bit environment).

Requirements:

- Windows 11, x86_64, with a graphics driver for OpenGL 4.6. Windows 10 needs version 1903 or later for paths with letters outside the ANSI code page; Windows 10 has not been tested.
- CMake 3.19 or newer; the MSYS2 UCRT64 package is newer.
- About 3 GB of disk space for the project and 2 GB for the MSYS2 packages.
- [Git for Windows](https://git-scm.com/download/win).
- [Python 3.11 or newer for Windows](https://www.python.org/downloads/windows/), with the `py` launcher (part of the installer). The commands call it as `python3`. In Git Bash, `python3 -V` must print `Python 3.11` or newer. If it opens the Microsoft Store or prints nothing, set up `python3` as described below.

Set up `python3` once:

- In the Python installer, tick "Add python.exe to PATH".
- Open Settings, Apps, Advanced app settings, App execution aliases. Turn off the entries for `python.exe` and `python3.exe`. They start the Microsoft Store.
- The installer creates `python.exe` only. Find the install folder with `py -3 -c "import sys; print(sys.prefix)"` and copy `python.exe` to `python3.exe` in it.
- Open a new Git Bash window and run `python3 -V`. It prints `Python 3.11` or newer.

1. **Get the code.** Run the `git clone` command of the README in Git Bash. `-c core.autocrlf=false` keeps the Linux line endings (LF) in your checkout, so Windows and Linux builds read the same files. The [Windows rules](rules/windows.md) explain why.
2. **Install MSYS2 and its packages.** Install MSYS2 with its installer into the default folder `C:\msys64`. Open **MSYS2 UCRT64** from the Start menu (the installer can open it at its end) and run the two `pacman` lines of the README. `pacman -Syu` updates MSYS2 itself. It may ask to close the window; then open it again and run both lines again. The second line installs GCC, CMake, Ninja, pkgconf, SDL2, libepoxy and FFmpeg for UCRT64. MSYS2 provides SDL2 as `sdl2-compat`, the SDL2 interface on top of SDL3. Its FFmpeg needs that package, which conflicts with MSYS2's classic `SDL2` package. FFmpeg also brings MSYS2's own Python; the tools still use the python.org one. When both lines are done, `gcc --version` prints a version.
3. **Build.** In Git Bash, in the project folder, run `/c/msys64/msys2_shell.cmd -ucrt64 -use-full-path -here`. It opens the UCRT64 window in the project folder and keeps the Windows `PATH`, so Git and Python are found there too. Run the two `cmake` commands of the README in that window. Ninja counts the build steps, and the last line shows both numbers equal, for example `[693/693] Linking CXX executable chase-game.exe`. The build writes `build/chase-game.exe` and the other programs; the shell runs them as `build/chase-game`.
4. **Extract.** Run `python3 tools/extract_romfs.py "path/to/game.cci" --output artifacts/romfs` in Git Bash. Write the path of your image with forward slashes, for example `C:/Games/game.cci` or `/c/Games/game.cci`. The last line names the output folder with a backslash: `51 files, 763156067 bytes -> artifacts\romfs`.
5. **Start.** Run `build/chase-game` in the UCRT64 window. The programs need the DLLs in `C:\msys64\ucrt64\bin`, which only that window has on its `PATH`. Started from Git Bash or the Explorer, Windows reports a missing DLL.

- **Saves.** The game saves to `%APPDATA%\chase\savegame.dat`, normally `C:\Users\<you>\AppData\Roaming\chase\savegame.dat`, and creates the folder. At the start the game prints the full path, for example `save: new game in slot 0, save requests write C:\Users\<you>\AppData\Roaming\chase\savegame.dat`. Start from it with `build/chase-game --save "$APPDATA/chase/savegame.dat" --slot 0` instead of the `~/.local/share` path of the README.
- **Tests and tools.** [Building the tools](building.md#build-on-windows) has the test commands and the Python setting for the tests.

## Other programs

- [Model viewer](model-viewer.md), [sound test](sound-test.md), [video player](fmv-player.md), [input laboratory](controls.md).
- Run `chase-game` with `--gameplay` to skip the start screens and go straight to the level. Other options: [development runtime](game-runtime.md) and [commands](commands.md).

## Problems

- **`build/chase-game` is missing after the build.** libepoxy or FFmpeg was not found at configure time. Install them, then run the `cmake` configure and build commands again. The configure output says which one was found.
- **CMake or the compiler is not found.** Install the packages for your system again (README step 2).
- **`chase-game: cannot open archive` or `cannot open controls: config/controls.cfg`.** The program looks for `artifacts/romfs/romfs/lego_city3ds.fib` and `config/controls.cfg` relative to the folder you start it from. Start it from the project folder, or pass `--fib`, `--names` and `--config`.
- **The extraction stops with "encrypted NCCH".** The image must be decrypted. The tools do not decrypt.
- **The extraction stops with "output already exists".** Delete the folder or choose another `--output`.
- **A hash check fails.** The image is damaged or a different build. Check it as described above.
- **The window does not open or stays black.** The game needs OpenGL 4.6. Update the graphics driver.
- **Windows: `cmake: command not found` or `pacman: command not found`.** You are in Git Bash. Open the UCRT64 window of step 3 and run the command there.
- **Windows: a program reports a missing DLL such as `SDL2.dll`.** Start it from the UCRT64 window of step 3.
- **Windows: `Python was not found` or the Microsoft Store opens.** `python3` points to the Store alias of Windows. Set it up as described under [On Windows](#on-windows).
- **Tests report skipped tests.** Some tests need the extracted data and are only added if it existed when `cmake` first ran. See [Building the tools](building.md#check-that-everything-works).
