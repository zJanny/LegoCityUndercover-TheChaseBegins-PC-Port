# Reference game version

> **WARNING: every address, offset, struct layout, literal location and evidence ID in `docs/re/`, in code comments and in tests refers to exactly the build below.** A different region, revision or an installed update almost certainly moves the code and data. With any other image, treat all `docs/re/` addresses as wrong until they are re-verified, and do not compare oracle traces from it with the pinned test values.

## The build used for all reverse engineering

| Field | Value |
|---|---|
| Image file | `LEGO City Undercover - The Chase Begins (Europe) (En,Fr,De,Es,It,Nl,Pt,Da,Ru).cci` (decrypted, `no_crypto`) |
| Image size | 1073741824 bytes |
| Image SHA-256 | `078faf7e231381e53d7d7503d4729feab390e3cd1b8e739dc44f9933d8b759ad` |
| Title ID (NCSD media ID, NCCH program ID) | `00040000000AD600` |
| Product code | `CTR-P-AA8P` (Europe) |
| Program name (exheader) | `LEGOCITY` |
| Title version (exheader remaster version, +0xe) | 0, the original release; no update applied |
| Decompressed `.code` SHA-256 | `4b91f1e353df297f04761ce3e7928fbb8a85db618f2e4c8696551ad20ac0439e` (2650112 bytes) |
| Analysis ELF `game.arm.elf` SHA-256 | `5152ed14b9ed41cc4fb091c12f5b960dac93bf15b55159e4bdfb3fca41d50881` |
| ARM segments | text 0x100000 (2450812 bytes), rodata 0x357000 (79480), data 0x36b000 (112384); bss 2219880 bytes |
| Exheader SHA-256 | `0c00367a256b424d932327c3f7db1575625e4856eb0b069c59c4a2a835a57a4a` |

The NCCH header field +0x112 (value 2) is the container format version, not the game version.

## How to check an image

1. `sha256sum "<image>.cci"` must print the image SHA-256 above. This is the authoritative check.
2. If the image hash differs (for example a re-dump with different padding or trimming), extract it with `python3 tools/extract_game.py "<image>.cci" --output <new dir>` and compare `<new dir>/manifest.json`: `product_code`, `decoded_code.sha256` and `analysis_elf.sha256` must equal the values above. Equal code hashes mean the addresses still hold; the RomFS can still differ, so also re-run `assets-real-data`.
3. Any other title ID, product code (for example another region), a non-zero remaster version, or a different `decoded_code.sha256` means a different build: the warning at the top applies.

## Rules for agents

- Before using an address from `docs/re/`, make sure the image in use is this build (step 1 or 2).
- If the user supplies a different build, stop and report it instead of applying the documented addresses.
- New findings are recorded against this build only.
