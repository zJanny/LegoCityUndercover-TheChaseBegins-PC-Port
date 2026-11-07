# Closed items of pedestrians-traffic-cars-movement

Done items of [pedestrians-traffic-cars-movement.md](pedestrians-traffic-cars-movement.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] TRAFFIC-7 — The kinematic movement of states 2 and 3: the route mover 0x2613ac (with its own request of state 3), the stop test 0x2ca504, the junction choice 0x2cb1e8 and the junction queue 0x22a080. evidence: [traffic-ai-driver.md §2026-11-02T23:44:38](../../re/traffic-ai-driver.md#ts-20261102-234438)
- [x] TRAFFIC-17 — The leave call 0x21d6b8 and the ground probe 0x20ae2c are read and ported (`traffic_leave_snap`). evidence: [traffic-ai-driver.md §2026-11-03T08:37:32](../../re/traffic-ai-driver.md#ts-20261103-083732)
- [x] TRAFFIC-18 — Fill `TrafficJunctionRecord` from the level: done 2026-11-03 by 20261103-large-A (`trafficJunctionFills`: choice table, lane ends, lane paths of length 4 and type 0, light pairs with the lights' routes, the cycle start). evidence: [traffic-junction.md §2026-11-03T08:53:43](../../re/traffic-junction.md#ts-20261103-085343)
- [x] TRAFFIC-S12 — removed 2026-11-03: a lane path is always type 0 (the zero-filled lane record), so the nearest sample of the lane's own type is the type 0 function the original calls. evidence: [traffic-junction.md §2026-11-03T08:53:43](../../re/traffic-junction.md#ts-20261103-085343)
- [x] TRAFFIC-19 — Wire the junction records into the traffic system: keep the fills at the scene enter, run the light cycle for each junction every update, give the route functions the lane paths; done, the update order is read. evidence: [traffic-junction.md §2026-11-07T12:09:40](../../re/traffic-junction.md#ts-20261107-120940)
