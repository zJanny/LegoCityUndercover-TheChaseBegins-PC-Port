# Closed items of script-system-natives

Done items of [script-system-natives.md](script-system-natives.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] SCRIPT-12 — `PositionObject` and `PositionPlayer` reset the player's fall distance and speed (`script_placement.*`): the player kind byte is 27, so 0x21aae0 is true. Message 8 and the `comp[0x315]` bits: SCRIPT-98. evidence: [script-natives-flags.md §2026-10-14T19:19](../../re/script-natives-flags.md#ts-20261014-191915).
