# Closed items of assets-formats

Done items of [assets-formats.md](assets-formats.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] ASSET-5 — `pregen/collectables_level1..7.bin` (71 bytes: version 2, 35 16-bit values in groups of 20, 10 and 5) and `pregen/levelobjectscount.bin` (82 16-bit values) decode in `parseCollectableLists` / `parseLevelObjectCounts` and all 8 shipped files parse exactly; layout from the loader 0x23b498 and the counting reader 0x239184. evidence: [data-tables-config.md §2026-10-16T10:12:03](../../re/data-tables-config.md#ts-20261016-101203).
- [x] ASSET-6 — `levels/scriptfns.txt` and `levels/gescriptfns.txt`: the generic reader 0x23bbe4 ends a value at the first `)` outside braces or at the line end, so a missing `)` (line 843) and an extra `)` (gescriptfns lines 264, 424, 426, 433, 435) are accepted; `parseConfigText` does the same and both files parse. evidence: [data-tables-config.md §2026-10-16T10:12:03](../../re/data-tables-config.md#ts-20261016-101203).
- [x] ASSET-7 — `options.txt` and `options3ds.txt` are not read by this build: none of their keys or the block name occurs in the executable and no path is built for them. evidence: [data-tables-config.md §2026-10-16T10:12:03](../../re/data-tables-config.md#ts-20261016-101203).
