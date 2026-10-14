# Runtime oracle: RPC channel

Subtopic of the Azahar runtime oracle. Start with [oracle.md](oracle.md) for setup and the fast workflow. Entries are dated; new entries go at the end.

The Azahar RPC server (UDP): memory reads and writes, screenshots, pad input without breakpoints, timings.

<a id="ts-20261013-120221"></a>

## 2026-10-13T12:02:21+02:00 — RPC channel (non-halting memory access, breakpoint-free pad input)

Azahar 2126.2 has an RPC server (source: `src/core/rpc/` of the Azahar source, `/usr/src/debug/azahar/azahar-unified-source-2126.2/`). The tool uses it as a second channel next to the GDB stub: `--rpc` (reads, screenshots, `sample`, `perf`) and `--rpc-input` (pad buttons without per-frame breakpoints). Tests of the client: `tests/test_azahar_oracle_rpc.py` (fake UDP server).

**Enabling**
- Setting `enable_rpc_server`, section `[Debugging]` (`citra_qt/configuration/config.cpp` `ReadDebuggingValues`). The generated config carries `enable_rpc_server=true` and `enable_rpc_server\default=false`. `core/core.cpp` creates the server at the end of `System::Init` only if the setting is on (and `ENABLE_SCRIPTING` was compiled in; the Arch build was).
- The UDP port is hard-coded: `udp_server.cpp` binds `0.0.0.0:45987` ("TODO: Make configurable"). Not configurable, so two emulators cannot both have RPC. If the bind fails, `Server` catches the exception, logs "Error starting UDP server" and runs without RPC, with no other sign. The tool therefore refuses `--rpc` when UDP 45987 cannot be bound by the tool itself, and `--rpc` is off by default. A running RPC emulator also answers any client on that port: do not attach a second tool instance to the wrong emulator.
- The log line in `azahar_log.txt` says `Starting RPC server (synchronous)`: the build has `ENABLE_SCRIPTING_SYNC`. Every request except `ReadScreenshot` is queued and executed by the emulation thread (`ProcessCoreRequests`, `core/core.cpp`), so reads are consistent (no torn words) and it works while the GDB stub has the CPU halted (verified).

**Packet format** (`packet.h`, `udp_server.cpp`, `rpc_server.cpp`)
- One UDP datagram per request and per reply: header of four little-endian u32 `version (=2), id, type, size`, then `size` payload bytes. The datagram length must be exactly 16 + size, else it is dropped without reply (no reply at all). Request size is at most 16 + 32768. The reply echoes version, id and type with its own size.
- A request with size < 8, a wrong version or an unknown type gets an empty reply (size 0). Every request type has two u32 arguments first.
- Types: 1 ReadMemory (addr, size; size 1..32768; reply = the bytes), 2 WriteMemory (addr, size, then data; size 1..32760; reply is empty), 3 ProcessList (start, max; reply u32 count + count * 20-byte packed `{u32 pid, u64 title, char name[8]}`), 4 SetGetProcess (op 0 = get, reply u32 selected pid; otherwise set pid), 5 TakeScreenshot (res_scale, flags; flags bit0 secondary window, bit1 raw RGB; reply `{u32 result, width, height, data_size}`), 6 ReadScreenshot (offset, size; up to 32768), 7 GetPerfStats (mode 0 last / 1 get-and-reset; reply 10 f64 + u32 = 0x54 bytes).
- Reads and writes use the selected process (default `0xFFFFFFFF` = whatever `Memory()` currently maps, logged as an error). The tool selects the application process (title high word 0x00040000, here pid 11, `LEGOCITY`) after connecting.
- A reply shorter than requested happens only when the selected process does not exist (size 0). Reads of unmapped addresses are not reported as errors by the protocol (see unknowns).
- WriteMemory is accepted only when the start address is inside the process image (0x100000..), heap (0x08000000..0x10000000), linear heap (0x14000000..0x1c000000) or New 3DS extra RAM; otherwise it is silently ignored, and the reply is the same empty reply as for success. The tool refuses such writes locally and verifies every write by reading it back. Writes to shared memory (0x10000000) and VRAM cannot be done over RPC.
- A successful write calls `System::InvalidateCacheRange`, which invalidates the JIT (`ARM_Dynarmic::InvalidateCacheRange`). GDB memory writes (`M` packet) call `ClearAllInstructionCache` for the same reason.
- `TakeScreenshot` returns `Unsupported` with the software renderer (`rpc_server.cpp`). With OpenGL it asks the renderer for a frame, so the emulation must be running (a halted CPU renders no frame and the request times out after 10 s). The PNG is a combined top-over-bottom layout, 400x480 at scale 1.

**Pad input without breakpoints (`--rpc-input`, verified)**
- **Correction (earlier version of this entry, commit 319610e):** the first design used the zero word at 0x10acf4 as the mailbox. That was wrong: `10ac94: eddf1a16 vldr s3, [pc, #88] @ 0x10acf4` loads it as the float constant 0.0, so writing the mask there corrupted that function whenever a button was held. A scan for absolute addresses cannot see pc-relative loads.
- The pad reader loads the held mask with `ldr r0,[r4]` at 0x10a578 (then `bic r0,r0,#0x2000` at 0x10a57c). The tool patches 0x10a578 to `b 0x356580` (`ea093000`) and writes a code cave: `0x356580 ldr r0,[pc]` (`e59f0000`, reads the mailbox), `0x356584 b 0x10a57c` (`eaf6cffc`), `0x356588` the mailbox word (held mask). `hold` writes the mailbox over RPC. The RPC write invalidates the JIT, so the patch takes effect on the next execution of the block. Encodings checked with `llvm-objdump` on a patched copy of `game.arm.elf`: `10a578: b 0x356580`, `356580: ldr r0,[pc] @ 0x356588`, `356584: b 0x10a57c`.
- Cave choice: the text segment is 0x100000..0x357000 (599 pages); `.code` text size is 2450812 bytes, so the last used byte is 0x35657b (`bx lr` at 0x356578) and 0x35657c..0x356fff is zero padding (2692 bytes). Evidence that 0x356580..0x356fff is unreferenced (script run against `artifacts/game/game.arm.elf` and `code.bin`): (1) the whole `llvm-objdump` disassembly (612545 lines): 19152 pc-relative `@ 0xADDR` comment lines and 90291 branch lines, none targeting the range; (2) an independent decode of every text word for pc-relative `ldr/str`, `ldrh/ldrd`, `vldr`, `add/sub rX,pc,#` and `b/bl` targets: none in the range; (3) every 32-bit word of `code.bin` (any alignment) with a value in 0x356500..0x357000: only one unaligned match (0x22dd8f, `0x356ee5`), which straddles the bytes of `ldr r0,[r0,#4]` and `bl 0x23b350` at 0x22dd8c/0x22dd90, not a pointer. The cave is cleared (zeroed) again on exit.
- Verified at the language screen: `hold A` + free run gives pad channel 16 = 1.0 and nothing else; `hold UP+B` gives channels 12 and 17 = 1.0; `hold -` gives zeros. It also works stepped (`frames 5` after `loadstate`). A loaded save state brings back the memory of the state, so the patch is re-applied after `loadstate` (re-checked after the cave change: after `loadstate` the instruction is `ea093000`, the cave `e59f0000 eaf6cffc <mask>`, and `frames 5` shows ch16 = 1.0). A later `--attach` reads the original `e5940000` at 0x10a578 and zeros in the cave: the patch is removed on exit.
- The tool restores the original instruction before leaving a kept emulator (otherwise the last mask would stay pressed for later attaches); `--stop`/kill needs no restore. The patch replaces the held mask entirely, host input is not combined.
- The circle pad is not covered: the raw words at r4+12/+14 are consumed at 0x10a540 as halfword loads, which cannot load a full word from the cave without a longer stub. `stick` still installs the per-frame breakpoint at 0x10a540 (only then, and the held mask stays breakpoint-free).

**Measurements** (this machine, software renderer unless noted, old 3DS, `--frame-limit 100`, state `fast-lang` at the language screen)
| Item | Time or value |
|---|---|
| Screenshot via RPC memory reads (software renderer, stepped/halted) | 0.05 s (identical PNGs to the GDB route) |
| Screenshot via GDB stub (same two screens, `screenshot NAME gdb`) | 10.7 s |
| Screenshot via RPC `TakeScreenshot` (OpenGL renderer, game runs during capture) | 0.18-0.20 s |
| RPC read round trip, language screen | about 0.03-0.1 ms per request (5 requests per `sample` row: 0.13-0.55 ms) |
| RPC read round trip, in-game dialog state (`agent-intro-dialog`) | about 1.7 ms per request (8-26 ms per `sample` row) |
| `sample 20 0.1` free run, language screen | 50.14 clock ticks per vblank, 60.2 vblanks/s (both counters 0x593d14/0x593d18 advance together) |
| Same, OpenGL renderer, `sample 10` | 50.14 ticks per vblank, 60.6 vblanks/s |
| Same, `agent-intro-dialog` state | 51.13 ticks per vblank |
| Stepped `frames 300` (breakpoint per frame), language screen | 65.5 ticks per vblank (0x593d14), 68.0 (0x593d18); 107 s wall |

The free-run rate of 50.14 ticks per vblank is the undistorted figure; the earlier 51.3 (see above) probably included the stub stops that bracket a `wait` interval; not isolated. Stepping inflates the clock by about 31 % even when the memory reads go through RPC, because the inflation comes from the halts.

**Limitations**
- RPC uses fixed UDP port 45987: one RPC emulator per machine. Port clashes are silent on the emulator side.
- Software renderer: no RPC screenshot request (frames only exist in emulated VRAM, read them with RPC memory reads). OpenGL: faithful timing and `TakeScreenshot` works, but the capture is taken from a running game, not a stepped frame, and VRAM reads are stale.
- RPC cannot set breakpoints, read registers or stop the CPU: stepped frame counting (`frames`, `trace`) still needs the GDB stub.
- `sample` reads about 5 values per row non-atomically (separate requests, so the game may advance a frame between them); rows carry host-side `t` as the midpoint of the row.
- Writes are limited to the regions above.

Unknowns:
- What an RPC read of an unmapped guest address returns (the code path was not exercised).
- Whether `ENABLE_SCRIPTING_SYNC` ordering guarantees (a request executes at a defined point of the emulation loop, e.g. at the start of a timeslice) matter for sampling alignment against vblank; not measured.
- Why the RPC round trip is 50 times slower in the dialog state than at the language screen (likely the emulation loop services pending work less often under load); not investigated.
- Behaviour of RPC writes into a JIT block that is executing at that moment on the second core (old 3DS has only core 0, so none seen).
- Circle-pad injection without a breakpoint (needs a patch of 0x10a540 and 0x10a550 to load full words from spare RAM, or a stub in the code cave).

<a id="ts-20261014-214240"></a>
<a id="ts-20261014-214240"></a>

## 2026-10-14T21:42:40+02:00 — Breakpoint-free circle pad, wait for a byte, screenshot loop (`tools/azahar_oracle.py`; llvm-objdump on `artifacts/game/game.arm.elf`, oracle run on `street-at-entrance`; REA was not used)

Resolves the last unknown of [§2026-10-13T12:02:21](#ts-20261013-120221) (circle pad without a breakpoint). The raw words are read at 0x10a540 (`ldrsh r0, [r4, #14]`, y) and 0x10a550 (`ldrsh r3, [r4, #12]`, x). `freestick X Y` (needs `--rpc-input`) replaces 0x10a540 by a branch to a cave at 0x3565a0 (zero padding after the button cave) that stores the two mailbox words 0x3565d0 (y) and 0x3565d4 (x) into `r4+14` and `r4+12`, executes the replaced `ldrsh` and branches back to 0x10a544. The words were checked with `llvm-mc -triple=armv6k` (pinned in `tests/test_azahar_oracle_stick.py`). The game keeps running freely, so a walk takes real guest time; the stepped `stick` (breakpoint per frame) inflates the clock about 30 times per update. The patch is removed when a kept emulator is left and after `loadstate`.

New scenario commands: `waitbyte ADDR VALUE SECONDS` (free-run, poll by RPC, halt when the byte matches) and `shotloop NAME SECONDS ADDR[,ADDR...] [VBLANKS:X:Y | VBLANKS:X:Y:Z ...]` (OpenGL only: RPC screenshots back to back, about 14 per second, with the vblank counter, the bytes at the addresses and the camera eye and look and player position per shot in `NAME_log.csv`; items set the stick or teleport after that many vblanks). A teleport of the player node across the Room7/Room8 box did not change the current room in 6 s (cause not found).

Unknowns:
- Why the teleport does not trigger the room change that a walk does.
- The software renderer crashes in `Pica::Texture::LookupTexture` on the street scene (three launches), so `shotloop` is OpenGL only.
