# Closed items of collision

Done items of [collision.md](collision.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] COLL-10 — Vehicles register as collision objects (mask 6, first model item). evidence: [vehicles-system.md §2026-10-14T23:59:31](../../re/vehicles-system.md#ts-20261014-235931).
- [x] COLL-20 — Objects enabled or disabled during play add and remove collision as the original does: the grid keeps every candidate and the owner flags (EnableObject bit 0, EnableCollision 0x200) decide per update. evidence: [collision.md §2026-10-15T07:58](../../re/collision.md#ts-20261015-075843).
- [x] COLL-22 — The harbour view at update 25 is the intro movie `01_GameIntro` of the level start (the run lacked `--no-start-cutscenes`), not the game camera. evidence: [scripts-missions.md §2026-10-15T12:41](../../re/scripts-missions.md#ts-20261015-124140).
