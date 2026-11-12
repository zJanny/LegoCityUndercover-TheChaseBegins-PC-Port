# Windows: tests and tools

## 1. Scope

Running all tests and tools on Windows; skips with reasons.

## 2. Status

NOT_STARTED. Planned only; nothing verified on Windows.

## 3. Verified behaviour

None yet.

## 4. Stand-ins

None.

## 5. Open items

- [ ] WINTEST-1 — Linux-only tests (Azahar oracle, Xvfb, libX11, libXtst) skip with a reason in their CTest properties and are listed here; needs: WINBUILD-1; evidence: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437)
- [ ] WINTEST-2 — Check floating-point equality of the pinned-value tests with MSYS2 GCC and `-ffp-contract=off`; needs: WINBUILD-1; evidence: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437)

## 6. Findings summary

Decisions: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437).

## 7. Code and tests

`tests/`, `CMakeLists.txt`.
