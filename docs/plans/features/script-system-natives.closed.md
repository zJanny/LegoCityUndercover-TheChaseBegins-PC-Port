# Closed items of script-system-natives

Done items of [script-system-natives.md](script-system-natives.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] SCRIPT-12 — `PositionObject` and `PositionPlayer` reset the player's fall distance and speed (`script_placement.*`): the player kind byte is 27, so 0x21aae0 is true. Message 8 and the `comp[0x315]` bits: SCRIPT-98. evidence: [script-natives-flags.md §2026-10-14T19:19](../../re/script-natives-flags.md#ts-20261014-191915).
- [x] SCRIPT-32 — Batch 7 (13 animation and attach natives): handlers read and run as stand-ins (SCRIPT-S110, rest SCRIPT-112). evidence: [script-natives-animation.md §2026-10-14T21:36](../../re/script-natives-animation.md#ts-20261014-213649).
- [x] SCRIPT-33 — Batch 8 done: music natives are stand-ins (AUDIO-S20, S11), the ten sound natives too (AUDIO-S30..S32). blocks: SCRIPT-7. evidence: [script-natives-audio.md §2026-10-15T17:27](../../re/script-natives-audio.md#ts-20261015-172705).
