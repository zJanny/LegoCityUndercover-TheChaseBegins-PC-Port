# Closed items of pedestrians-traffic-cars-push

Done items of [pedestrians-traffic-cars-push.md](pedestrians-traffic-cars-push.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] TRAFFIC-54 — Read the mesh test, the oriented test and the world slide that the push vector calls, the turn branch of the mesh path, and the writers of the list of excluded kinds. Done 2026-11-12: ported in `traffic_push_shapes.*`; the turn branch is unreachable, the list has no writer. evidence: [traffic-car-push.md §2026-11-12T10:09:38](../../re/traffic-car-push.md#ts-20261112-100938)
- [x] TRAFFIC-S40 — removed 2026-11-12: the mesh test, the oriented test and the world slide are ported behind the hooks (`connectTrafficPushShapes`), the turn branch is unreachable, and the excluded-kinds list has no writer, so it stays empty. evidence: [traffic-car-push.md §2026-11-12T10:09:38](../../re/traffic-car-push.md#ts-20261112-100938)
