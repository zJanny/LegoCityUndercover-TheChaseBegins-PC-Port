# Closed items of vehicles-physics-collision

Done items of [vehicles-physics-collision.md](vehicles-physics-collision.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] VEH-71 — The vehicle friction +0xe0 is the constant 0.5 of 0x243f90 (body+0xdac); the restitution matters only for contacts between vehicles (VEH-7). evidence: [vehicles-physics-collision.md §2026-10-15T19:12:47](../../re/vehicles-physics-collision.md#ts-20261015-191247)
- [x] VEH-61 — closed 2026-10-16: the sphere sweep of 0x1c54f8 (0x1b6ef8, 0x16788c, set-up 0x33a4fc); the message of 0x181e68 is VEH-100. evidence: [vehicles-physics-props.md §2026-10-16T09:53:49](../../re/vehicles-physics-props.md#ts-20261016-095349)
- [x] VEH-97 — closed 2026-10-16: the original casts wheel rays through the floor graph and the static Bullet boxes only, so no entity mesh is ever hit. evidence: [vehicles-physics-props.md §2026-10-16T09:53:49](../../re/vehicles-physics-props.md#ts-20261016-095349)
- [x] VEH-90 — closed 2026-10-16: the writer of the flag word is the callback 0x271764 (Bullet's gContactProcessedCallback, stored by 0x2e3bec); the trace's flag word 2 is the driven trooper as body B. evidence: [vehicles-damage.md §2026-10-16T10:21:53](../../re/vehicles-damage.md#ts-20261016-102153)
