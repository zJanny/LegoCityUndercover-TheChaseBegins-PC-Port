# Traffic cop whistle

## 1. Scope

The player's cop whistle: the request in the player input, the state 0x1b2 and the clock of the traffic stop it starts. The stop itself and the cars that obey it are in [pedestrians-traffic-cars-driver.md](pedestrians-traffic-cars-driver.md).

## 2. Status

IN_PROGRESS, updated 2026-11-12T08:06:30+01:00. The whistle is connected in `chase-game` (TRAFFIC-47 done): with the extras bit 19 set (`--extras 524288`; the mask is 0 after a load, as in the original) a D-pad Down press in the player's idle state with free hands plays the clip row 709, stops the traffic for 120 to 179 clock units and returns to idle when the clip has played. The clock unit and the meaning of the character row byte are not read.

## 3. Verified behaviour

- Read from the code (test `player-cop-whistle`), from [traffic-car-push.md §2026-11-12T08:05:53](../../re/traffic-car-push.md#ts-20261112-080553): the five reads of the request, ctrl +0x13c as the carried object, the enter and the update.

## 4. Stand-ins

- TRAFFIC-S43 Facts of the connected whistle the port chooses itself: the byte +0x20 of the player's character row is taken as 3; the idle state, the free hands and the Down edge come from the port's player state and pad slot 1; the state ends after the clip's length in updates without the slot's finished test; the player stands during the state. needs: TRAFFIC-60. evidence: [traffic-car-push.md §2026-11-12T08:05:53](../../re/traffic-car-push.md#ts-20261112-080553)

## 5. Open items

- [ ] TRAFFIC-59 — Measure the game clock over stepped updates (its unit and its value at the level start) on a street with traffic, and the update on which the whistle's clip reports finished. blocks: TRAFFIC-S33. evidence: [traffic-car-push.md §2026-11-12T08:05:53](../../re/traffic-car-push.md#ts-20261112-080553)
- [ ] TRAFFIC-60 — Read the meaning of the byte +0x20 of the character table row (value 3 for the player) and the menu that toggles the extras bit 19. blocks: TRAFFIC-S43. evidence: [traffic-car-push.md §2026-11-12T08:05:53](../../re/traffic-car-push.md#ts-20261112-080553)

## 6. Findings summary

- [traffic-car-push.md §2026-11-12T08:05:53](../../re/traffic-car-push.md#ts-20261112-080553): the request reads, the carried-object meaning of ctrl +0x13c, the port's connection.

## 7. Code and tests

`src/runtime/player_cop_whistle.{hpp,cpp}` (`PlayerCopWhistle`, no SDL) over `src/runtime/traffic_car_push.*`; glue `src/game_cop_whistle.cpp`; CTest `player-cop-whistle` (`tests/player_cop_whistle_test.cpp`) and the `--extras` rejection in `game-options` (`tests/game_options_test.cpp`).
