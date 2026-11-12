# Windows: build

## 1. Scope

MSYS2 UCRT64 GCC build with Ninja: dependency discovery and flags.

## 2. Status

NOT_STARTED. Planned only; nothing verified on Windows.

## 3. Verified behaviour

None yet.

## 4. Stand-ins

None.

## 5. Open items

- [ ] WINBUILD-1 — CMake finds SDL2, epoxy and FFmpeg only through `pkg_check_modules`; check it works with MSYS2 package manager and that `-ffp-contract=off` and the warning flags build clean; needs: none; blocks: WINBUILD-2, WINPLAT-1, WINPLAT-2, WINTEST-1, WINTEST-2, WINBUILD-3. evidence: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437)
- [ ] WINBUILD-2 — Windows build and test commands and the package list in `docs/commands.md` and `docs/building.md`; needs: WINBUILD-1; evidence: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437)
- [ ] WINBUILD-3 — Windows job in `.github/workflows/ci.yml` (`msys2/setup-msys2@v2`, UCRT64, Ninja, `ctest -LE slow`), added once the first local Windows build passes; render tests need Mesa llvmpipe or a skip, game-data tests skip; needs: WINBUILD-1; evidence: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437)

## 6. Findings summary

Decisions: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437).

## 7. Code and tests

`CMakeLists.txt`, `cmake/`.
