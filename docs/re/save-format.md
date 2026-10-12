# Save format

Reverse-engineering findings, moved verbatim from the progress log. Entries are chronological; later entries may correct earlier ones. Evidence IDs are shortened with an ellipsis.

## 2026-10-13T10:30+02:00 — S4f save format recovered (REA, Ghidra 12.1.2; read-only)

- Path "data:/savegame.dat" (0x36741c). 0x24bc0c mounts "data:" and opens the file with mode 7, read/write/create (ev_7bc912c1…, ev_7004d64d…). 0x24baa0 maps FS results to statuses 1/4/5; 4 = corrupted and 5 = not formatted are inferred.
- I/O runs on "saveiothread" 0x1688cc (ev_b8f03e66…, ev_56a85bfb…), by mode:
  - 1: probe (0x2a7928).
  - 2: load (0x29ff4c), a whole-buffer read; a short read gives status 3.
  - 3: save (0x1166f4), a write with flush, then 0x120a58("data:"), inferred to be the commit (ev_7b43e130…).
  - 5: 0x2ae218, probably format (inferred).
- Save manager: context at *0x379978 (0x15baa8, 0x2281c8, 0x228058, 0x228044); the buffer is set by 0x228244 (ev_0c1844a8…).
- Layout descriptor from 0x1a1fb8 → 0x1569d8 → 0x272948 (ev_56a85bfb…, ev_3efeec49…): options 0x21c bytes, slot 0x5230 bytes, 2 slots, version 0x1e. File size 0xa694 (42644 bytes), little-endian:
  - 0x00 u32 CRC, 0x04 u16 size, 0x06 u16 version 30, 0x08 "LEGO_ENGINE_SAVE" (16 bytes, no NUL). The header is written by 0x1fc604 (ev_de1097ad…).
  - 0x18: options block, a raw copy of live 0x51b490.
  - 0x234: slot 0. 0x5464: slot 1. Each is a raw copy of live 0x51b6b0 (0x1d0ba0, 0x1d0b80 save; 0x1c3478 load).
- CRC (0x248308, ev_3aed9451…, table ev_712d1c1c…): reflected polynomial 0xedb88320, initial value 0xffffffff, no final XOR, computed over [0x18, end). This equals ~zlib.crc32.
- Load check 0x1d0acc (ev_3b887da8…, ev_2d9d94d6…): a bad magic gives status 3; a CRC, size or version mismatch gives 4. Either resets the data to defaults (0x2081cc).
- Options fields:
  - +2 is the last-used slot (0xff = none).
  - +4..+7 hold defaults 10 and 8, inferred to be music/SFX volume (the UI has Music, SFX, InvertCamera and Language).
  - +0x1ec holds 2 × 0x18 slot summaries (inferred).
- Slot fields:
  - +0x07 bit 2 marks the slot as in use (inferred).
  - +0x5148 is a u64 stud count (inferred: a cheat sets 10000000).
  - +0x5190 is a u32 stud total (inferred).
  - +0x51ee is a 64-bit field cleared from the table at 0x372cb8 (meaning unknown).
- options.txt and options3ds.txt are engine logging configuration, not user settings; their loader is unknown. User settings live only in the options block.
- Unknown: story progress, unlocks and collectibles inside the slot; how the options block is restored on load; exact FS commands behind 0x120a58 and 0x325a04. The defaults callback 0x1a25cc unlocks character IDs 9..140 (table 0x372e10, stride 0x28); how these IDs relate to the 192 tablescharacters rows is unknown.
- Proposed next step (not started): chase_save under src/save/ (no SDL). It keeps the image as opaque bytes with header, CRC and validate (statuses 3/4), stores it at $XDG_DATA_HOME/chase/savegame.dat with an atomic write (temp file, fsync, rename), and has synthetic tests only (no real save is available).


## 2026-10-13T12:10+02:00 — Header size and version sources (REA, Ghidra 12.1.2; read-only)

- Confirms the 10:30 entry. Descriptor builder 0x272948 (ev_ff261669…) stores the running total 0x18 + options + slots × slot size at descriptor+0x1c. The header writer 0x1fc604 (ev_60bfa19c…) writes that total, truncated to u16, at file +4, and descriptor+10 (version) at +6. So the size field holds 0xa694, the whole file size.
- Load check 0x1d0acc (ev_53486e92…) compares the u16 at +4 with descriptor+0x1c and the u16 at +6 with descriptor+10. The magic check counts matching bytes over all 16, so a partial match still fails.
- 0x272948 also formats "Profile data [slot %u]" per slot; the label's use is unknown.
- Implemented as chase_save (src/save/), opaque blocks only. Unknowns: the defaults written by 0x2081cc, so a reset image is zero-filled rather than the game's defaults.

## 2026-10-13T12:30+02:00 — Reset defaults 0x2081cc (REA, Ghidra 12.1.2; read-only)

- Correction to the 10:30 entry: +4..+7 are two per-slot pairs, not one music/SFX pair. The descriptor from 0x1a1fb8 (ev_c8c0b40e…) holds sizes, slot count 2, version 0x1e, the slot callback 0x1a25cc and the options callback 0x1a25a0.
- 0x2081cc (ev_4e9c96fe…) zeroes the whole buffer (0x24d234), then sets the options block:
  - +0 = 0x8a: high nibble = global 0x379d64 (8), low nibble = 0x379d66 (10). Initial globals 08 08 0a (ev_a37531a1…). 0x1c33c0 applies the same nibbles from these globals, apparently a second reset path.
  - +1 = language: 0x21b650 (ev_b616888e…) maps the 3DS CFG system language (0x3292fc reads block 0xA0002, ev_57575aae…) as 2→2, 3→4, 4→3, 5→5, 7→12, 8→6, 9→8, 10→9, otherwise 1. 0x2babe0 (ev_4d5865e7…) falls back to 1 when the index is not in a runtime list at *0x376ae8 (stride 0x18; the pointer is 0 in the static image).
  - +2 = 0xff (last slot: none). +3 bits 2..5 = 0x379d65 (8).
  - Callback 0x1a25a0 (disassembled with llvm-objdump; not a Ghidra function): +4 = +5 = 10, +6 = +7 = 8, +3 bit 0 set, bit 1 cleared. Result: options start 8a LL ff 21 0a 0a 08 08, the rest zero.
- Then each slot is zeroed (0x2594d8, ev_66267ca4…) and the slot callback 0x1a25cc (ev_bea3ecb3…) runs. In the slot buffer it sets +0x51b3/+0x51b4 from a runtime object (*0x375914 +0x97/+0x98), ORs +0x51e0 with 0x10 and +0x520b with 0x20, and applies cheat stud bonuses (+0x5148 u64, +0x5190 u32; +0x520b bits 0/1 mark the 100000/300000 bonuses as given). It also writes live globals (0x51b6b6, 0x52086b, 0x52089d.., 0x52089e bitset, character unlocks 0x231fc0 for IDs 9..140) and options +4+slot = 10, +6+slot = 8, +0x1ec + slot×0x18 = 0.
- Implemented: SaveImage::resetOptions and languageForSystem in chase_save. The nibble meanings (likely volume or similar sliders) and the language index order are unknown. Slot defaults are not reproduced: they depend on runtime values and on how live globals reach the buffer.
