# Sound container, CWAV and mixer

Reverse-engineering findings, moved verbatim from the progress log. Entries are chronological; later entries may correct earlier ones. Evidence IDs are shortened with an ellipsis.

## 2026-10-12T15:45+02:00 — Sound container decoded (.bwav)

- The .bwav wrapper starts with u32 version 0x400 or 0x600.
  - v4 (1263 files): one standard CWAV at 0x44.
  - v6 (98 files, all music): one mono CWAV per channel, back to back from 0x4c, with the second CWAV starting exactly where the first ends.
- The other wrapper fields (rate copy, sample/loop counts, a 4-byte value before CWAV that may be a hash) are not interpreted. The CWAV structure is the public BCWAV layout.
- Added tools/convert_audio.py (PCM8/PCM16/DSP-ADPCM/IMA-ADPCM to WAV, v6 parts merged as channels, loop points kept in audio-manifest.json) and tests/test_convert_audio.py (4 tests: DSP frame arithmetic including floor shift, PCM8/16, IMA, two-part merge + WAV output, malformed inputs).
- `python3 tools/convert_audio.py --census artifacts/fib/files` — 1361/1361 parsed; all DSP-ADPCM: 12000 Hz 1068 (+98 two-part), 11999 69, 12001 75; the rest are between 7025 and 12003 Hz.
- `python3 tools/convert_audio.py artifacts/fib/files --output artifacts/audio` — PASS, 1361 WAVs, 0 failures, 40.4 s.
- Checks:
  - The first DSP frame header equals the context pred/scale byte (0x39 in pickup_positive_1_notloop), which confirms data alignment at DATA+0x20.
  - ffmpeg astats: voice-over and music peak at about −0.2 to −2.4 dBFS with RMS between −10 and −23 dB, which reads as normalized audio rather than noise. Zero-crossing rate is 0.06 for the music sample.
  - music/act_01 decodes to silence because its frames are genuinely all zero, so it is a placeholder.
- `python3 -m unittest discover -s tests -p 'test_*.py' -v` — PASS 30/30.

## 2026-10-12T18:25+02:00 — S4a complete (CWAV in C++), S4d mixer and sound test

- Added src/assets/audio.cpp: decodeSound splits the .bwav through parsePooled (0x2c record plus CWAV parts) and decodes PCM8/PCM16/DSP-ADPCM/IMA. The DSP sum is computed in 64-bit and floor-shifted, as in the reference.
- assets-core gains a synthetic DSP vector ([112, −16, −16], matching the Python test) and a malformed-record rejection.
- assets-real-data: 1361 sounds / 122501647 samples / 123 looping, equal to audio-manifest.json. The FNV-1a over act_bass_break's interleaved PCM is 0xdcbf1f85, bit-identical to the Python WAV output.
- Added the chase_audio library (src/audio/mixer.{hpp,cpp}, no SDL): voices with linear resampling, mono→stereo, gain, loop wrap to loopStart, and one-shot expiry.
- Added CTest `audio-mixer` (tests/mixer_test.cpp).
- Added chase-soundtest (SDL audio callback at 48 kHz float stereo). It plays archive sounds and keeps loops going for 10 s by default or `--seconds`.
- Added the opt-in CTest `soundtest-real-smoke` (SDL_AUDIODRIVER=dummy).
- `ctest --test-dir build --output-on-failure` — PASS 7/7; 0 warnings.

## 2026-10-13T09:33:38+02:00 — S4d play-sound entry point (REA Ghidra 12.1.2; llvm-objdump; read-only, partial)

- Binary: artifacts/game/game.arm.elf (sha256 5152ed14…0881). Script command addresses come from local scan of the `bl 0x23912c` registration calls (name via `add r0,pc`, handler via `ldr r1,[pc]`); local evidence, not REA.
- Sound thread: init at 0x103400 creates the thread named `fnaSound` (string 0x1034a8, xref 0x103444, ev_dd91ebb5…), stack 0x1000, entry 0x108ae0, then an object named `sound`. Thread body not read.
- Script commands (registered via 0x23912c; handler = 2nd argument; the args array is `param_2`, with value pointers at +0x0, +0x4, +0xc):
  - `RegisterSound` (name 0x160bc4, handler 0x1cdbdc, ev_efaa1c6c…): `FUN_00281fc4(*(u32*)arg[1] & 0xffff, (int)*(float*)arg[3] & 0xff)`. Appends a (u16 sound id, u8 value) pair to a dedup list at 0x593a60, count at 0x377598. It does not start a voice; it reads as a preload or usage list.
  - `PlaySoundAtObject` (name 0x161260, registered at 0x160ecc, handler 0x27bd48, disassembly only): calls 0x23ae84(sound id = `**(arg[3])`, object = `*arg[0]`).
  - Other handlers found, not read: `PlayCharacterSound` 0x285f94, `SetSoundVolumeGO` 0x1da758, `ScaleSoundFrequencyGO` 0x2915f4, `PauseMusic` 0x1a3020, `StopMusic` 0x195054, `OverrideMusic` 0x1cdb28, `OverrideMusicMood` 0x27bcb0, `AutomaticMusic` 0x1d9c70, `VehicleEnableDrivingMusic` 0x2bf2b4, `VehicleStopSounds` 0x27c148.
- 0x23ae84 (ev_aa513b99…): `FUN_0023fb78(obj, vec12)` gets the object position, then calls the core below with the sound id, that position, the u16 at obj+0x14 and 0.
- Core play function 0x23fa54(id, pos*, u16 param, flag) (ev_90aa6512…). The id is a plain integer, not a name or hash.
  - If `DAT_00376ce5 == 3` and `DAT_00376cf4 != 0`, the id is first remapped through a table of 0x10-byte entries: `id = u16 at (DAT_00376cf4 + id*0x10 + 4)`. Meaning of mode 3 unknown.
  - It walks a linked list of sound banks starting at `DAT_0058974c`. A bank (node+8 points to the bank struct) has: record array at +0x10 with 0x10 bytes per sound id, highest valid id (u16) at +0x20, a bit set of valid ids at +0x24. The first bank whose bit is set plays the sound.
  - `pos == 0` (non-positional) calls 0x23f82c(volume = rec[9]*0.01, bank, id, param, 1). `pos != 0` calls 0x23f940(rec[10], rec[11], bank, id, param, pos).
- Voice start 0x23f82c and 0x23f940 (ev_d1036049…, ev_5a119b87…): allocate a voice with 0x1b4420(bank, id) (handle at voice+8, not read), then use 0x227ff4 (set volume), 0x25c604 (set loop or similar from rec[6] bit 0), 0x227fac (pos/space), 0x227f78 (rec[10], rec[11] as two parameters, likely attenuation range; unverified), 0x227f4c (ducking value from 0x23fc98, when rec[6] bit 3 and `DAT_00376ce6`).
  - Record bytes used: +6 flags (bit0 passed to 0x25c604; bit1 gates the position setup in the non-positional path; bit3 duck; bit4 sets a global state at 0x5862d0..0x5862e4), +9 volume percent, +10 and +11 two byte parameters.
- Shipped table: `tables/tablessoundfx.bxls` (build/chase-tabledump: 758 rows, sheet 0). Each row has a path-like name without extension (for example `ui/menuselect`, `lego_sfx/red_button`), then trailing integers. For the rows checked (1, 5, 9, 21) the last three integers equal record bytes +9/+10/+11 (80/20/30, 100/1/100). That fits the volume percent in the table being the record's volume, and the row index being the sound id used by scripts. Row 0 has an empty name.
- Unknowns:
  - How the bank record array is built from the table and from which .bwav the name resolves to (the archive path under `sounds/`, whether names map directly).
  - Which table column feeds record +6 flags and the 16-bit field at +4, and the real meaning of the two bytes at +10/+11 (listed as range/pitch candidates only).
  - Real names of 0x227ff4, 0x25c604, 0x227fac, 0x227f78, 0x227f4c, 0x1b4420 and the voice object at voice+8, and the sound thread body 0x108ae0 (the actual mixing/streaming).
  - Callers other than script commands (gameplay C++ code, UI, `ATTR_SFX*` attributes), 3D positioning maths, pitch scaling (`ScaleSoundFrequencyGO`), and how music is started.

