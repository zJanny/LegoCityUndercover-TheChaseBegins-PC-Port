# Commands on Windows

The commands of the [command reference](commands.md) that differ on Windows. Set up the tools first, as in [Getting started](getting-started.md#on-windows).

Back to the [documentation list](../README.md#more-documentation).

## Which window

| Window | How to open it | Use it for |
|---|---|---|
| Git Bash | Start menu, from Git for Windows | `git`, the Python tools and the Python tests |
| MSYS2 UCRT64 with the Windows `PATH` | `/c/msys64/msys2_shell.cmd -ucrt64 -use-full-path -here` in Git Bash | `cmake`, `ctest` and the programs in `build/` |

The programs need the DLLs of MSYS2, so start them from the UCRT64 window. That window also finds Git and the python.org Python through the Windows `PATH`.

## Build and test

In the UCRT64 window:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCHASE_BUILD_TESTS=ON -DPython3_EXECUTABLE="$(py -3 -c 'import sys; print(sys.executable)')"
cmake --build build -j"$(nproc)"
ctest --test-dir build -j"$(nproc)" -LE slow --output-on-failure
ctest --test-dir build -j"$(nproc)" --output-on-failure
```

- `-DPython3_EXECUTABLE` picks the python.org Python for the Python checks of `ctest`. Without it CMake takes MSYS2's own Python, which comes with FFmpeg.
- The tests that start `chase-game` or `chase-viewer` offscreen are reported as skipped, and the 5 tests that check their pictures as disabled. SDL needs `libEGL.dll` for its offscreen mode, and MSYS2 has none.

In Git Bash:

```sh
python3 -m unittest discover -s tests -p 'test_*.py' -v
```

## Python tools

In Git Bash, from the project folder, point the tools to the Windows library once per window:

```sh
export CHASE_ASSETS_LIB=build/libchase_assets_c.dll
python3 tools/extract_fib.py artifacts/romfs/romfs/lego_city3ds.fib --names artifacts/romfs/romfs/lego_city3ds.csv --output artifacts/fib
```

The other tools of [Extracting game data](extracting-game-data.md) run the same way.

## Merging an agent branch

`tools/merge_agent.py` runs CMake and `ctest` itself. Start it from Git Bash with the python.org Python first on the `PATH`, then the MSYS2 tools, and Ninja as the CMake generator:

```sh
PY_DIR="$(dirname "$(command -v python3)")"
PATH="$PY_DIR:/c/msys64/ucrt64/bin:$PATH" CMAKE_GENERATOR=Ninja python3 tools/merge_agent.py prepare BRANCH
```

- Use the same prefix for `check` and `finish`.
- `merge_agent.py` passes its own Python to CMake as `-DPython3_EXECUTABLE`.
- `CMAKE_GENERATOR` applies only to a new `build` folder. Delete a `build` folder made with another generator first.
