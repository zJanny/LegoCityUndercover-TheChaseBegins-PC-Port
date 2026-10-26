# Closed items of script-system-missions-1-3-end

Done items of [script-system-missions-1-3-end.md](script-system-missions-1-3-end.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] SCRIPT-380 — `1-3_Crane_bits` did not build: build-its were selected by the class `props/prop_buildit`; they are now kind 21, so the `leprops/leprop_buildit` objects load. evidence: [buildit.md §2026-10-26T23:06:15](../../re/buildit.md#ts-20261026-230615).
- [x] SCRIPT-381 — The push cart (kind 71) could not be pushed and `1-3_Push_Done` ran only by `--call-script`; the pushable, its handles and the player's handle states 84 to 86 are ported. evidence: [pushables.md §2026-10-26T23:04:40](../../re/pushables.md#ts-20261026-230440).
