# Closed items of vehicles-physics

Done items of [vehicles-physics.md](vehicles-physics.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] VEH-30 — Rigid body orientation and angular part: quaternion step 0x1f2a50 (matrix conventions of 0x232788 and 0x1eb950), angular velocity and torque in 0x1f2eb4 with the inverse world inertia, pitch and roll of the node; the solver part moved to VEH-40. evidence: [vehicles-physics.md §2026-10-15T13:41:02](../../re/vehicles-physics.md#ts-20261015-134102)
- [x] VEH-31 — Wheels and suspension: loader 0x243628, ray 0x33c718, spring and damper 0x339030, force at the wheel centre; traced at rest and while driving (trooper, van). Wheel node pose moved to VEH-43, ray details to VEH-41. evidence: [vehicles-physics.md §2026-10-15T13:41:02](../../re/vehicles-physics.md#ts-20261015-134102)
- [x] VEH-S13 — removed 2026-10-15: the 8.0 ground probe below the body is gone; the wheel rays (VEH-31) find the ground and VEH-S34 limits the body. evidence: [vehicles-physics.md §2026-10-15T13:41:02](../../re/vehicles-physics.md#ts-20261015-134102)
- [x] VEH-S20 — removed 2026-10-15: the stand-in ground contact of the vertical fall is replaced by the suspension of 0x339030. evidence: [vehicles-physics.md §2026-10-15T13:41:02](../../re/vehicles-physics.md#ts-20261015-134102)
