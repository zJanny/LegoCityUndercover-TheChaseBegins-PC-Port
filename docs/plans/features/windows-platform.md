# Windows: platform files

## 1. Scope

Platform code behind plain interfaces: save directory and native library loading.

## 2. Status

NOT_STARTED. Planned only; nothing verified on Windows.

## 3. Verified behaviour

None yet.

## 4. Stand-ins

None.

## 5. Open items

- [ ] WINPLAT-1 — `src/save/save.cpp` uses `<unistd.h>`, `XDG_DATA_HOME` and `HOME`; add a Windows platform file for `%APPDATA%\\<name>`; needs: WINBUILD-1; evidence: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437)
- [ ] WINPLAT-2 — `tools/chase_native.py` loads `build/libchase_assets_c.so` through ctypes; handle the DLL name and path on Windows; needs: WINBUILD-1; evidence: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437)

## 6. Findings summary

Decisions: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437).

## 7. Code and tests

`src/save/save.cpp`, `tools/chase_native.py`.
