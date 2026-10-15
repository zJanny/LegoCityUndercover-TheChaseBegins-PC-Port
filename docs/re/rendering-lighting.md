# Fragment lighting: light objects, the light list, per-draw registers and lookup tables

Reverse-engineering findings. Entries are chronological. Addresses are for the Europe v1.0 build. Feature file: [rendering](../plans/features/rendering.md). Related: [materials.md](materials.md), [rendering-fog.md](rendering-fog.md).

<a id="ts-20261015-174725"></a>

## 2026-10-15T17:47:25+02:00 — Fragment lighting from light objects to the PICA registers (llvm-objdump on `artifacts/game/game.arm.elf`, Azahar oracle traps, watchpoints and command-list reads in state `gameplay-start`; REA was not used)

[Corrects materials.md §2026-10-13T22:24](materials.md#ts-20261013-222439): 0x5943d0 holds floats 0..1 that the game sums each frame from the type 0 lights (lvl_01: 136, 159, 179 over 255), not (138, 161, 182); the light list is filled each frame, not only reset. It also resolves that entry's unknown "fragment lighting itself". Oracle runs: own port 24781, home `artifacts/oracle/agentLIGHT-home`, output `artifacts/oracle/agentLIGHT-out` (`l1-trap.txt`, `cmdbuf1.txt`, `l4-*.txt`), OpenGL, RPC, no pokes.

**Light objects** (class setup 0x15dd50, literal pool with the parameter names at 0x15e0fc..0x15e204)
- Every object of the classes `lighting/lightambient`, `lightdirectional`, `lightpoint`, `lightspecular`, `lightspot` builds a light record (0x23a9fc with the name "light"): +0x00 `Colour` x 1/255 (alpha 1), +0x10 `SpecularColour` x 1/255 (a copy of `Colour` when the class lacks it), +0x20 byte `LightType` (a float parameter converted to an integer), +0x24 `Intensity` (default 1), +0x28 `MaxRange` (default 10), +0x2c/+0x30 `OuterCone`/`InnerCone`, +0x34/+0x38 `CamOffsetX/Y`, +0x3c `CastShadows` (only for types 1 and 4). `LightMask` is applied afterwards by 0x23a9f4 (not followed), `LightUpdateType` builds a flicker object (types 1, 3, 4, 5; not followed).
- Class values (class files `levels/lighting/*.blvl`): LightType 0 ambient, 1 directional, 2 point (MaxRange 5, SpecularColour 255), 4 specular; ambient and directional SpecularColour 127. lvl_01 has `Ambient` (Colour 136, 159, 179), `Directional01` (254, 209, 169, orientation (55.3, 40.6, 97.5)) and `psp_specular` (type 4).
- Verified in memory at gameplay-start: the ambient record at 0x08e0b088 (0.5333, 0.6235, 0.7020, type 0), the sun record at 0x08e0b318/0x08e0b380 (0.996, 0.820, 0.663; specular 0.498; type 1; intensity 1; range 10). The sun's node matrix rows equal `placementMatrix` of its orientation to 4 digits.

**Light list** (0x250688 add/remove, per-frame reset 0x10c8a0, registration loop 0x2523cc over the registry 0x52fa44, count at +0x140)
- Type 0: global ambient 0x5943d0 += colour x intensity. Type 1: +0x44 = -(row 2 of the node matrix) (0x11fcec with -1), the direction towards the light. Type 2: +0x44 = the node translation. Types 1 and 2 go into the first free of the 8 slots at 0x5943b0; +0x40 slot, +0x42 = -1 (register cache). Other types are not listed.
- At gameplay-start the registry held exactly the three lvl_01 lights; the list held only the sun.

**Per-draw setup 0x12d884** (called from 0x129684 with the material record and mask = table 0x3798f8 [lighting byte 2]: 0,4,0xf,0,4,0xf,0xf,0xf,0,0xf,0...; 0x12d780 turns lighting on (registers 0x8f = 1, 0x1c6 = 0) when that byte is non-zero)
- Mask bit 0: register 0x1c0 = trunc(global x material +0x10 byte + 0.5) per channel (0x130e0c), clamped 0..255; else 0. Bit k: lights of type k.
- Type 1: 0x142 diffuse = trunc(colour x +0x10 byte x intensity + 0.5), 0x143 ambient = 0, 0x140 specular0 = specular x +0x14 byte x intensity, 0x149 = 1 (directional), 0x14a/0x14b = 0, config1 bit 8+n set (no spot).
- Type 2: 0x142 = 0, 0x143 = the colour value, both colours halved when material byte 0xb bit 5 (x2 scale) is set, 0x149 = 2 (two-sided, positional), 0x14b = 1 / MaxRange (float20) with config1 bit 24+n cleared, else scale 0 and the bit set; config1 bit 8+n cleared (spot on).
- 0x144/0x145: +0x44 times the view matrix 0x52a00c with columns 0 and 2 negated, as float16. 0x1c2 = count - 1, 0x1d9 = identity permutation, 0x1c4 = config1 word 0x37792c. A flag at 0x377929 (0 when read) turns colours into luminance (0.299, 0.587, 0.114 at 0x377938).
- Command list at gameplay-start (0x140037e0, parsed): 0x1c0 = 0x08827cb3 (136, 159, 179), 0x142 = 0x0fe344a9 (254, 209, 169), 0x143 = 0, 0x140 = 0x07f1fc7f (127), 0x144/0x145 = (-0.150, 0.744, -0.651) in view space, 0x149 = 1, 0x1c3 = 0x88000400, 0x1c4 = 0xff7effff. For lighting key 1 draws the list writes 0x1c0 = 0 and no light. No LUT or LUT-input writes per frame.

**Lighting init 0x115850** (once)
- 0x1c3 config0 = [0x377990] = 0x88000400: LUT set 0 (D0, RR, SP, DA), clamp highlights (bit 27). 0x1c4 config1 = 0xff7effff: shadows off, D0 on, D1/FR/RR/RG/RB off, spot and distance per light as above.
- 0x1d0 = 0x200 (absolute input except SP), 0x1d1 = 0x2300 (D0 = N.H, SP = L.N, FR = N.V). 0x1d2 (scale) is not written.
- LUTs built by 0x24cae0 (256 samples at x = i/255; entry = floor(f x 4096) | floor((f(next) - f) x 2048) << 12, value clamped 0..0xfff and difference 0..0x7ff; 0x252c14 is floorf): LUT 0 (D0) pow(x, [0x37798c] = 20) (0x120a44 -> powf), LUTs 16..23 (DA) 1 - x (0x11fbe8), LUT 3 (FR) pow(1 - x, 4) (unused, disabled). LUTs 8..15 (SP) 0.5 + 0.5 x in two signed halves (x = i/127 at entries 0..127, -1 + i/127 at 128..255). All eight light registers start as zero, directional.

**Port** (`src/runtime/lighting.cpp`, `src/render/shaders/scene.frag.glsl`): the registers above per draw, evaluated per fragment as the PICA does (primary = sum((diffuse x max(L.N, 0) + ambient) x DA x SP) + global, secondary = sum(specular0 x D0(N.H) x [L.N > 0] x DA x SP), both clamped), in world space. At the gameplay-start pose the shirt colour of Chase (mean of blue pixels in the crop (160,120)-(240,220)) is oracle (80.9, 112.7, 157.9), port before (62.6, 94.2, 141.5), port after (76.5, 108.1, 153.8). Pictures: `artifacts/review/lighting-r13/`.

Unknowns:
- The writer of 0x37798c (the specular exponent 20 of LUT D0, read in memory).
- Which registered lights are active per room (0x2523cc loop and the registry fill); the port takes lights without a room or in the current room.
- `LightMask` (0x23a9f4) and the objects' `lightexcludemask`; the flicker object of `LightUpdateType`; spot cones (`OuterCone`, `InnerCone`) for type 3.
- 0x1d2 (LUT scale) is never written by the game; 0 (scale 1) assumed. The LUT decode (value / 4096, difference / 2048) and the normal interpolation follow the PICA documentation, not a console measurement; the port interpolates normals, the PICA interpolates quaternions; light vectors are not rounded to float16.
- Whether material +0x10/+0x14 change at run time (the port reads them from the model file; all traced records held ffffffff).
- Point lights (type 2) against the oracle (lvl_00 `Point`/`Point01`, Intensity 2, MaxRange 1).
