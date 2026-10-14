# Runtime oracle: Measuring methods

Subtopic of the Azahar runtime oracle. Start with [oracle.md](oracle.md) for setup and the fast workflow. Entries are dated; new entries go at the end.

Watchpoints, execution traps, fixed dt, memory traces, poke, and the limits of traps.

<a id="ts-20261013-134500"></a>

## 2026-10-13T13:45:00+02:00 — Watchpoints, execution traps, fixed dt and memory traces (locomotion work)

- `watch ADDR LEN N NAME [KIND]` sets a GDB watchpoint (Z2 write, Z3 read, Z4 access; the stub supports them, `gdbstub.cpp` `CommitBreakpoint`) and advances N frames. `trap ADDR[,ADDR] N NAME` does the same with execution breakpoints. Both write registers r0-r15, 32 stack words and 64 bytes at each pointer-like r0..r3 of every stop to `NAME.txt`. The stub reports `watch:ADDR;` (or `awatch`) in the stop reply; the Azahar log warns that dynarmic has no accurate watchpoints: the reported pc is at a block boundary, not the accessing instruction. Use a trap on candidate functions to confirm.
- `dt MIN MAX` writes the scheduler's dt limits (`C+0x10`, `C+0x14`, `C` = `*(P+0xc)`); with MIN == MAX each update gets exactly that step (the step is quantised to the 1 ms game clock: 0.0165 became 0.0163). Stepped runs otherwise get dt = max (0.07) on every update because the stub stops take longer than that; free-running games use about 0.033.
- `trace N NAME mem` also writes `NAME.mem`: per frame u32 frame, entity (0x100 bytes), controller (0x340) and node (0xb0).
- Bug fixed: `loadstate` with `--attach` did nothing (the quick-load key reached no window because the main window had no keyboard focus in the new X connection) while printing "loaded". The tool now focuses the main window before every quick save and quick load. Check a load by reading `player` afterwards.
- Timings (OpenGL, RPC, stepped): `trace` about 0.5 s per frame with memory dumps, `loadstate` plus 3 frames about 5 s with `--attach`.
- Findings from these tools: [player.md](player.md) (2026-10-13T13:42:20+02:00).

<a id="ts-20261013-144637"></a>

## 2026-10-13T14:46:37+02:00 — Trap limits seen while tracing the collision code

- `trap` dumps 64 bytes at every pointer-like register r0..r3 of every stop. A trap list that includes a function called with an unmapped pointer-like value (here 0x24c118, called for every node of the scene) aborts the whole run with `memory read failed at 0x5910100`. Trap single player-specific functions instead (0x20cd30, 0x21a73c, 0x20b48c, 0x1f9884, 0x2023e4 all worked).
- A trap of a function that other entities also call (0x2023e4: pedestrians, vehicles) stops for them too; the first stop of a frame was the player's in the checkpoint (confirmed by its box centre y and half extents against the player's height and root delta).
- The frame numbers of a `trap` run and of a `trace` run are not aligned across two runs from the same state (the first trap frame was about 6 updates earlier than the matching trace row in one pair of runs); compare values only inside one run. The mem trace record is 4 + 0x100 + 0x340 + 0xb0 = 1268 bytes per frame; older traces of 692 bytes per frame (loco-out push150_mem, mag*, turn*) have a shorter controller dump and do not hold ctrl+0x224.
- Findings: [collision.md](collision.md).

<a id="ts-20261013-211800"></a>

## 2026-10-13T21:18:00+02:00 — `poke` command (door work)

- `poke ADDR HEXBYTES` writes bytes at ADDR through the GDB stub (`M` packet; the stub clears the instruction cache). It is a measuring aid to send an object a request that its script would send, for example the target state of a door (record+4 = 2 opens it; docs/re/doors.md). A poked state is never saved as a reference state: the traces were taken from `gameplay-start` after a `loadstate`, the poke followed it in the same scenario, and every scenario starts with `loadstate`, which discards the poke.
- Pitfall: a scenario killed during a long `trace` (a `pkill` of the tool) took the emulator with it once; use `--stop` and relaunch with `--state`.
- A long stepped `trace` with the OpenGL renderer and `stick` input took about 3 s per update in this session (`frames 90` plus a 30-update trace for each of two runs: 394 s).
