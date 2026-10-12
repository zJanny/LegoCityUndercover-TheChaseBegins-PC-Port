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

