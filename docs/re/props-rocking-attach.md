# Attached children, the rocking clock rate and the generator camera calls

Subtopic of [props-rocking.md](props-rocking.md): mission 1-3's attached placed objects, the clock 0x241098 and the generator scene's camera calls.

## Current state

- Attached placed objects (the crow under the girder, the girder under the platform) keep a local matrix and follow the parent's whole world, a repeated AttachObject takes it again; `Rope_01` and emitters have no scene node (SCRIPT-397). [§2026-10-28T00:02:10](#ts-20261028-000210)
- The generator scene's camera eye agrees with the oracle within 0.004 and the path shots' direction within 0.001; the Track shot's look is off by 0.03. [§2026-10-28T00:56:05](#ts-20261028-005605)
- The rocking clock's rate is 30 and its divisor 3000 (about 1 per stepped update); the port's 2 per update stays a stand-in (SCRIPT-S297). The generator scene's camera calls agree within one update. [§2026-10-28T00:40:30](#ts-20261028-004030)

<a id="ts-20261028-000210"></a>

## 2026-10-28T00:02:10+01:00 — Attached placed objects take the parent's rotation (20261027-mission13-E)

Method: port work from the code read in [§2026-10-26T22:49:07](props-rocking.md#ts-20261026-224907) (0x245008 per AttachObject call, 0x24c118, 0x258bc4); no new original reading, no emulator run. Port: `src/runtime/attach_children.*` (`AttachedChildren` over `NodeAttachments`), glue `src/runtime/game_attach_children_glue.cpp`; CTests `attach-children`, `game-generator-1-3` (log lines `prop NAME: attached to PARENT at update N`).

- A placed object that an AttachObject puts under a placed parent (the girder under the crane platform, the crow under the girder; lvl_01 also attaches `1-3_Gardner_Prop*` to the lawnmowers and `1-3_Ledge_Platform_Switch` to the ledge platform) now keeps its local matrix (the world at the attach relative to the parent) and has world = local times the parent's world every update, parents before their children, so it takes the parent's rotation as well as its translation.
- Every AttachObject call numbers the attachment; a repeated call to the same parent takes the local matrix again from the child's current world (0x245008 per call), for level characters too.
- A child that is itself a platform (the crow's own travel) moves by the delta of its own node move, added in world space to the local translation (STAND-IN SCRIPT-S315: the platform's write of the child's world is reduced to a translation delta; the original writes the world through 0x24c118, which also re-derives the local matrix from the parent as of that moment). The follow runs after the platform carry of the update, one step after the characters' follow (STAND-IN SCRIPT-S316: the original order of the entity updates is not read).
- `Rope_01` has no placed scene node in the port (nor have the lawnmower particle emitters and the train station minecart emitters, which attach to placed objects): they stay unattached (SCRIPT-397). The ground follow 0x2b01e4 is still not ported (SCRIPT-398).

Unknowns:
- The clock rate and start of 0x241098 (SCRIPT-384), the camera poses and texts of the generator scene against the oracle (SCRIPT-375): requested as servant job 20261027-mission13-E-1, not answered when this entry was written.
- Whether the original updates an attached child's local matrix when its parent is rocked by 0x28a758 (here the child follows the written world).

<a id="ts-20261028-004030"></a>

## 2026-10-28T00:40:30+01:00 — The rocking clock's rate and divisor and the generator scene's camera calls (RPC 20261027-mission13-E-1, partial)

Method: servant job 20261027-mission13-E-1 (recorded from a save state on the generator trigger spot of mission 1-3 with the generator built; New 3DS, OpenGL, fixed dt 0.0333333; one load, unconfirmed; u0 = the update of the A press, 720 updates). Code trap at 0x241100 (after the `mul` of the clock 0x241098), logging r0, r1, r4, r5; call traps on the script handlers. The port side: the DirectorCamera's busy ranges in the `game-generator-1-3` run, counted from the press at port update 444 (the throwaway log was not kept).

**The clock.** The trap was hit about 6.6 times per update. Per hit r0 = process time times rate, r1 = 30 (the rate [[P+0xc]+0xc]), r5 = 3000 (the divisor, the timer's unit), r4 = P = 0x586018. clock = r0 / r5 read 49377.34 at u0, 49378.33 at u1, 49426.84 at u50, 49568.41 at u193, 49674.34 at u300 and 50070.34 at u700: +0.99 per stepped update. The absolute value is not the clock since the level start (the state was loaded mid-level). Stepped time inflates timers ([oracle.md](oracle.md)), so no free-run rate was measured. A read of the timer at P+0x40 in free run showed the total at +0x10 as 0x4bb72f, 0x4bb865, 0x4bb8bf, 0x4bba63, about one second apart each; not interpreted. So the rate is 30 per unit of the process time and the divisor 3000; with a 3000-per-second unit that is 30 per second, 1 per update at dt 1/30, while the port's stand-in SCRIPT-S297 runs 2 per update. Not changed: the unit of the timer and the free-run rate are unread.

**The camera calls.** Handler hits in the oracle: StartDCamTrack 0x1da810 at u90, ShowDialogueTextTimed 0x2a6d90 at u256 and u562, StartDCam 0x194fd8 at u288, u471 and u624. The port's director camera is busy over u91-287, u289-470, u472-623 and u625-715 (counted the same way), so each shot starts within one update of the oracle's call and the shot lengths agree. The argument slots (paths, snap, time), the camera pose per update and the text keys were not decoded in the oracle (re-requested as 20261027-mission13-E-2).

Unknowns:
- The unit of the clock's timer and the free-run rate of the clock (SCRIPT-384); the camera pose per update at the shots, the decoded call arguments and the text of the second ShowDialogueTextTimed (SCRIPT-375).

<a id="ts-20261028-005605"></a>

## 2026-10-28T00:56:05+01:00 — The generator scene's camera against the oracle (RPC 20261027-mission13-E-2)

Method: servant job 20261027-mission13-E-2 (20261027-mission13-servant, 2026-10-28; recorded from a save state on the generator trigger spot of mission 1-3 with the generator built; New 3DS, OpenGL, fixed dt 0.0333333, A held at u0, two loads). The camera is the game camera object (*0x377544): eye = translation row of its node matrix (+0x5c), forward = its third row. Port side: the director pose of the `game-generator-1-3` run (eye and look point, forward = normalised look minus eye), u0 = port update 444 (the A press, [props-rocking.md §2026-10-26T15:16:58](props-rocking.md#ts-20261026-151658)). 20261027-mission13-E, 2026-10-28.

Oracle (both loads agree within 0.001), eye / forward: u90 938.598 26.770 1133.002 / 0.6998 0.0825 0.7096; u146 938.598 25.944 1133.002 / 0.6734 -0.2877 0.6810; u177 938.598 24.627 1133.002 / 0.6415 -0.4098 0.6485; u240 938.598 21.934 1133.002 / 0.6632 -0.3358 0.6688; u256 938.598 21.624 1133.002 / 0.6681 -0.3160 0.6737; u288 938.598 21.427 1133.002 / 0.6709 -0.3032 0.6767; u340 939.572 21.248 1132.640 / 0.5556 -0.3466 0.7558; u412 943.650 20.498 1131.123 / 0.1681 -0.3291 0.9292; u471 944.790 20.288 1130.698 / 0.0155 -0.3075 0.9514; u624 and u670 958.423 27.987 1091.701 / 0.2138 -0.2575 -0.9423. Rows u209, u520, u562 and u717 differ between the two loads (up to 0.48; u717 is the gameplay camera) and are not used. Player position at u717 in both loads: (942.299, 17.250, 1140.440).

Port against it: the eye agrees within 0.004 on every row (u90 to u670), the forward of the path shots (u340, u412, u471, u520, u562, u624, u670) within 0.001. The Track shot `Dcam_Crane_Lower` (look at the girder, u90 to u256) has a forward off by up to 0.029 (y), -0.009 (x): the look point on the girder (the offset at the entity, [dcam.md](dcam.md) unknowns) is not the node translation (SCRIPT-399). The first row of each new shot (u288) differs by up to 0.058 (the blend between shots).

Call arguments (free run, raw words, slot layout uncertain): StartDCamTrack u90: track hash 0x1d776361, floats 1.0 and 6.5, as in the script. StartDCam u288: hashes 0x94672f6b and 0x6edab359, floats 0, 0, 6.0; u471: 0xd447982a and 0x12dd15a0, floats 2.0 and 5.0; u624: 0x175c7658 and 0xd7223853, floats 2.0 and 3.0. ShowDialogueTextTimed u256: key 0x79e69bdf (2045156319), int 31, time 4.0; u562: key 0x79e79c1f, int 11, time 5.0.

Unknowns:
- The FOV of the shots (not found), the camera names behind the StartDCam hashes and the speaker of the texts, the text of key 0x79e79c1f.
- The look offset on the girder (SCRIPT-399).
