# Building the tools

## What you need

- Python 3.11 or newer, for the extraction and conversion tools.
- CMake 3.16 or newer.
- A C++17 compiler.
- SDL2, found through pkg-config.
- libepoxy, which is needed only for the model viewer. Without it, the viewer is skipped.

## Build

Run these commands from the repository folder:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

The programs are written to `build/`.

## Check that everything works

```sh
ctest --test-dir build --output-on-failure
python3 -m unittest discover -s tests -p 'test_*.py' -v
```

Some tests need your extracted game data in `artifacts/romfs` (see [Extracting game data](extracting-game-data.md)). Without it, they are reported as skipped.

- Use `-DCHASE_ROMFS_DIR=...` when you configure, if the data is somewhere else.
- The viewer test is only added if the data existed when you ran the first `cmake` command. Run that command again after you extract the data.
