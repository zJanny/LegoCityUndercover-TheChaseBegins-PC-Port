# Closed items of pedestrians-traffic-cars-junction

Done items of [pedestrians-traffic-cars-junction.md](pedestrians-traffic-cars-junction.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] TRAFFIC-24 — The priority byte +0x19 of junctions and cars is read (128 for both, so the cars update first); the update list's room test and skip bits are left to TRAFFIC-31. evidence: [traffic-junction.md §2026-11-07T18:12:03](../../re/traffic-junction.md#ts-20261107-181203)
- [x] TRAFFIC-25 — The change result of each junction's light cycle reaches `TrafficLightLooks::applyChange` in the junction update, so the lights change colour. evidence: [traffic-junction.md §2026-11-07T18:12:03](../../re/traffic-junction.md#ts-20261107-181203)
- [x] TRAFFIC-S19 — removed 2026-11-07: the cycle's part changes are delivered to the looks. evidence: [traffic-junction.md §2026-11-07T18:12:03](../../re/traffic-junction.md#ts-20261107-181203)
- [x] TRAFFIC-26 — The traffic light's prop update 0x15aa70 was read: it has no physical motion of its own, and `Mass`, `CentreOfMass`, `BuoyancyOffset` and `Buoyancy` are stored and never read by anything the light runs, so there is nothing to port. evidence: [traffic-junction.md §2026-11-08T12:01:44](../../re/traffic-junction.md#ts-20261108-120144)
