# Closed items of pedestrians-traffic-cars-driver-body

Done items of [pedestrians-traffic-cars-driver-body.md](pedestrians-traffic-cars-driver-body.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] TRAFFIC-39 — Done: the kinematic traffic bodies are entries of the vehicle world step (`VehicleWorldBody::kinematic`) so the player's car collides with them, the contact message runs the driver's part of 0x80000002 (request 4, dynamic switch, queue leave, contact time) and `copyInputs` is connected to `copyVehicleRecordInputs`. evidence: [traffic-car-body.md §2026-11-12T08:20:03](../../re/traffic-car-body.md#ts-20261112-082003)
- [x] TRAFFIC-40 — Done: the driver's hearts, health points and forced stop are copied to and from the vehicle record each update, the body readings of state 4 reach the driver and the kinematic wheel spin shows on the wheel nodes; the engine value is TRAFFIC-50. evidence: [traffic-car-body.md §2026-11-12T08:20:03](../../re/traffic-car-body.md#ts-20261112-082003)
