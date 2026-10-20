# Closed items of pedestrians-traffic-pedestrians

Done items of [pedestrians-traffic-pedestrians.md](pedestrians-traffic-pedestrians.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] PED-2 — The pedestrian's state step 0x1d7e88, walk 0x2b9138 and avoidance 0x2edf08 are read and ported as `PedestrianWalk` (test `pedestrian-walk`). The walk clip starts in mode 1; mode 6 is the removal's stop. uses: chars/pedestrian. evidence: [pedestrians.md §2026-10-20T12:20:49](../../re/pedestrians.md#ts-20261020-122049).
- [x] PED-3 — The spawn and removal inputs are read: the limit source, the scene enter countdown, the update caller, the node, room and fade tier tests, 0x375d5c and 0x2edf08. What stays open is in PED-7 .. PED-9. evidence: [pedestrians.md §2026-10-20T12:27:05](../../re/pedestrians.md#ts-20261020-122705).
- [x] PED-4 — The steady updates of states 3, 4 and 7 (root motion 0x277afc, wall test 0x2059ec, probe) and of state 5 (0x277c94) are read and ported (test `pedestrian-walk`); their caller-supplied parts are PED-S8. uses: chars/pedestrian. evidence: [pedestrians.md §2026-10-20T13:09:29](../../re/pedestrians.md#ts-20261020-130929).
- [x] PED-6 — The walk inputs are read and ported: the probe floats (0.25, 0.25), the entity box 0x242df4, the draw test 0x2930e0, the vehicle test 0x278b04, path type 1 and the path node (never set). The fade point's writer is PED-13. evidence: [pedestrians.md §2026-10-20T13:09:29](../../re/pedestrians.md#ts-20261020-130929).
- [x] PED-S3 — removed 2026-10-20: `pedestrianLevelPath` samples type 1 paths with 0x1b33fc, and no path gets a node at run time, so no transform applies. evidence: [pedestrians.md §2026-10-20T13:09:29](../../re/pedestrians.md#ts-20261020-130929).
