# Closed items of script-system-natives-world

Done items of [script-system-natives-world.md](script-system-natives-world.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] SCRIPT-289 — Closed 2026-10-19: 0x23e130 is the only adder of the use-zone registry; 53 functions (create, and post-link for kinds 21, 30, 56, 98, 114) register it for 52 kinds, the gate of ToggleUseable and EnableSwitch follows that list (SCRIPT-S239, SCRIPT-294, SCRIPT-296). evidence: [object-kinds.md §2026-10-19T07:55:07](../../re/object-kinds.md#ts-20261019-075507).
- [x] SCRIPT-290 — Closed 2026-10-19: none of the 13 kinds answers message 24; enableobjectfade ends like EnableObject without message 251 (kinds 27 and 102 answer it: SCRIPT-S241, SCRIPT-295). evidence: [object-kinds.md §2026-10-19T07:55:07](../../re/object-kinds.md#ts-20261019-075507).
- [x] SCRIPT-291 — Closed 2026-10-19: the kind list at 0x3743ec holds the kinds created by 0x23e394 (121, 159, 242); bit 14 comes from the `Boss_IsBoss` parameter in the character setup 0x24166c (SCRIPT-S240, SCRIPT-297). evidence: [object-kinds.md §2026-10-19T07:55:07](../../re/object-kinds.md#ts-20261019-075507).
