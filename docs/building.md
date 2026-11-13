# Building the tools

How to install the packages, build the programs and run the tests on Linux and on Windows.

Back to the [documentation list](../README.md#more-documentation).

## What you need

- Python 3.11 or newer, for the extraction and conversion tools.
- CMake 3.16 or newer.
- A C++17 compiler.
- SDL2, found through pkg-config.
- libepoxy, which is needed only for the [model viewer](model-viewer.md) and the [development runtime](commands.md#development-runtime). Without it, both are not built.
- FFmpeg (libavformat, libavcodec, libswscale, libswresample, libavutil), which is needed for the [video player](fmv-player.md), `chase-frontshot` and the [development runtime](game-runtime.md). The runtime plays the movies of the start screens with it. Without FFmpeg, these are not built.
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

## Build on Windows

The Windows build uses GCC and the libraries of [MSYS2](https://www.msys2.org/) in its UCRT64 environment, with the Ninja build tool. It is work in progress; see [Project status](status.md#on-windows). Install the tools as in [Getting started](getting-started.md#on-windows).

The MSYS2 UCRT64 packages:

```sh
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-pkgconf mingw-w64-ucrt-x86_64-sdl2-compat mingw-w64-ucrt-x86_64-libepoxy mingw-w64-ucrt-x86_64-ffmpeg
```

Run the build and the tests in the UCRT64 window that keeps the Windows `PATH`. Open it from Git Bash in the project folder:

```sh
/c/msys64/msys2_shell.cmd -ucrt64 -use-full-path -here
```

Then configure, build and test there:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCHASE_BUILD_TESTS=ON -DPython3_EXECUTABLE="$(py -3 -c 'import sys; print(sys.executable)')"
cmake --build build -j"$(nproc)"
ctest --test-dir build -j"$(nproc)" --output-on-failure
```

- `-G Ninja` selects the Ninja build tool. The build writes the programs as `build/*.exe` and the library as `build/libchase_assets_c.dll`.
- `-DPython3_EXECUTABLE` sets the Python that the Python checks of `ctest` use. FFmpeg brings MSYS2's own Python, which comes first in this window, so name the python.org Python here. The `py` launcher prints its path.
- The build copies every DLL that a program or library of the build imports (transitively, except the Windows system DLLs) from the folder of the compiler, `C:\msys64\ucrt64\bin`, into `build` next to the `.exe` files (about 100 files, 150 MB). Afterwards the programs start from the Explorer, PowerShell or Git Bash without MSYS2 on the `PATH`. The first build takes about 30 seconds longer to read the DLL lists of all programs; later builds read only the programs that changed (about a second) and copy a DLL again when MSYS2 updated it. `sdl2-compat` loads `SDL3.dll` while running, which no program lists, so the build copies it too (`CHASE_RUNTIME_LOADED_DLLS`). A DLL that is in no folder searched stops the build with its name. The tests `runtime-dlls-present` (slow) and `runtime-dlls-start-without-msys2` check the result.
- The programs read their paths as UTF-8: a manifest in every `.exe` sets the code page of the process, so folders with letters outside the Windows ANSI code page (for example Cyrillic or Japanese) work for options such as `--config`. This needs Windows 10 version 1903 or later. The Python tools that load the native library do not accept such folders yet.
- The tests need Git, which the window finds through the Windows `PATH` (`-use-full-path`).
- Run the Python tests in Git Bash: `python3 -m unittest discover -s tests -p 'test_*.py' -v`.
- On Windows, the tests that start `chase-game` or `chase-viewer` offscreen use SDL's Windows video driver and a hidden window (`CHASE_HIDDEN_WINDOW=1`, set by `cmake/platform_files.cmake`). They need a graphics card with OpenGL 4.6. Without one they should skip with the reason `no OpenGL 4.6 context`, which is not yet checked on such a machine. The list is in [the Windows test notes](plans/features/windows-tests.md).
- The extraction and conversion tools load `build/libchase_assets_c.dll`. They find it themselves on Windows; `CHASE_ASSETS_LIB` is optional there and names another path ([Extracting game data](extracting-game-data.md#3-unpack-the-asset-archive)).
- `-DCHASE_BUILD_TESTS=ON` builds the tests again after a build with `-DCHASE_BUILD_TESTS=OFF`, as in the README. Without tests the build needs no Python setting.

### Linux tests in WSL

A change on Windows to code that Linux shares also has to pass the Linux build and tests ([Windows rules](rules/windows.md#linux-stays-green)). You run them in WSL, the Linux subsystem of Windows, with an Ubuntu distribution and the Debian packages of the README. The tests that run `chase-game` offscreen need OpenGL 4.6 in WSL, which comes from your graphics card through Mesa's D3D12 driver.

1. Install WSL with an Ubuntu distribution, as in [Microsoft's guide](https://learn.microsoft.com/windows/wsl/install), and the Debian packages of the README in Ubuntu. Install a current graphics driver with WSL support. Check with `wsl --list` in PowerShell that Ubuntu is listed. Then update WSL:

   ```powershell
   wsl --update
   ```

2. In WSL, add your user to the `render` and `video` groups and install the EGL check tool:

   ```sh
   sudo usermod -aG render,video "$USER"
   sudo apt install mesa-utils
   ```

   Then end WSL in PowerShell, so the next start has the new groups:

   ```powershell
   wsl --shutdown
   ```

   `wsl --shutdown` ends every WSL session, so run it yourself only when no other program or agent session uses WSL.

3. Check the driver in WSL:

   ```sh
   GALLIUM_DRIVER=d3d12 eglinfo -B
   ```

   You should see `OpenGL core profile renderer: D3D12 (...)` with your graphics card, and `OpenGL core profile version: 4.6 (Core Profile) Mesa ...`.

4. Build and test in a WSL clone of your branch, not in the Windows checkout. Replace `BRANCH` and `PROJECT` with your branch and the WSL path of your Windows checkout, for example `/mnt/c/path/to/project`:

   ```sh
   export GALLIUM_DRIVER=d3d12
   git clone -q -b BRANCH PROJECT ~/wslcheck
   cd ~/wslcheck
   ln -s PROJECT/artifacts artifacts
   cmake -S . -B build-wsl -DCMAKE_BUILD_TYPE=RelWithDebInfo
   cmake --build build-wsl -j4
   ctest --test-dir build-wsl -j4 --output-on-failure
   python3 -m unittest discover -s tests -p 'test_*.py'
   ```

Without `GALLIUM_DRIVER=d3d12` Mesa uses its software driver llvmpipe, which offers only OpenGL 4.5. The offscreen tests then fail with `Could not create EGL context`.

## Continuous integration

The workflow files build the project and run the tests for every push to `main` and every pull request. A second workflow runs every night and by hand, including the slow tests. Neither runs tests that need game data.

- The jobs install CMake 4.4.5 from Kitware's release (checked against its SHA256), so CI uses the same CMake as a current developer setup.
- The jobs install `g++`, `pkg-config`, `python3`, `libsdl2-dev`, `libepoxy-dev` and the five FFmpeg development packages (`libavformat-dev`, `libavcodec-dev`, `libswscale-dev`, `libswresample-dev`, `libavutil-dev`) with `apt-get`.
- The build fails on any compiler warning.
- The workflow files are `.forgejo/workflows/ci.yml`, `.forgejo/workflows/full.yml`, `.github/workflows/ci.yml` and `.github/workflows/full.yml`.
- On GitHub, `.github/workflows/ci.yml` has a second job, `windows-build-and-test`, on a `windows-latest` runner. It installs the MSYS2 UCRT64 packages of [Build on Windows](#build-on-windows) with `msys2/setup-msys2@v2`. It uses the Python 3.12 of `actions/setup-python`. It builds a `Release` build with `-O2` (not `RelWithDebInfo`, so the programs in the release zip carry no debug info) with Ninja and warnings as errors, runs the Python tests, then runs `ctest -LE 'slow|needs-game-data'`. Forgejo has no Windows job, because it has no hosted Windows runners. The job has not run on GitHub yet; see [the Windows build notes](plans/features/windows-build.md).
- After the tests, the Windows job runs `tools/package_windows.py`, which asks `pacman -Qo` and `pacman -Ql` for the license files of the package of each bundled DLL, and builds the zip of the [Windows download](../README.md#windows-download). It unzips the zip into a fresh folder and starts `chase-input-lab` and the asset library with only `C:\Windows\System32` on the `PATH`. On every push it uploads the zip as a workflow artifact. On a pushed tag `v*` it also creates or updates the GitHub release of that tag and attaches the zip. The job has `contents: write` for that. Command: [Commands on Windows](commands-windows.md#windows-zip).
- Before the checkout, the Windows job sets `core.autocrlf=true`, the default of Git for Windows. Then it checks that the clone still has LF line endings. It runs `tests/test_line_endings.py` and fails when `git ls-files --eol` shows a text file as `w/crlf` or `w/mixed`. See [the line-endings notes](plans/features/windows-line-endings.md).
- The GitHub runners have no graphics card, so the OpenGL tests skip there. Tests that need game data skip too.
- The exact commands to reproduce a run locally are in the [command reference](commands.md#continuous-integration).

For extraction, converter and program commands, see the [command reference](commands.md).
