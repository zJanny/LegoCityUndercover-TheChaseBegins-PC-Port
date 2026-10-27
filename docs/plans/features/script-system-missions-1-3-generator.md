# Script system: missions, the generator scene timing of mission 1-3

## 1. Scope

The update numbers of the carry placing (parts crate into `1-3_BasePlate`) and of `1-3_Generator_Script` against the oracle. Split from [script-system-missions-1-3.md](script-system-missions-1-3.md) (the garage, the parts crate and the generator scene). The camera shots and texts of the scene are SCRIPT-375 there.

## 2. Status

IN_PROGRESS, updated 2026-10-28T00:41:16+01:00. The placing of a carry object runs as in the oracle: counted from the A press (update P), the player enters the place state at P, releases the object at P+8, the target's state change runs at P+9, the target's script first runs at P+10, the state is 1 again from P+27 (oracle rows of [carrying-animation.md §2026-10-28T00:41:16+01:00](../../re/carrying-animation.md#ts-20261028-004116)). The earlier suspicion that the port ran the script one update early came from numbering one update too high in the first recording (SCRIPT-376, closed). The generator script's own update numbers differ from the oracle only by the lever use (SCRIPT-364).

## 3. Verified behaviour

- Place timing P+8 release, P+10 script, state 1 at P+27 equals the oracle in the port's run (A at update 148 of `game-generator-1-3`: placed 156, script created 157 and first run 158). evidence: [carrying-animation.md §2026-10-28T00:41:16+01:00](../../re/carrying-animation.md#ts-20261028-004116).

## 4. Stand-ins

None.

## 5. Open items

- [ ] SCRIPT-364 — Generator scene against the oracle: from the A press the script starts at +58 (oracle +59), the crow at +90, the rocking stop at +193 and the end at +716 (oracle +717); the one update comes from the lever use and was not re-measured. needs: SCRIPT-368. blocks: SCRIPT-375. evidence: [props-rocking.md §2026-10-26T15:16:58](../../re/props-rocking.md#ts-20261026-151658).

## 6. Findings summary

- The placing rows of the oracle, corrected: [carrying-animation.md §2026-10-28T00:41:16+01:00](../../re/carrying-animation.md#ts-20261028-004116).
- The generator script's call-by-call trace: [props-rocking.md §2026-10-26T15:16:58](../../re/props-rocking.md#ts-20261026-151658).

## 7. Code and tests

- `src/runtime/carry.*` (phase timing), glue `src/game_key_pickup.*` (log line `carry NAME: placed into TARGET at update N`); CTests `carry`, `carry-animation-real-data`, `game-generator-1-3` (slow).
