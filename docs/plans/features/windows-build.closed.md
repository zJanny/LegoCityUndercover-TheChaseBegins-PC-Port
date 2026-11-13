# Closed items of windows-build

Done items of [windows-build.md](windows-build.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] WINBUILD-1 — CMake finds SDL2, epoxy and FFmpeg through `pkg_check_modules` with the MSYS2 packages; `-ffp-contract=off` and the warning flags build clean (verified on Windows, see Verified behaviour); needs: none; blocks: WINBUILD-2, WINPLAT-1, WINPLAT-2, WINTEST-1, WINTEST-2, WINBUILD-3. evidence: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437)
- [x] WINBUILD-2 — Windows build and test commands and the package list are in [building.md](../../building.md#build-on-windows), [commands-windows.md](../../commands-windows.md) and the README; needs: WINBUILD-1; evidence: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437)
- [x] WINBUILD-4 — closed 2026-11-13 by 20261113-winopen-D: `tools/merge_agent.py` finds the MSYS2 tools and Ninja itself (verified behaviour in [windows-build.md](windows-build.md)); the plain command in [commands-windows.md](../../commands-windows.md#merging-an-agent-branch) works
