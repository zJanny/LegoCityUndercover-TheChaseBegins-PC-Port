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

In Git Bash, from the project folder. The tools find the library of the build, `build/libchase_assets_c.dll`, themselves; it needs no MSYS2 folder on the `PATH`. `CHASE_ASSETS_LIB` names another library when you need one:

```sh
python3 tools/extract_fib.py artifacts/romfs/romfs/lego_city3ds.fib --names artifacts/romfs/romfs/lego_city3ds.csv --output artifacts/fib
```

The other tools of [Extracting game data](extracting-game-data.md) run the same way.

## Merging an agent branch

`tools/merge_agent.py` runs CMake and `ctest` itself. Start it from Git Bash:

```sh
python3 tools/merge_agent.py prepare BRANCH
```

- Use the same command for `check` and `finish`.
- On Windows it puts the folder of its own Python and then the MSYS2 UCRT64 `bin` folder (`C:\msys64\ucrt64\bin`, or the `MSYSTEM_PREFIX` folder when that holds `gcc`) in front of the `PATH` of the programs it starts, and sets `CMAKE_GENERATOR=Ninja` when you have not set it and `ninja` is found. Values you set yourself win: a `PATH` that already lists the MSYS2 folder is kept, and so is your `CMAKE_GENERATOR`. Setting them by hand is optional:

```sh
PATH="$(dirname "$(command -v python3)"):/c/msys64/ucrt64/bin:$PATH" CMAKE_GENERATOR=Ninja python3 tools/merge_agent.py prepare BRANCH
```

- `merge_agent.py` passes its own Python to CMake as `-DPython3_EXECUTABLE`.
- `CMAKE_GENERATOR` applies only to a new `build` folder. Delete a `build` folder made with another generator first.
- A worktree has its `artifacts` folder as a directory junction to the game data. `merge_agent.py` removes only that link before it removes the worktree. To remove a worktree by hand, run `cmd /c rmdir <worktree>\artifacts` first. `git worktree remove`, with or without `--force`, follows the junction and deletes the game data behind it.
