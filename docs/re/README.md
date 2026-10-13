# Reverse-engineering findings

Everything recovered about the game's formats and code lives here, one file per topic.
Read this index, then open only the file you need.

> **All addresses here are for one exact build of the game (Europe, title ID 00040000000AD600, v1.0, image SHA-256 078faf7e…59ad). With any other image they are probably wrong.** Check the image first: [game-version.md](game-version.md).

| File | Contents |
|---|---|
| [game-version.md](game-version.md) | The reference build (hashes, title ID, version), how to check an image, the different-version warning |
| [container-and-archive.md](container-and-archive.md) | CCI/NCCH identity, RomFS, FUSE `.fib` archive and RefPack codec |
| [arm-executable.md](arm-executable.md) | Startup chain, SVC stubs, services, paths, HID input, channels, main loop |
| [pooled-resources.md](pooled-resources.md) | Shared layout of all pooled `b*` formats |
| [textures.md](textures.md) | `.btga/.bpng/.bdds`, v orientation, DMPGL notes |
| [audio.md](audio.md) | `.bwav`/CWAV decode, mixer |
| [models.md](models.md) | `.bfnmdl`, vertex layout, materials, shaders, UV scales, lightmaps (collision section: see collision.md) |
| [levels.md](levels.md) | `.blvl`, placements, Euler order, nav grids (`.bfnnav`) |
| [objects.md](objects.md) | Level objects: classes, class files (+0x10/+0x14), parameter slots, how the model is found, drawn classes |
| [skeleton-animation.md](skeleton-animation.md) | `.bfnskl`, `.bfnanm`, curves, pose composition, skinning, bone palettes |
| [cutscenes.md](cutscenes.md) | `csdata` cutscenes: tracks, cameras, actors, soundtrack |
| [text-localisation-fonts.md](text-localisation-fonts.md) | `.loc`, text escapes, `.tnf` fonts |
| [data-tables-config.md](data-tables-config.md) | `.bxls` tables, brace-block config text |
| [save-format.md](save-format.md) | `savegame.dat` layout, CRC, save thread |
| [ui-layouts.md](ui-layouts.md) | `.bxaml` UI layouts and storyboards |
| [fmv.md](fmv.md) | Mobiclip `.moflex` video and subtitles |
| [particles.md](particles.md) | `particledefs.bprt` particle definitions |
| [scripts.md](scripts.md) | Level scripts (section +0x18), the script executor 0x234204 and its words, EnableObject / EnableCollision, the `startup` script, the revealer links, the start state of placed objects |
| [doors.md](doors.md) | Doors (type 42): record, update 0x1d16b8, state change, messages, easing, hinge and slide pose, timings measured in the oracle |
| [studs.md](studs.md) | Studs (collectible coins): pickup pool records, sprite sheet drawing and spin, pickup radius and skip counter, collect timer, counter values and limits, stand-ins |
| [oracle.md](oracle.md) | Azahar runtime oracle: setup, fidelity rules, save states, fast workflow, timings |
| [camera.md](camera.md) | Gameplay camera: yaw word 0x376894 writer, manager and modes, follow camera law (OnFoot preset), projection, obstruction by level geometry (slab sweep, pull-in, side and vertical push), unknowns |
| [player.md](player.md) | Player entity pointer, scene node, controller fields, stick-to-intent, locomotion rules and root motion |
| [front-end.md](front-end.md) | Boot sequencer 0x302080, screen order and triggers from power-on to the first level, prompt state machine 0x313368, save indicator, language, savegameflow pages, loading screen, intro movie bubble and title card |
| [collision.md](collision.md) | Level collision data (model+0x1c section), ground probe by box bisection, wall slide, placed objects (world grid, registration, shapes, query layers), differential test against traces |

Python tools that were later removed from the repository (convert_models, convert_anims, convert_nav, tt_resource, preview_model, the census modes of convert_levels, and the Python decoders behind convert_textures, convert_audio, convert_loc, convert_font and extract_fib) last existed at commit `9d04c91`. Older entries that name them stay as written.

Rules for editing these files are in CLAUDE.md ("Reverse-engineering findings").
