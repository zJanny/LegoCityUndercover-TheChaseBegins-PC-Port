# Closed items of windows-tests

Done items of [windows-tests.md](windows-tests.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] WINTEST-1 — CTest tests that cannot run on Windows skip with a reason in their CTest properties and are listed in Verified behaviour; the CTest set has no Azahar, Xvfb, libX11 or libXtst test; needs: WINBUILD-1; evidence: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437)
- [x] WINTEST-13 — the 112 offscreen OpenGL tests and their 5 picture checks run on Windows with SDL's Windows video driver and a hidden window (`CHASE_HIDDEN_WINDOW`) instead of `SDL_VIDEODRIVER=offscreen` (approach approved by the user 2026-11-12); blocks: WINTEST-2; evidence: [Verified behaviour](windows-tests.md#3-verified-behaviour)
