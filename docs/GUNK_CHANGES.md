# Gunk's changes: verdicts for Emerald

Every change from Gunk's Platinum, checked against what Emerald (USA) already does. "Port" means it's planned;
the Status column says where it stands. A change is only **Done** once it was checked in the emulator and the
save suite passed on that build.

| # | Change | Emerald today | Verdict | Status |
|---|--------|---------------|---------|--------|
| 1 | Title `GUNK EM` (ROM header) | `POKEMON EMER` | Port (game code stays BPEE so emulators keep the flash + RTC settings) | **Done** (header checked in every build) |
| 2 | Save-safety suite | — | `emerald-tools/emu/suite/suite.py`: 10 save cycles + reload, 75 power cuts during a save, 30 torn-save images, damage in each of the 15 sectors and the signature | **Done**: 4/4 on retail (control) and on every Gunk build |
| 3 | Instant text | Options: SLOW / MID / FAST, default MID | INST. after FAST in Options (Right on FAST); new games start on FAST. A message prints whole in one frame | **Done** (option cycle and a 3-frame message checked) |
| 4 | Faster EXP bar | 1 px per frame | 2 px per frame (`battle_interface.c`) | Done in code; not timed in the emulator |
| 5 | Auto-run | Running Shoes: hold B | Run by default once Mom gives the shoes; hold B to walk. Mom's and the manual's texts say so | **Done** (5 tiles in 48 frames running, 3 holding B, 3 without shoes) |
| 6 | Repel prompt | "Repel's effect wore off..." only | "Use another SUPER REPEL?" when the bag has one of the same kind (not in the Battle Pyramid) | **Done** (YES used one and set 200 steps) |
| 7 | Endless Repel | — | Key item (unused item slot 0x34), USE toggles it. Works like a Repel that never runs out | **Done** (no encounter in 150 grass steps with a Lv. 30 lead) |
| 8 | Reusable TMs | TMs are used up; HMs reusable | Teaching a TM keeps it | **Done** (TM29 taught, still 1 in the bag) |
| 9 | Two registered items | One (Select) | Registering a second key item puts it on SELECT and moves the old one to L (marked "L" in the bag). Not in L=A mode | **Done** (L and Select each toggled their item) |
| 10 | Bag sorting | — | Start in the bag sorts the pocket by item number (the game's order); TMs and Berries are already sorted | **Done** |
| 11 | IV/EV view in the summary | — | A on the Skills page cycles stats → IVs → EVs | **Done** (values checked against the party data) |
| 12 | Exp. All | Exp. Share (held item) | Key item (unused item slot 0x35), USE toggles it. Pokémon that didn't battle get half a battler's EXP. (before boosts) and the EVs; battlers get the normal amount | **Done** (15 EXP to the battler, 7 to the bench; off: 0) |
| 13 | Vitamins to 252 EVs | Stop at 100 | Up to 252 per stat | **Done** (27 HP Ups → 252) |
| 14 | Trade evolutions without trading | Need a link trade | Trade evolutions without an item evolve at Lv. 36; the trade items work like evolution stones. The Everstone still stops both | **Done** (Kadabra → Alakazam at 36, Onix + Metal Coat → Steelix) |
| 15 | HMs without teaching | Must teach the HM | A Pokémon that could learn a TM/HM in the bag can use its field move (party menu, and the field scripts: Cut, Rock Smash, Strength, Surf, Waterfall, Dive, Secret Power). Badges still needed | **Done** in the party menu (FLASH, CUT, DIG shown by compatibility); in-world uses not yet walked to |
| 16 | Move Tutor | Move Relearner in Fallarbor (Heart Scale) | MOVE TUTOR row on every PC: a free relearner | **Done** (taught Teleport, back to the PC menu, log off) |
| 17 | Faster eggs | Retail egg cycles | An egg cycle every 128 steps instead of 255 | **Done** (a 0-cycle egg hatched after 127 steps) |
| 18 | More PC boxes | 14 boxes | 16 boxes (user's choice, `docs/PC_BOXES_PLAN.md`): 15 sectors per slot, Hall of Fame 33 teams in sector 30, no e-Reader Trainer Hill save. Old saves don't load | **Done** for saving (suite 4/4 on the 15-sector layout); boxes 15-16 not yet browsed in the PC |
| 19 | DexNav | — | Port | Planned |
| 20 | Following Pokémon | — | Port | Planned |
| — | Honey trees, Poké Radar info | Don't exist in Emerald | Skip | — |
| — | Running Shoes touch button, Poké Ball follower toggle | Don't exist / N/A | Skip | — |

## How changes are checked

`emerald-tools/emu/suite/feat_lib.py` builds test states: the start state (the truck), a party and bag poked into
RAM (`mons.py` writes real Gen 3 Pokémon with encryption and checksums), and warps through the game's own
continue-game warp (Route 101 grass, Oldale's Pokémon Center, Littleroot with the Running Shoes scene due).
