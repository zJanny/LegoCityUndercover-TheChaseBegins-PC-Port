# Disguises: character classes, the party and the swap

How Chase changes his disguise in the original: the character classes of `tables/tablescharacters.bxls`, the party manager at 0x51b248, the swap of the player entity, the ways a swap is asked for (D-pad, lower screen, script, swap pad) and what the levels and the save hold. Static passes; the party manager is ported, the swap of the player entity is not. Items: [disguise.md](../plans/features/disguise.md). The script natives were read before: [script-natives-world.md](script-natives-world.md) (SetPartyCharacter), [script-natives-party.md](script-natives-party.md), [script-natives-progress.md](script-natives-progress.md) (UnlockCharacter, UnlockAbility); the ability test and the use markers: [use-spot.md](use-spot.md).

## Current state

- A disguise is a character class. `tablescharacters.bxls` sheet 0 has eight classes (0 police, 1 robber, 2 construction, 3 farmer, 4 astronaut, 5 miner, 6 fireman, 7 civilian); the first row of each class (9, 19, 29, 39, 49, 59, 69, 79) is Chase's disguise, and all rows of a class share one ability set. [§2026-11-12T09:58:06](#ts-20261112-095806)
- The party manager 0x51b248 holds up to ten members (id bytes, flags, entity pointers); the members are the entities `Player1.Player01` and following of `levels/players.blvl`. A swap (0x245258) makes another member the player entity. [§2026-11-12T09:58:06](#ts-20261112-095806)
- At level start `Player1.Player01` is the player; the party is the eight ids of the TablesLevels row (+0x43..+0x4a, or the slot's +0x5201 when slot +7 bit 0 is set) plus the row's extras, each given the next member entity, and every id after the first whose group bit is clear is removed: a story level starts with Chase (9) usable and slots 1 to 7 removed. The add, removal, cycle, swap bookkeeping, SetPartyCharacter, UnlockCharacter and LockCharacter are transcribed. [§2026-11-12T11:17:20](#ts-20261112-111720)
- A swap is asked for by the D-pad (0x167554: right is next, left is previous, through 0x235110), by the lower-screen portrait code 0x2d9f84, by scripts (SetPartyCharacter, SuperFreeplaySwap through 0x1ece78), by the level entry 0x2e6264 (the character stored in the session) and by the character swap pad (object kind 126, player states 234 to 236). [§2026-11-12T09:58:06](#ts-20261112-095806)
- UnlockCharacter 0x263918 gives a disguise: it sets one bit per class in the save byte slot +0x51bb and adds the class's first row to the party. [§2026-11-12T09:58:06](#ts-20261112-095806)
- The missions up to 1-4 never swap: lvl_01 calls `SetPartyCharacter(Player1, 9, 1)` (Chase already is character 9) and the only swap of the level goes to character 16 (plain clothes). [§2026-11-12T09:58:06](#ts-20261112-095806)
- Per mission: plain clothes come from the locker of mission 1-9 (`CharacterLockerOpened`: UnlockCharacter(16), SuperFreeplaySwap(Player1.Player01, 16), SetPartyCharacter(Player1, 16, 0)); each class from an object trigger that unlocks its first row and swaps to it (robber `2-2_SO_RobberToken`, construction lvl_03, farmer 4-1, miner 6-1, astronaut 5-2, fireman 7-1), locked again by that mission's start script and unlocked by the startup scripts of later levels. [§2026-11-12T11:17:20](#ts-20261112-111720)
- Open: the names of the abilities, the lower-screen code, the swap pad, the swap gate's state tests, the entity side of the swap, the freeplay swap reader and everything listed under Unknowns.

<a id="ts-20261112-095806"></a>

## 2026-11-12T09:58:06+01:00 — First map of the disguise system (static)

Corrected by [disguise.md §2026-11-12T11:17:20](#ts-20261112-111720): the walk count +2 is written by 0x1b1c90; the level entry's row is 84 bytes; OnSceneEnter is asked for only when the member has no startup script.

Worker 20261112-trafficfeed-D, 2026-11-12. Evidence: llvm-objdump (`--triple=armv6k`) on `artifacts/game/game.arm.elf` with a literal-pool, `adr` and branch-target scan of the image; `build/chase-tabledump tables/tablescharacters.bxls`; the level files through `tools/chase_native.py` (`level_objects`, `level_census` with the main levels only). REA could not open the image in this session (no usable analysis provider), so no REA result is used. No emulator. Statements marked "inferred" are not read from code.

### Character classes and abilities (`tables/tablescharacters.bxls`)

Sheet 0 (192 rows of 40 bytes, table pointer `*0x372e10`): name pointer +0, word +4 (the price column: 0 for the eight first rows but 79), ability set +15 to +22 (64 bits, [use-spot.md §2026-10-20T21:58](use-spot.md#ts-20261020-215832)), linked character ids +0x22 and +0x23 (0x23b388). The class is the column after the second flag column of the dump; by the column order it is byte +0x1d (inferred: the columns before it land on the known offsets +15, +0x17 and +0x18; no reader of +0x1d was read).

| Class | Name (art names) | First row | Other rows | Ability set (+15 to +22) | Abilities beyond the common ones |
|---|---|---|---|---|---|
| 0 | police (`cop`) | 9 `police` | 10 to 18 (16 is `chase_plainclothes_cop`) | 77 ce 1a 02 03 c0 03 20 | 2, 20, 25, 33 |
| 1 | robber | 19 `robber` | 20 to 28 | 7b ce 1a 04 03 c0 83 21 | 3, 20, 26, 33, 55, 56 |
| 2 | construction (`builder`) | 29 `construction` | 30 to 38 | f3 ce 0a 80 01 c0 03 22 | 7, 31, 57 |
| 3 | farmer | 39 `farmer` | 40 to 48 | f3 ce 3a 20 03 c2 03 24 | 7, 20, 21, 29, 33, 41, 58 |
| 4 | astronaut (`astro`) | 49 `astronaut` | 50 to 58 | f3 ce 5a 10 03 c0 03 28 | 7, 20, 22, 28, 33, 59 |
| 5 | miner | 59 `miner` | 60 to 68 | 73 ce 0a 08 41 e0 03 20 | 27, 38, 45 |
| 6 | fireman (`fire`) | 69 `fireman` | 70 to 78 | f3 ce 1a 40 01 c4 03 30 | 7, 20, 30, 42, 60 |
| 7 | civilian (`generic`) | 79 `cherrytreehillsgang1` | 80 to 140 | f3 ce 0a 00 01 c0 03 20 | 7 |

- Common to all eight sets: abilities 0, 1, 4, 5, 6, 9, 10, 11, 14, 15, 17, 19, 32, 46 to 49, 61. Rows 1 to 8 (`custom1` to `custom8`) carry the sets of classes 0 to 7 in this order.
- The other rows of a class (shop characters with a price) have the same ability bytes as its first row; rows 141 and above are AI and story characters with class values outside 0 to 7 (for example 42, 18, 84, 12) and other sets.
- The use-marker texture switch of 0x292388 ([use-spot.md §2026-10-20T21:58](use-spot.md#ts-20261020-215832)) names the class of an ability: 2, 25 police; 26, 55, 56 robber; 27, 38 miner; 28 astronaut; 29, 58 farmer; 30, 60 fireman; 31, 57 construction; 21 chicken, 35 pig. That agrees with the table above.
- Sheet 2 (64 ability rows of 8 bytes): byte +6 marks an ability that the save can lock (2, 38, 46 to 49, 54 to 60), byte +7 is 2 for abilities 2 and 46 to 49, 7 for 38, 3 for 55, 6 for 56 and 59, 4 for 57, 5 for 58, 8 for 60 (meaning not read); byte +4 is 5 for 9 to 14, 6 for 21 and 22, 4 for 34 and 38, 3 for 35 and 41, 2 for 36.
- Sheet 5 lists the use-marker textures by class: `usegeneric`, `usepolice`, `userobber`, `useminer`, `useastronaut`, `usefarmer`, `usefireman`, `useconstruction`.
- Class art in the executable: `heartsBG_<cop|robber|miner|astro|farmer|fire|builder|generic>.png` (HUD), `portrait_border_<cop|robber|miner|astro|farmer|fire|builder>.png` and `Sprites/portraits/Locked.png` (lower-screen map buttons), `Token_<Police|Robber|Farmer|Miner|Astronaut|Construction|Fireman>.tga` (character token, loaded by 0x1a049c), `Lineup_01_Civilian` to `Lineup_08_Fireman` (0x3135bc), `UI_Class_Civilian.png` under `Sprites/UI_Shop/CharacterClasses` (0x31ef14).
- The HUD set-up 0x17bb14 loads the eight heart backgrounds into `0x51920c + 0x248 + 4 * class` (cop +0x248, robber +0x24c, builder +0x250, farmer +0x254, astro +0x258, miner +0x25c, fire +0x260, generic +0x264), the class order of the table. The same object holds the party portraits at `+0x268 + 4 * slot` (0x262320: `sprites/portraits/<row name>.png`, `sprites/portraits/Question.tga` when the file is missing).

### Party manager 0x51b248

| Offset | Size | Content |
|---|---|---|
| +0 | u16 | number of usable members (raised by the add 0x22dbcc, lowered by the removal 0x22db24) |
| +2 | u16 | number of slots the cycle 0x235110 walks and SetPartyCharacter accepts (writer not found) |
| +4 | 10 bytes | character id of each slot (row of sheet 0), 0 for an empty slot; 0x2457bc(id) returns the slot index, -1 for no match or id 0 |
| +0xe | 10 bytes | per slot an id that stands for another slot (written by the swap when its sixth argument is set, read by the cycle) |
| +0x18 | 10 bytes | 1 after the member was removed (0x22db24); such a slot is skipped unless cheat byte 0x375961 has bit 0 |
| +0x22 | 10 bytes | second skip flag, cleared by the add |
| +0x2c | 10 words | entity of each slot |
| +0xbc, +0xc6 | 10 bytes each | +0xbc cleared by the add; +0xc6 read by the D-pad code (below) |
| +0xd0 | 10 bytes | the entity's attribute `CharacterClass` (0x2451d0 in the add) |
| +0xdc, +0xe0 | words | handles played at a swap and at an add (0x23fa54), cleared by the reset 0x1677b4 |

- Reset 0x1677b4: clears +0 and the byte lists; skipped parts when the level byte `0x5163e8 + 0x26` is 9. The add 0x22dbcc returns at once in level 9.
- Add 0x22dbcc(entity, id, flag): id 0 means the entity's own character (`component + 0x2cc`). A slot that already holds the id gets the entity and its skip flags cleared; otherwise the first empty slot of the first eight takes id and entity and its portrait is loaded (0x262320). Callers: AddCharacterToParty 0x29151c (then 0x23ae84(75, object)), AddCharacterToPartyEnum 0x2b4a50, UnlockCharacter, the unlock screen 0x1a17e0 and 0x171bb0. The three AddCharacterToParty natives and RemoveCharacterFromParty do nothing of this when the byte 0x37751c is 1.
- Removal 0x22db24(id) (RemoveCharacterFromParty 0x2b9dc4, LockCharacter 0x1cda60, 0x171bb0): when the member is the player it first cycles to the next member (0x235110), then sets the +0x18 flag and clears the entity word. 0x23b388(id) (RelockChasePlainClothes, LockCharacter) clears the save bits of the id and of its linked rows (+0x22, +0x23).
- 0x263224(a, b): replaces id a by id b in its slot (returns 1), or swaps to b when b already has a slot (returns 0). 0x29ee04(entity, id) uses it and then rebuilds the entity from `levels/chars/char_<row name>.lvl`.
- The members are level objects: `levels/players.blvl` has `Player01` to `Player16` of class `chars/char_player` without parameters, next to the ability props (`Ability_Handcuffs`, `Ability_Scanner`, `Ability_PickAxe`, `Ability_FireAxe`, `Ability_PneumaticDrill`, `Ability_JetPack`, `Ability_Char_Token` and others) and the weapons (`Weapon_GrappleGun`, `Weapon_EnergyGun`, `Weapon_WaterCannon` and others).
- Natives that only ask: IfPartyCharacter 0x26fc98 (runs its script argument when any usable member has the ability bit of its first argument), IfPlayerCharacter 0x27bc58 (the same for the player, test 0x23a70c).

### The swap 0x245258(slot, index, a3, a4, quiet, updateStandFor, noScripts)

Read before for SetPartyCharacter ([script-natives-world.md §2026-10-18T11:01](script-natives-world.md#ts-20261018-110140)); confirmed and extended:

- Returns 0 when the index is 255 or is the slot of the player's character. Otherwise, when 0x2254c4() is not 0, it runs 0x2253b8 on the player and releases the object at `component + 0x170` (not read further). Unless `quiet` is set, and when the clock (0x241098) is past `0x375d44 + 0xc`, it spawns the effects 7 and 241 at the player's node (0x225248) and sets that gate to now + 15.
- It looks up the member entity `Player<1|2>.Player%02d`, copies heading words +8 / +0xa and the state byte +0x2ce, runs 0x224f54 (the member becomes the player entity 0x3772c0 / 0x3772b8), and unless `quiet` is set asks for the member's `startup` script and, unless `noScripts` is set, `OnSceneEnter<level number>` (0x224eb4); when both calls return 0 it calls 0x23fa54 on the handle `manager + 0xe0`.
- The new player entity takes state 1 (0x2414b4), or the old state when that was 2 or 3; the byte 0x375d45 gets the character id of the player entity at that point; the carried thing (`component + 0x13c`) moves over; the byte 0x375d44 gets twice the value of 0x23ca4c.
- Callers: SetPartyCharacter 0x27beb4, the D-pad code 0x167554, the cycle 0x235110, 0x263224, the pending-record reader 0x1ece78, the lower-screen code 0x2d9f84 and the level entry 0x2e6264.

### Asking for a swap

- **D-pad (0x167554, called from the gameplay update 0x2e5158).** The request word is `0x375d44 + 8` (-1 none). When the swap gate 0x235358(0) fails the request is dropped. When it passes, a request from the previous update is carried out: 1001 calls the cycle forward (0x235110(0, player, 0)), 1000 backward (0x235110(1, player, 0)), any other value is a party index for 0x245258; a successful cycle sets a 5-update countdown at `+0x10` and calls 0x1719bc. Then, unless the byte 0x377531 is set, a new request is read from the pad device (`*0x3769e4`, 20-byte channels from +0x14, the pressed halfword at +16): the channel in the word 0x3769f4 pressed, or 0x263200() (signed byte `0x51a224 + 112`) not 0, asks for 1001; else the channel in 0x3769f0 asks for 1000. The channel words were read as 14 and 15 in [vehicles-driving.md §2026-10-15T09:30](vehicles-driving.md#ts-20261015-093035), the D-pad left and right of [oracle-routes.md §2026-10-15T16:07](oracle-routes.md#ts-20261015-160734): right is the next disguise, left the previous one. With the first test true and the channel in 0x376a08 also pressed, the request becomes the first slot whose byte `manager + 0xc6` is set while the current slot's byte is clear.
- **Cycle 0x235110(direction, player, flag).** Walks the slots from the current one, forward or backward with wrap-around over the count at manager +2, and takes the first slot with an id, no removal flag (or cheat bit 0) and no second skip flag; slot 7 is also taken when bit 7 of the save byte slot +0x51bb is set. The stand-for ids at +0xe redirect the choice. Details of the redirect and of the argument at stack +0x18 are not transcribed.
- **Swap gate 0x235358.** False when 0x1a21ec(player) is 0, when the words 0x37689c and 0x3768c4 are equal, when 0x246128() or 0x24255c(player) is not 0, when the byte 0x375ef0 is set or bit 2 of `[component + 0xe0] + 0x130` is set; the following tests on the player's state word (`component + 0x6c`, values 1 and 3 seen) are not read.
- **Lower screen (0x2d9f84, called from 0x2ddd64).** After the gate 0x235358 it starts a storyboard on one of eight items (40-byte records from `+660`), calls 0x245258(0, item, 0, 1, 0, 0, 0) and stores 0x21d340(player) at `+0x46c`. The set-up 0x2dc590 loads the seven class portrait borders into `0x375b0c + 8` and following and `Locked.png` into `+0x24`. Which touches lead here is not read.
- **Scripts.** SetPartyCharacter; SuperFreeplaySwap and RelockChasePlainClothes write the pending record `0x5163e8 + 0xc8` (entity) / `+0xcc` (id) of [script-natives-party.md §2026-10-19T15:53](script-natives-party.md#ts-20261019-155302). Its reader is 0x1ece78 (called from the gameplay update 0x2e5158, the level entry 0x2e6264 and 0x2f67fc): it rebuilds the entity as that character (0x29ee04), reloads the portrait, sets bit 0 of the byte `0x51b6b0 + 7`, copies the eight party ids `manager + 4` to `0x5206b0 + 0x201` and swaps to the character.
- **Level entry 0x2e6264.** Unless the level row (28 bytes, `*0x375914`) has bit 1 of byte +5 or a non-zero byte +4, it swaps to the party slot of the character id stored in byte +0xbb of its argument (taken as the session 0x5163e8: inferred).
- **Character swap pad.** 0x182cc0 registers object kind 126 (handlers 0x19d018, 0x18e2e4, 0x1ab7e4, 0x19d35c, 0x19d224, 0x1ab808, 0x19d220) and the player states 234 "CHARACTER SWAP PAD ENTER", 235 "... SWAP" and 236 "... EXIT". The update of state 235 (0x2b1f48) sets the requested state 5 of the pad the player uses (`component + 0x130`). The pad's class name and its handlers are not read; kind 126 is not in [object-kinds.md](object-kinds.md).

### Getting a disguise and the save

- UnlockCharacter 0x263918(id): for the ids 9, 19, 29, 39, 49, 59, 69 it sets bit 0 to 6 of the byte 0x52086b, which is slot +0x51bb (slot copy at 0x51b6b0, [save-format-state.md](save-format-state.md)); for these and for 16 it sets the unlock bit when not yet set ([script-natives-progress.md](script-natives-progress.md), batch 27); and, except for 16, it adds the id to the party with the entity of the slot that already holds it (`Player1.Player%02u`, slot + 1) or no entity.
- The save also keeps the party ids (slot +0x5201 to +0x5208 by the address arithmetic above; inferred, the save writer was not read) and the ability lock set at slot +0x51ee ([use-spot.md §2026-10-20T21:58](use-spot.md#ts-20261020-215832)).
- The unlock screen 0x1a17e0 (object 0x5182b8) adds its entity to the party when its animation value passes 1.0.

### Uses in the eight main levels

`level_census` with the main levels only (the `*_main.blvl` of lvl_00 to lvl_07), calls / levels: SetPartyCharacter 27 / 7, UnlockCharacter 33 / 7, LockCharacter 8 / 6, UnlockAbility 53 / 7, LockAbility 2 / 2, RelockChasePlainClothes 3 / 1, SuperFreeplaySwap 4 / 3, ShowCharacterIntro 7 / 6, CharacterShop 1 / 1, GetCharacterUnlocked 1 / 1. No call of AddCharacterToParty, AddCharacterToPartyEnum, AddCharacterToPartyFade, RemoveCharacterFromParty, IfPartyCharacter, IfPlayerCharacter or ChangeCharTokenTexture in these files.

- `pickups/pickup_charactertoken`: 106 objects (16 in lvl_01, 15 in each of lvl_02 to lvl_07, none in lvl_00).
- Objects named after a disguise: lvl_00 `Use_CharShop` (`props/prop_simpleuse`) with `Use_CharShopScript`, and the script object `UNUSED_Disguise_Greeting_Script`; lvl_01 `1-9_UndercoverDisguiseCupboard` (`props/map_useobjectwaypoint`) and `1-9_FakeCharToken`; lvl_02 `2-2_SO_RobberToken`; lvl_04 `4-1_Waypoint_FarmerDisguise` and `4-7_Wardrobe`; lvl_06 `Reveal_ConstructionToken01_SO`. So the first new disguise by name is the robber of mission 2-2 (inferred from the names; the scripts were not decoded).
- Class objects that need a class ability exist in every level (`props/ability_*`, 78 classes; for example `ability_freeplay_robber_atm` 24, `ability_freeplay_miner_gem01` 28, `ability_freeplay_fireman_cat` 28, `ability_freeplay_astronaut_alien01` 28, `ability_freeplay_construction_repair01` 24, `ability_axe_door` 18, `ability_drillspot_hole` 20, `ability_jetpackpad` 40, `ability_teleport_pad` 42).
- Missions 1-1 to 1-4 (lvl_01): the calls are `SetPartyCharacter(Player1, 9, 1)` nine times, which change nothing while Chase is character 9, one `SetPartyCharacter(Player1, 16, 0)` and the three RelockChasePlainClothes calls, one of them in mission 1-1 ([script-natives-world.md §2026-10-18T11:01](script-natives-world.md#ts-20261018-110140), [script-natives-census.md](script-natives-census.md)); mission 1-4 calls UnlockAbility 54, 46, 48, 49 and 47 ([scripts-missions-1-4.md](scripts-missions-1-4.md)), abilities of every class. No mission up to 1-4 needs a swap; they need the police class look of the HUD and the ability tests of row 9.

### Unknowns

- The script calls with their arguments per mission (which script gives which disguise, where character 16 is put on): the Python binding has no script decoder with arguments.
- The names of the abilities (sheet 2 has no names) and the action behind each class ability; bytes +4 and +7 of sheet 2; the row bytes +0x20 and +0x21 (+0x21 is 2 for the rows 9, 16, 19, 29, 39, 49, 59, 69, 79) and the reader of the class byte.
- The lower-screen code 0x2d9f84 / 0x2ddd64 (touch areas, storyboards, the locked look) and the writer of the byte `0x51a224 + 112`.
- The channel number in 0x376a08 and the writer of the bytes `manager + 0xc6`.
- The rest of the swap gate 0x235358, of the cycle 0x235110 and of 0x1719bc; the function 0x200f94 that uses the attribute `CharacterSwitchAnim`.
- The swap pad (class name, handlers, states 234 to 236, where it stands), the character shop (CharacterShop 0x1cd8e4, `Use_CharShop`), the freeplay swap and the character token pickup's effect on the party.
- What differs between the member entities beyond the character row (models, weapons, animation sets per class) and how `Player01` to `Player16` get their characters at level start.
- Who writes the byte +0xbb read at the level entry and the party ids of the save, and how a loaded save fills the party.

<a id="ts-20261112-111720"></a>

## 2026-11-12T11:17:20+01:00 — Party set-up at level start, cycle, swap bookkeeping and the disguise calls per mission

Worker 20261112-enemy-B, 2026-11-12. Evidence: llvm-objdump (`--triple=armv6k`) of `artifacts/game/game.arm.elf` with literal pools and strings read from the image; `build/chase-tabledump tables/tableslevels.bxls`; the script calls with their arguments listed from the eight `*_main.blvl` through `chase_assets` (`parseLevel`, a throw-away listing program, not committed). REA was not used. No emulator. Port: `src/runtime/party_manager.*` ([disguise.md](../plans/features/disguise.md)). [Corrects §2026-11-12T09:58:06](#ts-20261112-095806): the writer of the walk count manager +2 is 0x1b1c90 (below); the level row read by the level entry 0x2e6264 is 84 bytes (index × 84, the TablesLevels rows), not 28; in the swap the member's `OnSceneEnter<n>` is asked for only when its `startup` call returned 0.

### How the members get their characters at level start

- The level set-up 0x2e41e4 calls 0x19f41c, which looks up `Player1.Player01` and makes it the player entity (0x3772c0 and 0x3772b8, through 0x224f54). Then, unless the byte 0x377530 is set (writer not read), the reset 0x1677b4, the party fill 0x171bb0 and 0x17c838 (a 12-byte record per member `Player%d.Player%02d` (1, 1..10) at 0x51b158 with the member's words +0x24, +0x7c, +0x2c; use not read).
- The list 0x519c38 (10 bytes) is built earlier by 0x1b1c90, called from the level change 0x20a4d4(?, level), which stores the level in session byte +0x26 and in +0x25 the level for rows 9 and above, else 0. 0x20a4d4 passes the slot's party ids `0x5206b0 + 0x201` (slot +0x5201) when slot +7 bit 0 is set; otherwise it first copies the eight ids of the TablesLevels row (+0x43..+0x4a) into slot +0x5201..+0x5208 and passes the row. 0x1b1c90 clears the list and copies the eight ids, or with the cheat byte 0x375961 bit 0 writes 9, 19, 29, 39, 49, 59, 69, 80; it counts the listed ids up to the first 0 (count at 0x375908 + 1) and formats `chars/char_<row name>` for each (64-byte names at 0x519990, loaded through 0x267518); then it appends the row bytes +0x4b and +0x4c (stored at 0x37590c / 0x37590d) up to the first 0 (count at 0x375908 + 3). Manager +2 = listed + appended. It also writes a count to 0x375908 + 2 and manager +0: 8 with the cheat, the number of set bits 0..7 of slot +0x51bb when slot +7 bit 0 is set, else row byte +0x42; the reset clears manager +0 again.
- 0x171bb0: with slot +7 bit 0 it copies slot +0x5201..+0x5208 over the first eight list bytes. 0x375d45 / 0x375d46 = list bytes 0 and 1. For each of the ten list bytes that is not 0 it looks up the next member (`Player1.Player%02u`, numbered from 01 over the non-zero bytes only), calls the add 0x22dbcc(member, id, 0) and writes the id to the member's component +0x2cc (its character). For list positions 1 to 9, unless the cheat bit is set, it removes the id (0x22db24) when bit `position` of the bytes from 0x52086b is clear: positions 0..7 are the group bits slot +0x51bb, positions 8 and 9 read bits 0 and 1 of slot +0x51bc.
- TablesLevels sheet 0: rows 1 to 8 (police station and the seven story levels) have +0x42 = 1, ids 9 19 29 39 49 59 69 80 and no extras; row 0 (`lvl_the_city`) has +0x42 = 7 with the same ids; row 9 (`lvl_customisation`) has 4 and ids 1 2 3; the test rows 10 to 27 have 7 and the ids 9 to 69 or 9, 19. So on a new game (group bit 0 only) every story level starts with eight slots of which slot 0 (character 9) is usable and slots 1 to 7 are removed; walk count 8; the player is `Player1.Player01` with character 9.

### The manager functions, as transcribed into the port

- Reset 0x1677b4: manager +0 = 0; the ten ids (+4), removal flags (+0x18), second skip flags (+0x22) and stand-for ids (+0xe) cleared; the swap effect gate `0x375d44 + 0xc` = 0; unless in level 9 also +0xdc / +0xe0 and a registration 0x23cb90(0x518218, ..., 0x187cb0, 6). The walk count +2 and the entity words +0x2c stay.
- Lookup 0x2457bc(id): the first slot whose id byte equals the id, -1 for none and for id 0 (a halfword argument compared with the bytes, so 265 finds nothing).
- Add 0x22dbcc(entity, id, flag): nothing in level 9 (session +0x26); id 0 is the entity's +0x2cc. A slot that holds the id (all ten searched) takes the entity; manager +0 rises when its removal flag was set; both skip flags are cleared. Otherwise the first slot with id 0 among the first eight takes id and entity (removal flag and +0xbc cleared, +0xd0 = the entity's `CharacterClass` attribute through 0x2451d0, the portrait 0x262320 when the row has a name) and manager +0 rises. Then, with an entity: when manager +0 > 1 and the flag is set it sets bit 2 and clears bit 9 of the entity's +0x10 flags, may enter it into the three-word list 0x51b330 (byte 0x375d47 + n = 0x23ca4c()) and plays the handle +0xdc (0x23fa54); else it tests the entity against the level object `Player1` (0x1b3d4c) and otherwise calls 0x2428c0.
- Removal 0x22db24(id): returns 0 when no slot holds the id; when it is the slot of the player's character the cycle 0x235110(0, player, 0) runs first; then manager +0 falls unless the removal flag was already set, the flag is set and the entity word cleared; returns 1.
- Cycle 0x235110(backward, player, flag): start = the slot of the player's character; when that is not below the walk count, the first slot below the walk count whose stand-for id is not 0 and equals the id at the start index becomes the start. Then up to walk-count steps from the start (forward: +1, back to 0 at the walk count; backward: from 0 to the walk count - 1). A slot is taken when its id is not 0, its removal flag is clear (or the cheat bit 0 is set) and its second skip flag is clear, or when it is slot 7 and bit 7 of slot +0x51bb is set. Unless the byte 0x37751c is 1 or the flag is set, the start slot (and 0xffff) is passed over. A taken slot with a stand-for id leads to that id's slot instead (passed over when that is the start). Then 0x245258(0, slot, 0, flag, 0, 0, 0), whose result it returns; 0 when nothing is taken.
- Swap 0x245258(player, index, a3, a4, quiet, updateStandFor, noScripts): returns 0 when the index is 255 or the slot of the player's character. 0x2254c4() is "session byte +0x25 is not 9"; with it the player's held state is undone first (0x2253b8, the object at component +0x170 released). Effects 7 and 241 at the player's node only when 0x2254c4() is set, `quiet` is 0 and the update count 0x241098 is above the gate; the gate becomes update count + 15. The member is `Player<k>.Player%02d` with k = 1 when the player entity belongs to the level object `Player1` (0x1b3d4c), else 2, and number index + 1 (a loop over ten numbers: an index of 10 or more finds no member). When the member exists and 0x2254c4() is set it becomes the player entity (counter 0x376668, heading +8 / +0xa, 0x224f54, the state byte +0x2ce handed over and set to 255 on the old one); unless the word 0x3768c4 equals 0x37689c or 0x376898, or `quiet` is set, the member's `startup` script is asked for (0x224eb4) and, only when that returns 0 and `noScripts` is 0, `OnSceneEnter%d` (number from 0x376c78 + 44); when that also returns 0 the handle manager +0xe0 is played. The new player takes state 1, or the old state when it was 2 or 3 (0x2414b4). In every case 0x375d45 = the player's character afterwards, and with `updateStandFor`: when manager +0 > index the index's stand-for id is cleared; else when manager +0 <= the old slot (signed) every stand-for id below the walk count that equals the old slot's id becomes the index's id; else the old slot's stand-for id becomes the index's id (for an old slot of -1 that byte is manager +0xd, the id of slot 9). Then the carried thing and the bytes +0x315 / +0x317 of the new player, 0x22aab8, 0x199024, 0x375d44 = 2 × 0x23ca4c(); returns 1. The arguments a3 and a4 are not read in the parts transcribed.
- SetPartyCharacter 0x27beb4: index = (u16) 0x2457bc(character & 0xffff); nothing when the walk count is not above it; `Player1` calls 0x245258(0, index, 0, 1, flag, 0, 0), `Player2` 0x245258(1, ...). The removal flag is not tested, so a removed member can be swapped to.
- UnlockCharacter 0x263918 (confirmed): the add runs for 9, 19, 29, 39, 49, 59, 69 also when the character was unlocked before; the popup 0x205c18(id, 1, 0) and the counter 0x518024 + 0x30 follow for each of the 20 halfwords at session +0x11a that equal the id, only on a first unlock.
- LockCharacter 0x1cda60: for the same seven ids 0x23b388(id) (save bits, linked rows), the removal 0x22db24(id), then the group bit of the id is cleared.
- 0x1e8a50 also writes the party into the slot: it sets slot +7 bit 0 and copies the manager ids to slot +0x5201..+0x5208, then overwrites +0x5208 with the id of its record (+0xc).

### Disguise calls per mission (main levels)

From the scripts of the eight `*_main.blvl` with their arguments; script names resolved from the trigger, object and argument strings of the level, the others by hash. Of the other `.blvl` files only test levels (`lvl_test_playground_*`) call these natives.

| Level | Script (trigger) | Calls |
|---|---|---|
| lvl_01 | `CharacterLockerOpened` (object trigger, the locker of mission 1-9) | GetCharacterUnlocked(16, PlainclothesAlreadyCollected), UnlockCharacter(16), SuperFreeplaySwap(Player1.Player01, 16), SetPartyCharacter(Player1, 16, 0) |
| lvl_01 | `Police_ExtToInt`, `Police_Station_Transition_Upper`, startup (hash e48e50f6) | RelockChasePlainClothes() |
| lvl_01 | `1-1_MissionStart` (twice), `1-1_SO_MissionEnd`, `1-2_Bone_Trigger`, `1-3_Helipad_Built` and four by hash | SetPartyCharacter(Player1, 9, 1) |
| lvl_02 | `2-2_SO_RobberToken` (object trigger) | UnlockCharacter(19), SetPartyCharacter(Player1, 19, 0) |
| lvl_02 | `2-2_MissionStart` / `2-2_MissionSkip` | LockCharacter(19) / UnlockCharacter(19) |
| lvl_02 | `Startup`; `2-9_BossFightComplete` (CharacterArrested of 2-9_Birdman) | SuperFreeplaySwap(Player1.Player01, 16); SuperFreeplaySwap(Player1.Player01, 9) |
| lvl_03 | `SO-Class_Locker`; `3-3_Worksite_Banter`; hash b714029d | SetPartyCharacter(Player1, 29, 0) and UnlockCharacter(29); SetPartyCharacter(Player1, 29, 0); SetPartyCharacter(Player1, 9, 0) |
| lvl_03 | `3-2_MissionSkip`; startup | UnlockCharacter(29); LockCharacter(29), UnlockCharacter(19) |
| lvl_04 | `4-1_FarmLocker_Opened_SO`; `4-1_MissionStart`; `4-1_MissionSkip`, `4-6_Pig_Race_Waypoint_Entered`, `4-9_Castle_Fight_Start`, `4-9_MissionEnd`, hash e3cf07ae | SetPartyCharacter(Player1, 39, 0) and UnlockCharacter(39); LockCharacter(39); UnlockCharacter(39) |
| lvl_04 | startup; `4-X_SO_HeliPad_Built` | LockCharacter(39), UnlockCharacter(19), UnlockCharacter(29); SetPartyCharacter(Player1, 9, 1) |
| lvl_05 | `6-1_SO_Miner_ClassLocker`; `6-9_SO_Get_Dynamite`; `6-1_MissionSkip` | SetPartyCharacter(Player1, 59, 0) and UnlockCharacter(59); SetPartyCharacter(Player1, 59, 0); UnlockCharacter(59) |
| lvl_05 | startup; `6-5_Dcam_Goon_Caught`, `6-9_EnterBossArena` | UnlockCharacter 19, 29, 39, 49, LockCharacter(59); SetPartyCharacter(Player1, 9, 1) |
| lvl_06 | `5-2_SO_AstronautGet`; `5-9_FreezeGun_Pickup`; `5-2_MissionStart` / `5-2_MissionSkip` | UnlockCharacter(49), SetPartyCharacter(Player1, 49, 0); SetPartyCharacter(Player1, 49, 0); LockCharacter(49) / UnlockCharacter(49) |
| lvl_06 | startup; hash 7a7d1caf; `5-11_SO_JetPack_Pad_Final_Land` | LockCharacter(49), UnlockCharacter 19, 29, 39; SuperFreeplaySwap(Player1, 9), SetPartyCharacter(Player1, 9, 1); SetPartyCharacter(Player1, 9, 0) |
| lvl_07 | `7-1_AwardFireman`; `7-1_MissionStart` / `7-1_MissionSkip`; hashes 09174e41, 6ca547a7, b13b4b6f | UnlockCharacter(69), SetPartyCharacter(Player1, 69, 0); LockCharacter(69) / UnlockCharacter(69); SetPartyCharacter(Player1, 69, 0); UnlockCharacter(69); SetPartyCharacter(Player1, 9, 0) |
| lvl_00 | `Use_CharShopScript` (object trigger) | CharacterShop(Arg1) |

- Pattern: each class is given by an object trigger that unlocks the class's first row and swaps to it (robber in mission 2-2, construction in lvl_03, farmer in 4-1, miner in 6-1, astronaut in 5-2, fireman in 7-1); the `MissionStart` script of that mission locks it first and its `MissionSkip` unlocks it; the startup script of each later level unlocks the classes of the levels before. Character 16 (plain clothes) is never added by UnlockCharacter (16 is excluded from the add); it reaches the party only through SuperFreeplaySwap (the pending record, then 0x29ee04 / 0x263224 replace an id in its slot), so `SetPartyCharacter(Player1, 16, 0)` of lvl_01 finds no slot unless that swap ran first.
- Abilities in the same scripts: mission 1-4 (`1-4_MissionSkip`, `1-4_Spawner_06_Script`, hash c363da24) 46 to 49 and 54; `1-8_MissionStart` 2, 46, 49; `2-5_TeachSafeCrack` 55; lvl_03 31 and 57; lvl_04 58; `6-9_SO_Get_Dynamite` 38; lvl_06 59 (LockAbility(59) in `5-9_MissionStart`); lvl_07 60 (LockAbility(60) in `7-8_MissionStart`); the startup scripts of lvl_04 and later unlock the abilities of the levels before.

### Unknowns

- The writers of the bytes 0x377530 (party kept at the set-up) and 0x37751c (cycle, AddCharacterToParty natives) and of session byte +0xbb (the character of the level entry swap); the words 0x3768c4, 0x37689c, 0x376898 that skip the member scripts.
- Whether `Player2.Player%02d` members exist (a second instance of `levels/players.blvl`) and what the member table of 0x17c838 is used for.
- The byte the cycle compares when the player's character has no slot (it indexes the id list with 0xffff, outside the manager).
- The swap arguments a3 and a4, the entity side of the swap (0x2253b8, 0x22568c, 0x23ddb4, 0x2afeac, 0x224d44, 0x22aab8, 0x199024) and of the add with a flag.
- What 0x1ece78 and 0x1e8a50 are called for (freeplay swap, character shop) and the rebuild 0x29ee04.
