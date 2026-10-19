# Closed items of abilities-scanner

Done items of [abilities-scanner.md](abilities-scanner.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] SCAN-4 — Real-time transition counts, lock / scan / exit timing and the hang: measured with the motion source, ported (test `scanner`); the B exit was not measured (SCAN-7). evidence: [abilities-scanner.md §2026-10-19T08:43:45](../../re/abilities-scanner.md#ts-20261019-084345)
- [x] SCAN-1 — Scanner view look: frame, zoom meter, headphone icon, reticle and scan meter ported from the layouts and the reticle update; the rest is SCAN-8 and SCAN-9. evidence: [abilities-scanner.md §2026-10-19T08:53:30](../../re/abilities-scanner.md#ts-20261019-085330)
- [x] SCAN-7 — Exit by B (slot 7) in real time: ends 57 updates after the press, like the script exit (oracle B-1 of round 28); the exit callback count stays SCAN-S1. evidence: [abilities-scanner.md §2026-10-19T10:01:16](../../re/abilities-scanner.md#ts-20261019-100116)
- [x] SCAN-8 — The cyan wash over the 3D view: the material override 0x530834 of the audio mode, ported (test `scanner-render`); the "green dog" was the 2D spinner, the dog is behind a wall; the glow pass is RENDER-68. evidence: [abilities-scanner.md §2026-10-19T14:06:22](../../re/abilities-scanner.md#ts-20261019-140622)
- [x] SCAN-9 — Remaining overlay parts: `TargetAquired_On`, `ScanButtonPrompt_*` and `WAV*` are never started by the code (nothing to draw); info boxes and frame tint read and split off as SCAN-12 / SCAN-13. evidence: [abilities-scanner.md §2026-10-19T14:13:26](../../re/abilities-scanner.md#ts-20261019-141326)
- [x] SCAN-14 — Chase holds no model while using the scanner (user playtest); the scanner's hand item is not shown. evidence: [abilities-scanner.md §2026-10-19T18:21:29](../../re/abilities-scanner.md#ts-20261019-182129).
