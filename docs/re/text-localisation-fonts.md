# Localisation, text escapes and fonts

Reverse-engineering findings, moved verbatim from the progress log. Entries are chronological; later entries may correct earlier ones. Evidence IDs are shortened with an ellipsis.

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

## 2026-10-13T00:45+02:00 — Text escapes decoded (Cyrillic/Polish)

- Escape 0x01 '2' c → character c − 0x30 of localisation/extended_chars_utf8.txt (84 characters, in file order).
  - Found by frequency: the most common ru codes ':' 'C' '7' '>' '<' 'A' map to о а и н е т at offset 0x30, matching Russian letter frequency.
  - Confirmed by readable text ("Начальник тюрьмы Филдинг", "Сохранить и выйти"). After decoding, 0 escapes remain over all 66 files.
  - The earlier brute force had stopped at offset 0x2b. The meaning of the '2' byte is unknown (5295 of 5306 ru escapes use it; the rest look like data, not escapes).
- Data oddity: the files with the "_pl" suffix hold Dutch text (e.g. "Opslaan en stoppen").
- Python decode_text (`--extended`); C++ extendedCharacters/decodeText (UTF-8 output).
- Tests: synthetic escape/Latin-1/unknown-escape cases. Real data: FNV-1a of decoded gametext_ru = 0xc0c84ff0 in both implementations.
- ctest PASS 7/7; Python PASS 54/54.

## 2026-10-13T01:05+02:00 — Bitmap fonts (.tnf) decoded

- Layout (data): u16 end code, u8 first code (0x20), u8 line height, then (end − first) records of (u16 width, u16 height, u16 x, u16 y) into <name>.btga.
  - The file size equals 4 + 8 × count for all 6 fonts: debug 0x20–0x80, editorfont 0x20–0xff, frontendfont(-small, -large) 0x20–0x15f, pixelfont 0x20–0x10f; 1521 glyphs.
  - The x positions step by width + 1 within an atlas row.
- Codes ≥ 0x100 are the extended characters in extended_chars_utf8.txt order: "Сохранить и выйти" and Polish/quote/™ glyphs render correctly from frontendfont. So the escape 0x01 '2' c probably denotes code ('2' − 0x31) × 0x100 + (c − 0x30); the general rule for other first bytes is unknown.
- In a test render, "ź" looked like "ż" (unclear at that size; possibly swapped atlas glyphs); unconfirmed.
- Added tools/convert_font.py (draw text to PNG) and tests/test_convert_font.py (3); C++ parseFont/layoutText with synthetic and real-data (6 fonts / 1521 glyphs) checks.
- ctest PASS 7/7; Python PASS 57/57.

