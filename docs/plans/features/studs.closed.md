# Closed items of studs

Done items of [studs.md](studs.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] STUD-2 — The on-screen counter and studs flying to it. Done 2026-10-13 (needs HUD-1, done). evidence: [studs.md §2026-10-13T21:35](../../re/studs.md#ts-20261013-213500), [hud.md](../../re/hud.md).
- [x] STUD-11 — Dropped studs ported: 0x21e2e4 modes 0..2, 0x19913c, 0x180168 and 0x17fc38 for the records 0..19, the request of 0x2149ac; callers in STUD-12, the heart in STUD-13. evidence: [studs.md §2026-10-19T14:02:59](../../re/studs.md#ts-20261019-140259).
- [x] STUD-12 — The first callers: the non-lethal wobble hit 0x2bc370 (`WobbleStudValue`, mode 0) and the finished build-it 0x1aad90 (`StudValue`, mode 1) drop studs; the car break 0x16cccc and the forcing callers of 0x22ca88 drop none. evidence: [studs.md §2026-10-19T14:36:25](../../re/studs.md#ts-20261019-143625).
- [x] STUD-17 — Closed 2026-10-19: the level entry 0x20a4d4 loads the stud counter and the super brick count from slot +0x5148 / +0x5190 (`enterLevel`); both are carried across scene changes and written by every save request. The HUD's display after a load is SAVE-18. evidence: [save-format-state.md §2026-10-19T20:24:11](../../re/save-format-state.md#ts-20261019-202411)
- [x] STUD-18 — The flight of a collected brick (target 6.0 in front of the camera, step (target - p) / (30 x timer)) and the rainbow tint writer 0x2600dc, ported; the spin clock origin is STUD-5. evidence: [super-bricks.md §2026-10-19T20:35:01+02:00](../../re/super-bricks.md#ts-20261019-203501)
