# Gunk's changes: verdicts for Emerald

Every change from Gunk's Platinum, checked against what Emerald (USA) already does. "Port" means it's planned;
the Status column says where it stands. A change is only **Done** once it was checked in the emulator and the
save suite passed on that build.

| # | Change | Emerald today | Verdict | Status |
|---|--------|---------------|---------|--------|
| 1 | Title `GUNK EM` (ROM header) | `POKEMON EMER` | Port (game code stays BPEE so emulators keep the flash + RTC settings) | Written, not built yet |
| 2 | Save-safety suite (save cycles, crash during a save, damaged-save recovery) | — | Test harness | Harness ready; suite in progress |
| 3 | Instant text | Options: SLOW / MID / FAST, default MID | Port: add INSTANT, new games default FAST | Planned |
| 4 | Faster EXP bar | Fills 1 px per frame-step | Port | Planned |
| 5 | Auto-run | Running Shoes: hold B | Port: run by default, B walks (Running Shoes still needed) | Planned |
| 6 | Repel prompt | "Repel's effect wore off..." only | Port: offer another Repel of the same kind | Planned |
| 7 | Endless Repel | — | Port: new key item, toggled with USE | Planned |
| 8 | Reusable TMs | TMs are used up; HMs reusable | Port | Planned |
| 9 | Two registered items | One (Select) | Port: L or R uses a second item (L=A option stays respected) | Planned |
| 10 | Bag sorting | — | Port: sort the pocket by name / type / amount | Planned |
| 11 | IV/EV view in the summary | — | Port | Planned |
| 12 | Exp. All | Exp. Share (held item) | Port: new key item, toggled with USE | Planned |
| 13 | Vitamins to 252 EVs | Stop at 100 EVs per stat | Port | Planned |
| 14 | Trade evolutions without trading | Need a link trade | Port: by level / by using the item | Planned |
| 15 | HMs without teaching | Must teach the HM | Port: owned HM + compatible Pokémon + badge | Planned |
| 16 | Move Tutor | Move Relearner in Fallarbor (Heart Scale) | Port: free relearner on every Pokémon Center PC | Planned |
| 17 | Faster eggs | Retail egg cycles | Port: half the steps | Planned |
| 18 | More PC boxes | 14 boxes | 16 boxes (user's choice, `docs/PC_BOXES_PLAN.md`): 15 sectors per slot, Hall of Fame 33 teams in sector 30, no e-Reader Trainer Hill save. Old saves don't load | Written, not built yet |
| 19 | DexNav | — | Port | Planned |
| 20 | Following Pokémon | — | Port | Planned |
| — | Honey trees, Poké Radar info | Don't exist in Emerald | Skip | — |
| — | Running Shoes touch button, Poké Ball follower toggle | Don't exist / N/A | Skip | — |
