# Closed items of windows-platform

Done items of [windows-platform.md](windows-platform.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] WINPLAT-1 — the default save path is `%APPDATA%\chase\savegame.dat` on Windows (`save_directory_windows.cpp`); evidence: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437)
- [x] WINPLAT-3 — the debug overlay memory line reads the working set on Windows (`process_memory_windows.cpp`); evidence: [debug-overlay.md](../../debug-overlay.md)
- [x] WINPLAT-2 — closed 2026-11-13 by 20261113-winopen-D: the default library name follows the platform (verified behaviour in [windows-platform.md](windows-platform.md)); `CHASE_ASSETS_LIB` is no longer needed on Windows
- [x] WINPLAT-4 — without `SDL2main` the programs got their arguments in the Windows ANSI code page, so a path with letters outside it did not open; closed by the UTF-8 manifest of every executable (Windows 10 1903 or later); evidence: Verified behaviour above
