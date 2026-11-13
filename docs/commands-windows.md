# Commands on Windows

The commands of the [command reference](commands.md) that differ on Windows. Set up the tools first, as in [Getting started](getting-started.md#on-windows).

Back to the [documentation list](../README.md#more-documentation).

## Which window

| Window | How to open it | Use it for |
|---|---|---|
| Git Bash | Start menu, from Git for Windows | `git`, the Python tools and the Python tests |
| MSYS2 UCRT64 with the Windows `PATH` | `/c/msys64/msys2_shell.cmd -ucrt64 -use-full-path -here` in Git Bash | `cmake` and `ctest` (the programs in `build/` also start from any window) |

`cmake --build` copies the DLLs of MSYS2 that the programs need into `build` next to the `.exe` files (the folder of the compiler is searched; Windows system DLLs are not copied), so the programs in `build` start from any window or from the Explorer. The UCRT64 window is for building and testing; it also finds Git and the python.org Python through the Windows `PATH`.

## Build and test

In the UCRT64 window:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCHASE_BUILD_TESTS=ON -DPython3_EXECUTABLE="$(py -3 -c 'import sys; print(sys.executable)')"
cmake --build build -j"$(nproc)"
ctest --test-dir build -j"$(nproc)" -LE slow --output-on-failure
ctest --test-dir build -j"$(nproc)" --output-on-failure
```

- `-DPython3_EXECUTABLE` picks the python.org Python for the Python checks of `ctest`. Without it CMake takes MSYS2's own Python, which comes with FFmpeg.
- The tests that start `chase-game` or `chase-viewer` offscreen use SDL's Windows video driver and a hidden window (`SDL_VIDEODRIVER=windows` and `CHASE_HIDDEN_WINDOW=1`, set for you by CMake). They need a graphics card with OpenGL 4.6. Without one they should skip with the reason `no OpenGL 4.6 context`, which is not yet checked on such a machine.
- To run `chase-game` or `chase-viewer` by hand without showing the window, set `CHASE_HIDDEN_WINDOW=1`. Unset or `0` shows the window; any other value, even an empty one, stops the program with an error.

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

## Windows zip

`tools/package_windows.py` builds the downloadable zip from a finished build. Run it in the UCRT64 window (it asks the MSYS2 package manager about the DLLs), after the build:

```sh
python3 tools/package_windows.py --version v0.1.0 --commit "$(git rev-parse HEAD)" --repository https://example.org/owner/project --output-dir release
```

- `--version` is the label in the file name (a tag or a short commit), `--commit` the full hash of the source, `--repository` the https address named in the zip's `README.txt` for the source offer. All three are required, and a malformed value stops the tool with an error.
- `--output-dir` receives chase-windows-VERSION.zip; the tool never overwrites an existing zip. `--build-dir` (default `build`) is the folder with the programs, `--runtime-dir` the MSYS2 `bin` folder the build copied the DLLs from (default: the folder of `gcc`).
- The zip holds every `chase-*.exe` and every `.dll` of the build folder, the Python tools of [Extracting game data](extracting-game-data.md) with the modules they import, `config/controls.cfg`, `LICENSE`, a generated `README.txt` and the license files of the MSYS2 package of each bundled DLL. Test programs are left out.
- It refuses anything game-derived (`artifacts/`, images, audio, `.cci`, extracted game files) and anything that is not on its allow list, also when it reads the finished zip back. Its tests: `python3 -m unittest tests.test_package_windows`.
- The GitHub workflow runs it in the Windows job, starts the unzipped programs with only `C:\Windows\System32` on the `PATH`, and uploads the zip as a workflow artifact on every push. A pushed tag `v*` also publishes it on the Releases tab. The workflow has not run yet; see [the Windows build notes](plans/features/windows-build.md).

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
