# Closed items of studs-super-bricks

Done items of [studs-super-bricks.md](studs-super-bricks.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] STUD-22 — Closed 2026-10-25: the camera shift *0x58ef40 is the camera matrix translation that the pickup update 0x180168 copies at its end (0x180c74); the port is STUD-28. evidence: [super-bricks.md §2026-10-25T11:12:12+01:00](../../re/super-bricks.md#ts-20261025-111212)
- [x] STUD-23 — Closed 2026-10-25: the tint phase and the spin start at 0 (static data), only the tint writer 0x2600dc and the HUD callback write them, and no level load resets them (the run carries both); 0x2600dc is the hook at +0x10 of 0x58ef04, called at the end of the pickup update once per game update. evidence: [super-bricks.md §2026-10-25T11:12:12+01:00](../../re/super-bricks.md#ts-20261025-111212)
