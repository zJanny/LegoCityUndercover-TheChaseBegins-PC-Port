# Localisation, text escapes and fonts

Reverse-engineering findings, moved verbatim from the progress log. Entries are chronological; later entries may correct earlier ones. Evidence IDs are shortened with an ellipsis.

## Current state

- `.loc` files: "LOCA", version 2, count, table bytes, string bytes, then sorted (u32 key, u32 offset) pairs and NUL-terminated Latin-1 strings; 66 files, 25207 entries. [§2026-10-13T00:25](#ts-20261013-0025)
- Keys are the sdbm hash `h = h * 65599 + c` of the upper-case identifier; data-only evidence, the hash routine was not found in code. [§2026-10-13T00:25](#ts-20261013-0025)
- Text escape `0x01 '2' c` is character `c - 0x30` of `localisation/extended_chars_utf8.txt` (84 characters); no escapes remain after decoding. [§2026-10-13T00:45](#ts-20261013-0045)
- Files with the suffix `_pl` hold Dutch text. [§2026-10-13T00:45](#ts-20261013-0045)
- `.tnf` fonts: u16 end code, u8 first code (0x20), u8 line height, then (end - first) records of (u16 width, height, x, y) into `<name>.btga`; 6 fonts, 1521 glyphs. [§2026-10-13T01:05](#ts-20261013-0105)
- Codes at or above 0x100 are the extended characters in file order; the escape probably means code `('2' - 0x31) * 0x100 + (c - 0x30)`. [§2026-10-13T01:05](#ts-20261013-0105)
- The three front-end font files (`frontendfont`, `-small`, `-large`) are byte-identical: one glyph set, line height 23, glyphs for ASCII, the Latin-1 letters and the 84 extended characters; `gametext` has On, Off, Yes, No and eight language names in their own spelling. [§2026-10-29T11:50](#ts-20261029-115026)
- Open: whether the layout font size changes the drawn glyph size (HUD-75), the byte 0xac for the trademark sign, the meaning of the '2' byte and other escape first bytes, and whether "ź" and "ż" are swapped in the atlas.

<a id="ts-20261013-0025"></a>

## 2026-10-13T00:25+02:00 — Localisation tables (.loc) decoded

- Layout (data): "LOCA", u32 version 2, u32 count, u32 table bytes (= count × 8), u32 string bytes (header + table + strings = file size); then (u32 key, u32 offset) pairs sorted by key; then NUL-terminated strings. All 66 files (25207 entries) parse.
- Keys are the sdbm hash h = h × 65599 + c of the upper-case identifier.
  - Found by brute-forcing the multiplier on "OK" → 0x4f13bc (only 65599 fits) and confirmed on "NO" and "ON".
  - 37 of 351 gametext_uk entries hash from their own upper-cased text, and 212 keys over all files resolve from code.bin strings (e.g. PAUSEMENU, CLASS_MINER).
  - The hash routine was not found in code (no 0x1003f literal or shift pattern); data-only evidence.
- Text is Latin-1 (e.g. "Français" 0xe7). 0xac stands where ™ belongs ("Nintendo\xac3DS"); not confirmed.
- Byte 0x01 + 2 bytes is a glyph escape (Russian, Polish). It is neither an offset into extended_chars_utf8.txt nor a constant code-point offset, and probably indexes the .tnf glyph table (8-byte x, y, w, h records). Not decoded.
- Added tools/convert_loc.py (dump with `--names`, `--census`), tests/test_convert_loc.py (3), and src/assets/text.cpp (textKey, parseLocalisation) with synthetic and real-data (66/25207) checks.
- ctest PASS 7/7; Python PASS 53/53.

<a id="ts-20261013-0045"></a>

## 2026-10-13T00:45+02:00 — Text escapes decoded (Cyrillic/Polish)

- Escape 0x01 '2' c → character c − 0x30 of localisation/extended_chars_utf8.txt (84 characters, in file order).
  - Found by frequency: the most common ru codes ':' 'C' '7' '>' '<' 'A' map to о а и н е т at offset 0x30, matching Russian letter frequency.
  - Confirmed by readable text ("Начальник тюрьмы Филдинг", "Сохранить и выйти"). After decoding, 0 escapes remain over all 66 files.
  - The earlier brute force had stopped at offset 0x2b. The meaning of the '2' byte is unknown (5295 of 5306 ru escapes use it; the rest look like data, not escapes).
- Data oddity: the files with the "_pl" suffix hold Dutch text (e.g. "Opslaan en stoppen").
- Python decode_text (`--extended`); C++ extendedCharacters/decodeText (UTF-8 output).
- Tests: synthetic escape/Latin-1/unknown-escape cases. Real data: FNV-1a of decoded gametext_ru = 0xc0c84ff0 in both implementations.
- ctest PASS 7/7; Python PASS 54/54.

<a id="ts-20261013-0105"></a>

## 2026-10-13T01:05+02:00 — Bitmap fonts (.tnf) decoded

- Layout (data): u16 end code, u8 first code (0x20), u8 line height, then (end − first) records of (u16 width, u16 height, u16 x, u16 y) into <name>.btga.
  - The file size equals 4 + 8 × count for all 6 fonts: debug 0x20–0x80, editorfont 0x20–0xff, frontendfont(-small, -large) 0x20–0x15f, pixelfont 0x20–0x10f; 1521 glyphs.
  - The x positions step by width + 1 within an atlas row.
- Codes ≥ 0x100 are the extended characters in extended_chars_utf8.txt order: "Сохранить и выйти" and Polish/quote/™ glyphs render correctly from frontendfont. So the escape 0x01 '2' c probably denotes code ('2' − 0x31) × 0x100 + (c − 0x30); the general rule for other first bytes is unknown.
- In a test render, "ź" looked like "ż" (unclear at that size; possibly swapped atlas glyphs); unconfirmed.
- Added tools/convert_font.py (draw text to PNG) and tests/test_convert_font.py (3); C++ parseFont/layoutText with synthetic and real-data (6 fonts / 1521 glyphs) checks.
- ctest PASS 7/7; Python PASS 57/57.


<a id="ts-20261029-115026"></a>

## 2026-10-29T11:50:26+01:00 — Front-end font files, glyph coverage and menu words (data only; 20261029-qolview-D)

By 20261029-qolview-D, data only (no code read, no emulator): the `.tnf` headers of the extracted archive read with the layout of [§2026-10-13T01:05](#ts-20261013-0105), texts with `tools/convert_loc.py --names` on `gametext_uk.loc`.

- `fonts/frontendfont`, `fonts/frontendfontsmall` and `fonts/frontendfontlarge` are byte-identical, both the `.tnf` and the `.btga`: one glyph set, codes 0x20..0x15f, line height 23, glyph heights 10 to 22 pixels ("x" and "0" are 8 pixels wide).
- Codes without a glyph (width 0): 0x80..0x9b, 0x9d..0xa0, 0xa2..0xa8, 0xac, 0xad, 0xaf, 0xb1..0xba, 0xbc..0xbe, 0x154..0x15f. Every code of 0x20..0x7e, the Latin-1 letters 0xc0..0xff (the sign × 0xd7 included) and the 84 extended characters 0x100..0x153 have glyphs.
- The extended set holds only the Cyrillic, Polish and punctuation characters the shipped texts use, not whole alphabets; all letters of "Русский" are in it.
- The other fonts: `debug` 0x20..0x80 (line 8), `editorfont` 0x20..0xff (line 13), `pixelfont` 0x20..0x10f (line 9); `pickupfont` has a `.btga` and no `.tnf`.
- `gametext_uk` has `ON` "On", `OFF` "Off", `YES` "Yes", `NO` "No", `OPTIONS` "Options", `LANGUAGE` "Language", `SELECT` "Select", `PAUSEMENU` "Pause Menu", and language names in their own spelling under keys whose names are not known: 0x04723177 "Deutsch", 0x04ee198b "Français", 0x363c0017 "Nederlands", 0x4f11523f "English", 0x952b154f "Italiano", 0xeecdb206 "Português", 0xf63a151e "Dansk", 0xf7468743 "Español".

Unknown: whether the three entries of the layout font table 0x52ebf8 ([ui-layouts.md §2026-10-13T11:00](ui-layouts.md#ts-20261013-1100)) are these three files, and whether the font size field of a layout text changes the drawn glyph size (HUD-75); the Polish and Russian language names (not found under the names tried); where the Languages page takes its names from (UI-23).
