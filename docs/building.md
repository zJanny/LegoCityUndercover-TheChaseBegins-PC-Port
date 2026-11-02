# Building the tools

## What you need

- Python 3.11 or newer, for the extraction and conversion tools.
- CMake 3.16 or newer.
- A C++17 compiler.
- SDL2, found through pkg-config.
- libepoxy, which is needed only for the [model viewer](model-viewer.md) and the [development runtime](commands.md#development-runtime). Without it, both are not built.
- FFmpeg (libavformat, libavcodec, libswscale, libswresample, libavutil), which is needed for the [video player](fmv-player.md), `chase-frontshot` and the [development runtime](game-runtime.md), which plays the movies of the start screens. Without it, these are not built.
- Optional, only for the [emulator comparison tool](commands.md#original-game-in-the-emulator-oracle): the Azahar 3DS emulator (version 2126.2 tested; `--rpc` needs a build with the RPC server), Xvfb, `libX11` and `libXtst`. The [requirements list](commands.md#original-game-in-the-emulator-oracle) has the details.

## Build

Run these commands from the repository folder:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j"$(nproc)"
```

The programs are written to `build/`. The build also makes `libchase_assets_c.so`, which the unpacking and conversion tools use (see [Extracting game data](extracting-game-data.md)). Build the project before you run them.

### Build only the game, without tests

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCHASE_BUILD_TESTS=OFF
cmake --build build -j"$(nproc)"
```

- This skips every test program and every CTest test, so the build is shorter.
- `ctest` then finds no tests. To get them back, configure again with `-DCHASE_BUILD_TESTS=ON` (the default).

### Faster rebuilds with ccache (optional)

- If `ccache` is installed, CMake uses it automatically as the compiler launcher. Repeated builds, for example in a new worktree, then reuse compiled files.
- Nothing changes when `ccache` is missing. You install it yourself with your package manager.
- To switch it off, configure with `-DCHASE_USE_CCACHE=OFF`.
- A launcher you set yourself (`CMAKE_CXX_COMPILER_LAUNCHER`) is kept.

## Check that everything works

```sh
ctest --test-dir build -j"$(nproc)" --output-on-failure            # all tests, about one and a half minutes on a recent 8-core machine
ctest --test-dir build -j"$(nproc)" -LE slow --output-on-failure   # without the slow tests, about half a minute on the same machine
python3 -m unittest discover -s tests -p 'test_*.py' -v
```

Some tests need your extracted game data in `artifacts/romfs` (see [Extracting game data](extracting-game-data.md)). Without it, they are reported as skipped or are not added.

- Use `-DCHASE_ROMFS_DIR=...` when you configure, if the data is somewhere else.
- The viewer, development runtime, sound, video player and start-screen tests are only added if the data existed when you ran the first `cmake` command. Run that command again after you extract the data.

Tests that need the game data carry the CTest label `needs-game-data`. Exclude them with `-LE needs-game-data`, for example on a machine without the data.

## Continuous integration

The workflow files build the project and run the tests for every push to `main` and every pull request. A second workflow runs every night and by hand, including the slow tests. Neither runs tests that need game data.

- The jobs install CMake 4.4.5 from Kitware's release (checked against its SHA256), so CI uses the same CMake as a current developer setup.
- The jobs install `g++`, `pkg-config`, `python3`, `libsdl2-dev`, `libepoxy-dev` and the five FFmpeg development packages (`libavformat-dev`, `libavcodec-dev`, `libswscale-dev`, `libswresample-dev`, `libavutil-dev`) with `apt-get`.
- The build fails on any compiler warning.
- The workflow files are `.forgejo/workflows/ci.yml`, `.forgejo/workflows/full.yml`, `.github/workflows/ci.yml` and `.github/workflows/full.yml`.
- The exact commands to reproduce a run locally are in the [command reference](commands.md#continuous-integration).

For extraction, converter and program commands, see the [command reference](commands.md).
