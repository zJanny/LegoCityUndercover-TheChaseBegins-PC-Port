# Reverse-engineering findings

Everything recovered about the game's formats and code lives here, one file per topic.
Read this index, then open only the file you need.

| File | Contents |
|---|---|
| [container-and-archive.md](container-and-archive.md) | CCI/NCCH identity, RomFS, FUSE `.fib` archive and RefPack codec |
| [arm-executable.md](arm-executable.md) | Startup chain, SVC stubs, services, paths, HID input, channels, main loop |
| [pooled-resources.md](pooled-resources.md) | Shared layout of all pooled `b*` formats |
| [textures.md](textures.md) | `.btga/.bpng/.bdds`, v orientation, DMPGL notes |
| [audio.md](audio.md) | `.bwav`/CWAV decode, mixer |
| [models.md](models.md) | `.bfnmdl`, vertex layout, materials, shaders, UV scales, lightmaps (collision section: see collision.md) |
| [levels.md](levels.md) | `.blvl`, placements, Euler order, nav grids (`.bfnnav`) |
| [skeleton-animation.md](skeleton-animation.md) | `.bfnskl`, `.bfnanm`, curves, pose composition, skinning, bone palettes |
| [cutscenes.md](cutscenes.md) | `csdata` cutscenes: tracks, cameras, actors, soundtrack |
| [text-localisation-fonts.md](text-localisation-fonts.md) | `.loc`, text escapes, `.tnf` fonts |
| [data-tables-config.md](data-tables-config.md) | `.bxls` tables, brace-block config text |
| [save-format.md](save-format.md) | `savegame.dat` layout, CRC, save thread |
| [ui-layouts.md](ui-layouts.md) | `.bxaml` UI layouts and storyboards |
| [fmv.md](fmv.md) | Mobiclip `.moflex` video and subtitles |
| [particles.md](particles.md) | `particledefs.bprt` particle definitions |
| [oracle.md](oracle.md) | Azahar runtime oracle: setup, fidelity rules, save states, fast workflow, timings |
| [camera.md](camera.md) | Gameplay camera: yaw word 0x376894 writer, manager and modes, follow camera law (OnFoot preset), projection, unknowns |
| [player.md](player.md) | Player entity pointer, scene node, controller fields, stick-to-intent, locomotion rules and root motion |
| [collision.md](collision.md) | Level collision data (model+0x1c section), ground probe by box bisection, wall slide, differential test against traces |

Rules for editing these files are in CLAUDE.md ("Reverse-engineering findings").
