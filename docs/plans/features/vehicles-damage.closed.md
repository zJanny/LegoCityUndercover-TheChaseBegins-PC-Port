# Closed items of vehicles-damage

Done items of [vehicles-damage.md](vehicles-damage.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] VEH-110 — closed 2026-10-18: the break 0x16cccc, the wreck state 6 (0x1765cc, 0x1d27f4), the return 0x22b224 / 0x1aff24 and the matrix ring 0x307cac are ported; the player stays in the Drive state (message 10 goes to a `TriggerObject` link); the remaining gaps are VEH-120..122. evidence: [vehicles-damage.md §2026-10-18T09:34:10](../../re/vehicles-damage.md#ts-20261018-093410)
- [x] VEH-111 — What broke the oracle car 98 updates after it stopped at the static box: not the water test 0x2d8d08 (false in every update) but the stuck timer of the driven body (more than 3.0 s below 3 m/s with one pedal sets body +0x90 bit 0x40, which 0x23019c turns into a type 6 message with 1000 hearts); measured with state `drv-police-car-seated`, ported with VEH-110. evidence: [vehicles-damage.md §2026-10-18T10:01:07](../../re/vehicles-damage.md#ts-20261018-100107)
