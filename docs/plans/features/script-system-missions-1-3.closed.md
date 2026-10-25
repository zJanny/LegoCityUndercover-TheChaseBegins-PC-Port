# Closed items of script-system-missions-1-3

Done items of [script-system-missions-1-3.md](script-system-missions-1-3.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] SCRIPT-361 — closed 2026-10-25: the key of mission 1-3 had no use zone (its mesh is a class parameter) and the keyhole was not a carry target; both are loaded by ObjectType now, the pickup and its carry arrows follow the oracle. Rest of the mission: SCRIPT-362 to SCRIPT-364. evidence: [carrying.md §2026-10-25T14:37:21](../../re/carrying.md#ts-20261025-143721).
- [x] SCRIPT-363 — Parts crate: `1-3_Lift_Crate_01` names `1-3_Got_parts`, which enables the big carry object `1-3_Generator_Parts_Carry`, carried to `1-3_BasePlate` (`TriggerObject` `1-3_Hidden_Exhaust_Carry_Target_Script`, `PlacedObject` the build-it `1-3_Pull_Switch_Bits`); runs in `chase-game` from the crate (the build-it is hidden until the placing); the walk through the garage door needs SCRIPT-362. blocks: SCRIPT-364. evidence: [carrying.md §2026-10-25T16:20:07](../../re/carrying.md#ts-20261025-162007).
