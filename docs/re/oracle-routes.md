# Runtime oracle: Route scripts and scenario pitfalls

Subtopic of the Azahar runtime oracle. Start with [oracle.md](oracle.md) for setup and the fast workflow. Entries are dated; new entries go at the end.

Scripted routes to a point in the game, and the pitfalls of scenario files (teleport, input that survives a load).

<a id="ts-20261013-124333"></a>

## 2026-10-13T12:43:33+02:00 — Route to gameplay and the gameplay-start checkpoint

- `tools/oracle/to-gameplay.txt` (worker, Haiku; run with `--renderer opengl --display xvfb`, about 340 s from boot) reaches player control: language English (A), StreetPass No (B), HOME notice (A), autosave notice (A), slot 1 New Game (A), intro cutscene runs by itself (~140 s free run), "Doughnut Disaster" card (A). It ends with `savestate gameplay-start`.
- Lead check: `--state gameplay-start --renderer opengl --rpc --rpc-input`: the RPC screenshot shows Chase in the city street with the HUD (portrait, hearts) and the minimap; the player entity starts at world (886.764, 16.589, 1111.166). Stick (0, 150) for 60 stepped frames moved it to (856.937, 16.587, 1098.050), intent heading -18569, held flags 1, controller state byte 255.
- The software renderer crashes in this area (Azahar SIGSEGV in Pica::Texture::LookupTexture, seen three times), so gameplay work uses OpenGL with RPC screenshots.
- Checkpoint files live in artifacts/oracle/states/ (local, not committed); recreate them with the scenario.

Unknowns: the meaning of controller state 255; whether the cutscene timing (free-run waits) is long enough on slower hosts.

<a id="ts-20261013-160530"></a>

## 2026-10-13T16:05:30+02:00 — `teleport` command and scenario pitfalls (falling and jump work)

- `teleport X Y Z` writes the world (node+0x8c) and local (node+0x4c) translation of the player node over RPC (heap writes are allowed). It refuses when world and local differ (the node has a non-trivial parent). It is a measuring aid for placing the player at a ledge or in the air without walking; a standing character on the ground is not probed, so a teleported standing player does not fall until it moves (see player.md falling entry). After `teleport` run at least one `frames` before the first trace row.
- The injected stick and button state of a scenario (`stick`, `hold`) is a variable of the tool and survives `loadstate`: start every scenario with `stick 0 0` and `hold -` after `loadstate`, or a later run starts with the previous scenario's input (this made one of the first recordings a walk instead of a still start).
- A scenario file may chain many `loadstate ... trace` runs; each costs about 5 s for the load plus about 0.5 s per traced frame with `mem`. `--output DIR` may be an existing directory; trace names are relative to it. Writing scenario files to the main checkout's artifacts/ directory with the Write tool is refused for worktree agents; keep scratch scenarios under /tmp.
- Findings: [player.md](player.md).

<a id="ts-20261015-160734"></a>

## 2026-10-15T16:07:34+02:00 — Steering a vehicle in a stepped scenario (VEH-32 traces)

- In the state `drv-police-car-seated`, `stick 156 0` with `hold A` and `memtrace` did not reach the vehicle: the body's steer input (body+0x1bc) stayed 0 for 30 updates. `hold A+RIGHT` and `hold A+LEFT` (the D-pad channels 15 and 14 of 0x29cd9c) steered as expected (steer ±1). Use the D-pad for vehicle steering in stepped scenarios; why the raw circle-pad words written at 0x10a540 do not reach channel 0 of the vehicle input was not looked into.
- Findings: [vehicles-physics.md §2026-10-15T15:58:54](vehicles-physics.md#ts-20261015-155854).
