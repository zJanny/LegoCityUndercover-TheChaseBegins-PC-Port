# UI layouts (.bxaml)

Reverse-engineering findings, moved verbatim from the progress log. Entries are chronological; later entries may correct earlier ones. Evidence IDs are shortened with an ellipsis.

## Current state

- `.bxaml` layouts load through 0x217220 (second path 0x2ae5dc, falling back to `.xaml`); blocks in order: u32 header, element tree (0x4c-byte records, types 1 container, 2 image, 3 reference), optional storyboards. [§2026-10-13T11:00](#ts-20261013-1100)
- Storyboards: 0x10 record per storyboard, 0x20 per track, 0x20 per keyframe; the runtime builder 0x1dbb80 skips tracks without a resolved target and sets the storyboard length to the longest track (duration times repeat). [§2026-10-13T09:12](#ts-20261013-0912)
- All 55 real layouts consume every block: 383 containers, 614 images, 110 references, 161 storyboards, 1888 tracks, 6740 keyframes; the decoder is `src/assets/xaml.cpp`. [§2026-10-13T11:00](#ts-20261013-1100)
- The meaning of element floats and track property ids (opacity, rotation, scale, position, translate) is a data fit only; the evaluator was not found. [§2026-10-13T09:12](#ts-20261013-0912)
- The round menu layout (320x240) holds, beside the round, a list form of five bars with a highlight, a framed strip and a page indicator whose use is not read; the Options, pause and language pictures are hexagonal badges: [§2026-10-29T11:50:58](#ts-20261029-115058).
- Layout text fonts: table 0x52ebf8 maps fontSize to three identical frontendfont instances that differ only in minimum spacing; the width rule behind HUD-75 is open. [§2026-10-29T13:58](#ts-20261029-135814)
- Open: which screen uses the list form and the page indicator, element +0x14, other flag bits, track +0x1c and keyframe +0x1c, and the property evaluator.

<a id="ts-20261013-1100"></a>

## 2026-10-13T11:00+02:00 — .bxaml UI layouts decoded (REA, Ghidra 12.1.2)

- Loader 0x217220 (ev_b5b7e922…460e), reached from the ".bxaml" string at 0x2175d4. A second path, 0x2ae5dc, loads `%s.bxaml` and falls back to `%s.xaml` (ev_e349a2af…94b1).
- Block order, all in pool −14:
  - A u32 header; non-zero means storyboards follow.
  - The element tree, depth-first, read by 0x29f7b4 (ev_950e0153…c69f). Each element is a 0x4c record and a name. Types 2 and 3 add a source block. The child count is the s16 at +0x40.
  - Optional storyboards: an 8-byte list (count at +4); per storyboard a 0x10 record (track count at +0xc) and a name; per track a 0x20 record (keyframe count at +8) and a target name, then 0x20-byte keyframes.
  - Record words +0, +4, +0xc and +0x10 are pointer slots that the loader overwrites.
- Node builder 0x1b2ec0 (ev_0069b6e8…4fd2):
  - Types: 1 container, 2 image (path relative to the layout folder unless rooted; a size ≤ ~1e-7 takes the texture size), 3 reference to another layout.
  - Fields: flags at +0x42 (bit 0x80 = the child the parent keeps), hook id at +0x44 (always 0). +0x46 is matched against the 3-entry font table at 0x52ebf8; names containing "noexport" are skipped. +0x47 & 3 is alignment; +0x48 is a u32 colour.
- Storyboard builder 0x1dbb80 (ev_5b4bf86b…a55a5):
  - Track: repeat at +0xa (0 = once), property byte at +0x14, duration at +0x18.
  - Keyframe: value at +4, four spline floats at +8..+0x14 (passed to 0x249030), time at +0x18, flag at +0x1c. Targets are resolved by 0x24085c.
- Data fits only (not traced in code):
  - The element floats +0x18..+0x3c are opacity, rotation (radians), scale, position, origin (always 0.5) and size.
  - Track properties: 6 opacity, 3/4 scale x/y, 5 rotation, 1/2 translate x/y.
  - The keyframe spline defaults to (0,0,1,1) in 6499 of 6740 keyframes (XAML KeySpline).
- Unknown: element +0x14 (0xff ×1105, 3 ×2), the flag bits other than 0x80 (values 1, 0x11, 0x41), track +0x1c and keyframe +0x1c, and the code that applies property ids. Two references (waiting, UI_StrikeControl) have no sibling .bxaml. Two track targets name no element.
- Added src/assets/xaml.cpp (parseLayout, UiLayout types) with a synthetic layouts() test.
- Real data: 55/55 files consume every block (13880). 383 containers, 614 images and 110 references. 161 storyboards, 1888 tracks, 6740 keyframes. Every track's duration equals its last keyframe time. The pinned float sums come from a throwaway Python walk.
- Merged onto main by hand (the worktree was based on origin/main). ctest PASS 8/8.

<a id="ts-20261013-0912"></a>

## 2026-10-13T09:12+02:00 — Storyboard runtime layout (REA Ghidra 12.1.2 ev_5b4bf86b…; read-only, evaluator not found)

- 0x1dbb80 allocates a 0x38-byte animation object from pool *0x376490. It stores the storyboard set at +0x30 and a flat target list at +0x34 (one u32 per track, from the file track record +0x10). Byte +1 = 0x1c when the second argument is the constant 0x5429d0d0, otherwise 0x10 (meaning unknown). Storyboard sets are shared by that argument and reference-counted (set +0x10), in a list at *0x376488.
- Run-time records:
  - Storyboard (0x10): +0 tracks, +4 s16 track count, +8 f32 total length, +0xc name copy.
  - Track (0x10): +0 keyframes, +4 u8 property (file +0x14), +6 s16 keyframe count, +8 s16 repeat (file +0xa), +0xc f32 duration (file +0x18).
  - Keyframe (0x1c): +0 value, +4..+0x10 spline coefficients from 0x249030, +0x14 time, +0x18 u8 flag.
- Tracks whose file pointer +0x10 is 0 (no resolved target) are skipped. This explains the two tracks whose targets name no element.
- Storyboard length = max over its tracks of (repeat == 0 ? duration : duration × repeat).
- Keyframes are written from the end of the array while walking the loaded list. Whether this reverses the file order depends on how 0x29f7b4 links them (not checked).
- Searched without finding the property evaluator: byte +4 reads followed by a 6/7-entry jump table (0x2215bc is a collision direction filter), multiply-by-28 indexing (0x1fac18, 0x273e84: unrelated), and users of *0x376488/*0x376490 (0x103538, 0x223b5c: unrelated). The property ids stay data-fit only.



<a id="ts-20261029-115058"></a>

## 2026-10-29T11:50:58+01:00 — Round menu layout pieces and the status-screen pictures (chase-layoutdump; 20261029-qolview-D)

By 20261029-qolview-D, data only (no code read, no emulator): `chase-layoutdump` on the layouts named below, the picture folders listed and viewed. Pictures are described in words.

- `sprites/ui_roundaboutmenu/main.bxaml`, the layout of the round menu and of the pause Options page ([ui-screens.md §2026-10-28T22:51:42](ui-screens.md#ts-20261028-225142)): `LayoutRoot` 320x240 (the bottom screen). Background `BG_FlipA` (`UI_LOTR_BG`, 512x256 at (0, 0), a blue honeycomb pattern over a pale city skyline; 320x240 of it is on the screen). The round: five 128x128 boxes `Button_1`..`_5` in `Menu` (opacity 0.75, 0.5, 1, 1, 0.75 in the file). The name plate `Item_Name` (256x32 `UI_itemNameBG`, a light blue outlined plate) with `TEXT_ItemName` (182x70, font 18); `TEXT_Icon`, `TEXT_Icon_In`, `TEXT_Icon_Out` (256x32, font 12, centred). The top bar `Top_Bar` (320x32 of `UI_Barstrip_Tile` and `UI_Barstrip`) with `TEXT_Title` (font 15); the subtitle bar `Subtitle` (256x32 `UI_subtitleBar`, a blue bar with a slanted end) with `TEXT_Subtitle` (font 15); the corner plates `Bottom_Left_BTN`, `Bottom_Right_BTN` (64x32 `UI_buttonBG`, one mirrored) under the references `backButton` and `selectButton`.
- The same layout also holds a list form: `Menu_Bars` with five 256x64 bars `Menu_Bar_1`..`_5` (two mirrored halves of `UI_LSHFrontendMenuBar`, a dark red bar in a steel frame) with text boxes `TEXT_Menu_n_3DS` (168x16), the highlight bars `Menu_Bar_HI` and `Menu_Bar_HI_Down` (256x32, two halves of `UI_LSHFrontendMenuBarHI`, a pale glow); `Menu_Strip` (two mirrored `UI_LSHFrontendMenuFlip`, a dark panel in an ornate frame) with `TEXT_Bio` (200x120); and a page indicator `Pageslots` with `Left_Arrow` and `Right_Arrow` (16x32 `UI_FEArrow`, a light blue chevron, the right one mirrored) and `PageNumber` (font 16). The picture folder also has `UI_PageSlot` (8x8 blue dot) and `UI_PageSlotCurrent` (16x16 blue ring), which no element of this layout uses.
- `sprites/ui_roundaboutmenu/menu_bar.bxaml` (no loader found, [ui-screens.md §2026-10-19T20:33](ui-screens.md#ts-20261019-203310)): one 256x32 bar of the same art with the text `TEST` (font 12, centred).
- `sprites/ui_statusscreen/options/`: the four 128x128 row pictures `UI_Options_SFX`, `_Music`, `_InvertCamera`, `_Language`, hexagonal blue badges in a steel frame showing one note, two notes, a camera on a stick, a globe with a speech bubble. `sprites/ui_statusscreen/`: the pause pictures `UI_StatusScreen_Resume`, `_Options` (a wrench and a nut), `_Credits`, `_Exit`, `_Extras` (a yellow brick), same badge style. `sprites/ui_statusscreen/languages/`: eleven 64x64 hexagonal flag badges `UI_Language_Danish`, `_Dutch`, `_English` (a United States flag), `_French`, `_German`, `_Italian`, `_Polish`, `_Portuguese`, `_Russian`, `_Spanish`, `_UK`.
- The notice and question page `page_alerttext` is described in [front-end.md](front-end.md) (layout pages).

Unknown: which screen shows the list form, the menu strip and the page indicator (the round menu create 0x2206f0 resolves `Menu_Bar_1`..`_5`, [ui-screens.md §2026-10-19T20:34](ui-screens.md#ts-20261019-203404), but their use is not read); which font size the values 12, 15, 16 and 18 select (HUD-75).

<a id="ts-20261029-135814"></a>

## 2026-10-29T13:58:14+01:00 — Layout text font table 0x52ebf8 and the font object (llvm-objdump of `artifacts/game/game.arm.elf`, agent 20261029-qolmenu-A; no emulator)

Read with llvm-objdump (armv6k, vfp2); table bytes read from the code image.

- **Table 0x52ebf8** (3 entries of 33 bytes: byte 0 largest size, name string from byte 1) is filled once by 0x2706c4, called from the start-up function at 0x1a2018 with the stack entries (12, "\x02"), (14, "\x03"), (16, "\x04"). The node builder 0x1b2ec0 (loop at 0x1b32ec) takes the first entry whose largest size is at least the node's fontSize (node +0x7c, record +0x46) and whose font is loaded through the callback 0x2d0fb0; sizes 0 and above 16 match no entry and leave node +0x78 at 0. Cache: 0x52ebe0, 8 bytes per entry (font handle, use count).
- **Callback 0x2d0fb0** treats the name byte as an index into the 8-byte table 0x374214 (string pointer, two signed bytes): ids 2, 3, 4 all name `Fonts/frontendfont` (the three font files are identical) with minimum spacing -1, -2, -2 and minimum line gap -5, stored at font +0xae / +0xaf (default -30), then glyph range 32..160 (0x1f3744).
- **Font object** (0xb8 bytes, constructor 0x1f394c): +0xac / +0xad are the current spacing and line gap (default 0), clamped from below by +0xae / +0xaf (0x235cb0, 0x235c9c); +0x88 / +0x8c are the scale set by 0x235d14 (only the stud bar, HUD and cutscene text callers found). The fitting loop at 0x1ce904 lowers the spacing one step at a time until the text fits.
- **Setting a node's text** (0x20a828: node, font, text, colour): stores font to +0x78, colour +0x80, a copy of the text +0x88; a zero font becomes the first cache slot. The Build-it prompt (0x213df4) passes the global `frontendfont` object (pointer in 0x3757a0), not the table font, for name, cost and prompt, so fontSize 17 selects nothing there.
- **Measured port widths** of "Build" (frontendfont, glyph rectangles): 24 / 28 / 32 / 36 at spacing -2 / -1 / 0 / 1; the original shows 30, which no integer spacing gives (scale 1.25 at spacing -2 would, but the measured scale is 1.0, see below). The writer of the spacing at draw time was not found.

- **Oracle read 20261029-qolmenu-A-2** (recorded from save states, 2026-10-29, two loads each, all agreeing): the pointer at 0x3757a0 is 0x084eba30 in the Build-it prompt state and on the options page. In the 0xb8 bytes it points to the floats at +0x88, +0x8c and +0x98 are 1.0 in both states, so no glyph scale explains the width. The two -30 bytes sit at +0xac / +0xad there (the constructor puts them at +0xae / +0xaf), so this block is not laid out like the font object read from 0x1f394c and its spacing bytes were not obtained. Measured widths: "Build" 30 (fontSize 17), "Music: 5" 53, "Paused" 44, "Options" 47 pixels (fontSize of the last three unknown).

Unknown: the code that draws a layout text node (every call of the text printer 0x235cc4 and the layout 0x21bc2c was checked; none reads node +0x78), the state of the global font (+0x88, +0xac) while the prompt is drawn, and what the sizes 12 to 16 change beyond the minimum spacing.
