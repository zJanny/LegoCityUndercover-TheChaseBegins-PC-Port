# Closed items of pedestrians-traffic

Done items of [pedestrians-traffic.md](pedestrians-traffic.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] PREGEN-1 — The path record, the unit tangent and the room of each point (`deriveSpawnFields`, both kinds). evidence: [pregen-traffic.md §2026-10-20T12:11:03](../../re/pregen-traffic.md#ts-20261020-121103)
- [x] PREGEN-2 — The 72 closed-path pedestrian points off the port's sample are a deviation of the stored position on one axis, not a sampler difference. evidence: [pregen-traffic.md §2026-10-20T12:11:03](../../re/pregen-traffic.md#ts-20261020-121103)
- [x] PREGEN-5 — No path gets a node: the setter 0x1c1b48 is called only by the natives EffectAttachParticlesToPath and EffectLinkPathToObject, which no level or class script calls. evidence: [pedestrians.md §2026-10-20T13:09:29](../../re/pedestrians.md#ts-20261020-130929)
- [x] PREGEN-S3 — removed 2026-10-20: no path gets a node at run time (only two natives no script calls set it), so the plain spline tangent is the original's. evidence: [pedestrians.md §2026-10-20T13:09:29](../../re/pedestrians.md#ts-20261020-130929)
