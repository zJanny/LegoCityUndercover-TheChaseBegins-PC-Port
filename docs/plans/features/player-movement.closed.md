# Closed items of player-movement

Done items of [player-movement.md](player-movement.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] PLAYER-9 — The keyboard walk modifier has its own non-3DS action `Walk` (Left Alt) in `config/controls.cfg` (done 2026-10-13).
- [x] PLAYER-4 — Choice among overlapping near-flat triangles for the ground normal: the candidate list order of 0x2092d8 (flag 8 swap-removal, first overlap above / last overlap below) reproduces 133 of 133 recorded normals (done 2026-10-16). evidence: [player-falling-jumping.md §2026-10-16T10:30:18](../../re/player-falling-jumping.md#ts-20261016-103018).
- [x] PLAYER-12 — Attack on foot ported (Y tap, states 0xa3 / 0xa4, hit message 0 to `DonutTable_14` from the m12-table-before spot); the props' reaction is LEVOBJ-86. blocks: LEVOBJ-86. evidence: [player-attack.md §2026-10-18T19:59:57](../../re/player-attack.md#ts-20261018-195957).
- [x] PLAYER-20 — The root step of the combo clips comes from the extra joint curve (joint 21, read in 0x270778 via 0x2327b4), equal to the sub-animation track 0xdcd633e2 used before; the track stand-in is replaced. evidence: [player-attack.md §2026-10-19T08:03:14](../../re/player-attack.md#ts-20261019-080314).
- [x] PLAYER-17 — The attack sounds: the swing (weapon row +0xc, on entering a state) and hit (+0xa, in the hit event) sounds are ported from the tables; both ids are 0 for every character, so the attack is silent. blocks: PLAYER-S11. evidence: [player-attack.md §2026-10-19T12:54:52](../../re/player-attack.md#ts-20261019-125452).
