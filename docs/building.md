# Building the tools

## What you need

- Python 3.11 or newer, for the extraction and conversion tools.
- CMake 3.16 or newer.
- A C++17 compiler.
- SDL2, found through pkg-config.
- libepoxy, which is needed only for the [model viewer](model-viewer.md) and the [development runtime](commands.md#development-runtime). Without it, both are skipped.
- FFmpeg (libavformat, libavcodec, libswscale, libswresample, libavutil), which is needed for the [video player](fmv-player.md) and for the movies of the [development runtime](game-runtime.md). Without it, both are skipped.
- Optional, only for the [emulator comparison tool](commands.md#original-game-in-the-emulator-oracle): the Azahar 3DS emulator and Xvfb.

## Build

Run these commands from the repository folder:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

The programs are written to `build/`. The build also makes `libchase_assets_c.so`, which the unpacking and conversion tools use (see [Extracting game data](extracting-game-data.md)). Build the project before you run them.

## Check that everything works

```sh
ctest --test-dir build --output-on-failure
python3 -m unittest discover -s tests -p 'test_*.py' -v
```

Some tests need your extracted game data in `artifacts/romfs` (see [Extracting game data](extracting-game-data.md)). Without it, they are reported as skipped.

- Use `-DCHASE_ROMFS_DIR=...` when you configure, if the data is somewhere else.
- Use `-DCHASE_LOCO_TRACE_DIR=...` if the recorded movement traces are not in `artifacts/oracle/loco-out`. Without the traces, the movement comparison test is skipped.
- The viewer, development runtime, sound and video player tests are only added if the data existed when you ran the first `cmake` command. Run that command again after you extract the data.

For extraction, converter and program commands, see the [command reference](commands.md).
