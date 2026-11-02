# Closed items of player-movement-attack

Done items of [player-movement-attack.md](player-movement-attack.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] PLAYER-14 — Characters are attack targets now (gate 0x21a2d4, team test 0x219ba0, weights 0x1b7358, the grab test and the fight move choice); the line of sight is PLAYER-27, the remaining character facts PLAYER-29 (done 2026-11-01).
- [x] PLAYER-27 — The attack's line of sight 0x219990: the ray between the box centres with the query words include 5 / exclude 0x10, clear when the nearest entry is the target's (`attackLineClear`, `attack-facts`; done 2026-11-02). evidence: [player-attack-characters.md §2026-11-02T15:32:44](../../re/player-attack-characters.md#ts-20261102-153244).
- [x] PLAYER-29 — The near list (`markAttackNearList`) and the characters' entity boxes (the class's `CollisionBound` and `BoundCollision`, `characterEntityBox`); the rest of the facts are PLAYER-30..32 (done 2026-11-02). evidence: [player-attack-characters.md §2026-11-02T15:32:44](../../re/player-attack-characters.md#ts-20261102-153244).
