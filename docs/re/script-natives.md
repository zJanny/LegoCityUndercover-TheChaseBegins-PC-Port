# Script natives: census, handlers and work batches

Every native (script function) of the level scripts: handler address, argument count, use in the levels, class and work batch. Script engine itself: [scripts.md](scripts.md). Items: [script-system-natives.md](../plans/features/script-system-natives.md) (SCRIPT-26 to SCRIPT-45). Batch 27 (progress, saves, studs): [script-natives-progress.md](script-natives-progress.md).

<a id="ts-20261014-133339"></a>

## 2026-10-14T13:33:39+02:00 — Census of the 245 natives used by level and class scripts, handler addresses, classes A-D and batches (llvm-objdump on game.arm.elf, C++ level loader; no emulator, no REA session)
Corrected by [script-natives-progress.md §2026-10-14T14:41](script-natives-progress.md#ts-20261014-144155): CountRacesWon is class A (not blocking) and the second argument of EnableStudsByGroup is not an enable flag.
Corrected by [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311): none of the 17 batch 34 natives blocks (all return 1; class C with B? was wrong).
Corrected by [script-natives-animation.md §2026-10-14T21:36](script-natives-animation.md#ts-20261014-213649): PlayCharAnim's handler is 0x29e5f0 and it does not block; PlayCutscene has its own body.
Corrected by [script-natives-flags.md §2026-10-14T14:16](script-natives-flags.md#ts-20261014-141621): the note of PrintDebugString (0x26fde8 is a no-op, not the draw call 0x2665d0) and the classes of the batch 26 rows (what each handler does).

### Method

- **Handlers.** Two sources, which agree for every name found in both. (1) A static table of 16-byte entries {name pointer, handler, info = argument count | types << 8, 0} at 0x359154 (69 entries; the script words and the core natives, for example Wait 0x174604, EnableObject 0x1ddd00, EnableCollision 0x27e9f4). (2) About 290 registration calls `bl 0x23912c` in the startup code (0x15f8f8 and on): `r0 = name string` (pc-relative inline string), `r1 = handler` (literal), `r2` and `r3` = argument count and type byte (they equal the count of `Argument(` lines of `levels/scriptfns.txt` in 247 of 249 comparable cases; exceptions SpawnStuds, SetGravity). 0x23912c itself hashes the name (0x24da28) and stores the handler into the entry of the runtime table whose hash matches (+4), returning -1 when none matches. In the pool layout the handler word sits directly before the aligned name string, which is how the remaining natives were found.
- **Correction.** [Corrects scripts.md §2026-10-13T20:42](scripts.md#ts-20261013-204236): the claim that `scriptfns.txt` lists every native is wrong: it documents 300 of the 377 table entries; the argument types of the others come from the registration values above.
- **Handler ABI** (read in about 25 handlers): `handler(r0 = thread, r1 = array of argument value pointers)`; argument i is `*(r1 + 4 i)`, a float is `*(float*)value`, an object argument is resolved with 0x22dacc (entity pointer; its component pointer is entity+0x7c). Return 1 = done (pc advances), 0 = blocked: the executor calls the handler again every update with thread flag bit 2 set (`ldrh r0,[r0,#20]; tst r0,#4` = "is this a repeat call").
- **Counts.** `Calls` = call sites in the level scripts of the 26 `levels/levels/*.blvl` files that have scripts (30 files in total, 239 distinct names: the number of `progress_report.py`); `lvl_01` = the same in `lvl_01_cherrytreehills_main.blvl`; `1-1` = call sites of the scripts reachable (CallScript / SpawnScript closure) from `startup` plus every trigger script whose name starts with `1-1` in lvl_01 (43 of 314 scripts; static text, not an execution trace); `class files` = call sites in the 40 class `.blvl` files outside `levels/levels/` that have scripts. Six natives occur only there (EffectStart, EffectEnd, EffectAttachParticlesOffset, EffectDisableSpecialObject, ChangeCharTokenTexture, StartFireTruckHose); the progress measure does not count them.
- **Classes.** A = getter / setter / flag on state `chase-game` already has (placed-object state, triggers, bounds, player, save progress, script variables, messages to objects); B = blocking (returns 0 until something ends); C = needs a system that `chase-game` lacks (named in the batch); D = debug only or no call anywhere. The classes come from the name, the argument list of `scriptfns.txt` and a quick look at the handler's size and callees (`n`); only the entries with a text in the Note column had their body read. Everything else is a classification to be confirmed by the agent who implements the batch, and A entries may turn out to need more state than assumed (UnlockCharacter has 104 instructions). A batch agent must read each handler before transcribing it.
- B entries with a note were read in the disassembly; `B?` entries are blocking candidates (a conditional result in the body) not yet confirmed. Natives of batches 28 to 38 listed as C may also block; the executor makes that visible only in the first level that calls them.

### Batches (one agent, one session each; the order is the recommended one)

| Batch | Item | Natives | Class | Calls all / lvl_01 / 1-1 | Needs |
|---|---|---|---|---|---|
| 26 | SCRIPT-26 | object flags and script bookkeeping (22) | A | 1598 / 221 / 37 | - |
| 27 | SCRIPT-27 | progress, save and stud getters / setters (21) | A | 354 / 58 / 12 | - |
| 28 | SCRIPT-28 | DCam core and camera (13) | C, B | 1842 / 317 / 12 | SCRIPT-40 |
| 29 | SCRIPT-29 | DCam cutscenes, credits, camera effects (13) | C, B | 300 / 65 / 10 | SCRIPT-40, MOVIE-2 |
| 30 | SCRIPT-30 | dialogue, tips, mission texts (16) | C | 1985 / 325 / 40 | SCRIPT-41 |
| 31 | SCRIPT-31 | waypoints, objective markers, map, scanner (15) | C | 1232 / 204 / 46 | SCRIPT-41, HUD-11 |
| 32 | SCRIPT-32 | animation and attach (13) | C, B | 1613 / 212 / 23 | ANIM-2 |
| 33 | SCRIPT-33 | sound and music (13) | C | 902 / 110 / 1 | AUDIO-2 |
| 34 | SCRIPT-34 | AI control (17) | C, B | 1051 / 165 / 2 | SCRIPT-42 |
| 35 | SCRIPT-35 | characters, party, weapons, boss hooks (18) | C | 157 / 28 / 5 | SCRIPT-42 |
| 36 | SCRIPT-36 | vehicles and traffic (19) | C | 725 / 91 / 7 | SCRIPT-43 |
| 37 | SCRIPT-37 | platforms and rideables (14) | C, B | 161 / 8 / 0 | SCRIPT-44 |
| 38 | SCRIPT-38 | effects, particles, streaming, semi-transparent lists (20) | C | 553 / 55 / 5 | PART-1 |
| 39 | SCRIPT-39 | hub, shops, minigames (10) | C | 14 / 0 / 0 | SCRIPT-45 |

Sum (at the census): 224 unhandled natives in batches, 20 implemented, 1 stand-in (FadeScreen); batch 26 and 27 changed this, see their entries. The "needs" column names the system whose absence stops the native from doing anything visible; a batch agent can still transcribe the handler's argument handling and state writes, and mark the effect as a stand-in.

### Table

Moved on 2026-10-15 to [script-natives-census.md](script-natives-census.md), where it is kept up to date.

### Natives with no call in any level or class script (class D)

These table entries have a handler (or at least a name) and no call site in the files above; they need no work until a script that uses them is found (the class `.blvl` files outside `levels/levels/` were included; mission and cutscene logic written in C++ may still call the handlers directly). The extraction of names from the startup code produced three garbled strings that are left out.

AIRemoveRangeBound, ActivateObject, Active, AddCharacterToParty, AddCharacterToPartyEnum, AddCharacterToPartyFade, AddFerryMapMarker, AddToSemiTransListx2, AddTreasureHunterObject, AlertMiB, AttachHeadParticles, BendSound, BlockLadderAtTop, BossStunForArrest, CarryResetLoc, ConversationPlay, DetatchFromBone, DuckMusicFor, EffectAttachParticles, EffectAttachParticlesToPath, EffectFadeAlphaThreshold, EffectLinkPathToObject, EffectPlayParticles, ElectrifyObject, EnableObjectWithFX, EnableSubobject, FireProjectile, FireTruckSpray, FlashEffectLight, FlushAnimations, FlushSounds, GetOnFireTruckHose, GetRandomNumber, GetSuperCounter, HideTally, HitObject, HitObjectInBound, HubCharSelect, IncreaseSuperCounter, IsAtWayPoint, IsInBound, IsObjectEnabled, IsRaceWon, IsStealthed, KillPet, MPSync, MakeDynamic, MakeStatic, MakeTransparent, NoCameraFollow, OverrideMusicMood, PanelCamera, PauseAllOtherScripts, PauseMusic, PauseScript, PauseTallyTimer, PickupObject, PlaceObjectFX, PlayCharAnimWithGravity, PlayCharacterSound, PlayCurrentAnimToEnd, PlayEffect, PlayParticles, PositionObjectOffset, PropFaceTarget, RemoveCharacterFromParty, RemoveTreasureHunterObject, ResetAnimation, Return, RideableWalk, SavedOk, ScaleAnimCharNode, ScaleSoundFrequencyGO, Set3DFocusDepth, Set3DFocusPath, Set3DFocusSnap, SetAnimationFrame, SetAnimationPaused, SetAutoOpen, SetCullFlags, SetFOV, SetFog, SetGravity, SetMapDummyLocPath, SetPlatformLooped, SetPlayerSkydiving, SetPropRotating, SetRespawnPoints, SetSilverBossHearts, SetSoundDistances, SetSoundFrequency, SetSoundVolumeGO, SetToppleTarget, ShowTallyTimer, SpawnDebrisTransparent, StartDCamTrackAuto, StartNodelessDCamTrack, StartRain, StartSimplePathCam, Stop, StopEffect, StopFireExtinguisher, TallyTimerWin, TransitionScreenIn, TransitionScreenOut, UnLoadAnim, UseFireExtinguisher, WaitForPlayingAnimFrame, Walnuts_StartPhase3, WhichPlayerInBound

### Unknowns

- Which A entries really are plain flag or state writes (only the handlers with a Note were read); which C entries block. A batch agent confirms each handler body before transcribing it.
- The meaning of the type byte (r3 / info >> 8) beyond the argument count; the runtime table builder that creates the 377 entries before 0x23912c binds the handlers (not located).
- Whether natives are also called by game code outside the script executor (messages to the mission logic); the call-site census only sees scripts.
- The 1-1 closure is static: scripts reachable by trigger events other than the name prefix `1-1` and by class startup scripts (SCRIPT-6) are not in it.

Moved entries:

- [script-natives-flags.md §2026-10-14T14:16](script-natives-flags.md#ts-20261014-141621): batch 26 (SCRIPT-26), the 22 handlers read.
