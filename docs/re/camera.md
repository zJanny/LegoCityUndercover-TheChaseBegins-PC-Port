# Gameplay camera

How the original game places the gameplay camera and where the yaw word 0x376894 comes from. Findings from REA (Ghidra 12.1.2, `artifacts/game/game.arm.elf`) and the Azahar oracle (`gameplay-start` checkpoint, Chase in the street of lvl_01).

Split by subtopic because of the 40 KB limit:

- [camera-follow.md](camera-follow.md): camera chain, manager and modes, follow camera law, yaw word, checks
- [camera-collision-sweep.md](camera-collision-sweep.md): slab sweep, pull-in, side and vertical push, box slicing and layer of placed objects
- [camera-records-blending.md](camera-records-blending.md): camera lock and its icon, record selection and record pose 0x298cd8, fade between commands and pose filter
