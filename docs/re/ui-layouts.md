# UI layouts (.bxaml)

Reverse-engineering findings, moved verbatim from the progress log. Entries are chronological; later entries may correct earlier ones. Evidence IDs are shortened with an ellipsis.

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


