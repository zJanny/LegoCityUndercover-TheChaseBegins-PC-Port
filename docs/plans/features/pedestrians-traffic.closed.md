# Closed items of pedestrians-traffic

Done items of [pedestrians-traffic.md](pedestrians-traffic.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] PREGEN-1 — The path record, the unit tangent and the room of each point (`deriveSpawnFields`, both kinds). evidence: [pregen-traffic.md §2026-10-20T12:11:03](../../re/pregen-traffic.md#ts-20261020-121103)
- [x] PREGEN-2 — The 72 closed-path pedestrian points off the port's sample are a deviation of the stored position on one axis, not a sampler difference. evidence: [pregen-traffic.md §2026-10-20T12:11:03](../../re/pregen-traffic.md#ts-20261020-121103)
- [x] PREGEN-4 — The loaders see the list 0x589a18 as 0x1ed57c rebuilds it just before the scene enter, and the current room of `Player1Start` or of the transition's destination; `chase-game` passes its level and the room streamer's start room. evidence: [pregen-traffic.md §2026-10-20T13:13:47](../../re/pregen-traffic.md#ts-20261020-131347)
- [x] PREGEN-5 — No path gets a node: the setter 0x1c1b48 is called only by the natives EffectAttachParticlesToPath and EffectLinkPathToObject, which no level or class script calls. evidence: [pedestrians-character.md §2026-10-20T13:09:29](../../re/pedestrians-character.md#ts-20261020-130929)
- [x] PREGEN-S3 — removed 2026-10-20: no path gets a node at run time (only two natives no script calls set it), so the plain spline tangent is the original's. evidence: [pedestrians-character.md §2026-10-20T13:09:29](../../re/pedestrians-character.md#ts-20261020-130929)
- [x] PREGEN-3 — The traffic floor cell (0x1edaac(x, y, z, 0.0), point +40) is ported as `findTrafficFloorCell` and `deriveTrafficFloorCells` (closed 2026-10-25, 20261025-traffic-C). evidence: [pregen-traffic.md §2026-10-25T12:36:20](../../re/pregen-traffic.md#ts-20261025-123620)
