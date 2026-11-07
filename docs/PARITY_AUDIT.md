# Gunk's Emerald against the Gunk's Platinum parity checklist

Checklist: `C:\Projects\gunk-reference\PARITY_CHECKLIST.md` (the bar is Gunk's Platinum v1.0.2). First audit
2026-11-05 on build 13a21380; updated 2026-11-06 on build e16c7317 and 2026-11-07 on build ed5ce754 (`roms/gunk-v11.gba`), where every
test named below was rerun (the gym run and the follower sweep ran on 06672103, which only lacks the Safari check). Evidence names the test, screenshot or `GUNK_CHANGES.md` row. Tests live in `emerald-tools/emu/suite`.

| Row | Item | Verdict | Evidence / what's missing |
|-----|------|---------|---------------------------|
| 1 | DexNav screen in Platinum's structure | **HAS** | Title bar with area + CAUGHT x/y, info panel (name, status, Found, level range, search level, chain, types, abilities or "Capture first!"), hint panel (R target, A, B), LAND/WATER/HIDDEN grids with "?" unseen, silhouettes for seen-not-caught, a ball on caught icons, cursor (`out/dn_registered.png`, row 19) |
| 1 | Searches on land and water | **HAS** | `dexnav_test.py` (land) and `dexnav_water_test.py` (surfing at the Petalburg pond) both PASS |
| 1 | Search levels, chains, better IVs/shinies/items | **HAS** | Search levels in PC storage free space, chain byte, potential stars and shiny rerolls by level/chain (row 19) |
| 1 | Rules (10 s, shake, chain) | **HAS** | 10 s, 2 shakes/s, timeout and other-species resets, flee resets, win/catch adds one (`dexnav_rules_test.py`). The START menu and the Bag pause the 10 s; the search bar hides under the menu and comes back (`dexnav_pause_test.py` 11/11) |
| 1 | Shown shiny = given shiny; forms; wild items, Compound Eyes | **HAS** | Peeked roll equals made Pokémon 60/60 (row 19). No map forms in Gen 3. A Compound Eyes lead raises the held-item rates (+25 any item, +15 rare) |
| 1 | No pause when a search starts | **HAS** | Rolls are peeked, the worst case (search level 255) starts in ≤ 1 s (was 5 s) |
| 1 | Every DexNav catch legal in PKHeX | **HAS** | Every DexNav Pokémon VALID as EncounterSlot3 (row 19, `legality_batch.py`). No searches in the Safari Zone, where a DexNav battle (any ball) would be illegal: all six areas with the extension refuse, with a flag-off control (`dexnav_safari_test.py` 18/18) |
| 1 | Key item before the first gym, usable from the Bag, registrable | **HAS** | DEXNAV key item given with the Pokédex in Birch's lab; USE from the Bag, REGISTER to SELECT (`dexnav_item_test.py` 6/6) |
| 1 | Only seen Pokémon can be chosen; fishing-only can't; hint says where to press R | **HAS** | Only seen species can be registered; fishing has no grid. "Found: Grass/Cave/Surfing/Hidden" on the info panel; searching the wrong way says "search it by surfing" / "in the grass" |
| 1 | Field search bar | **HAS** | Species, level, a move, ability, held item, potential stars, chain, SHINY mark, nature, search level (`out/dn_bar_far.png`, `out/dn_bar_near.png`) |
| 1 | Clear messages when R can't search | **HAS** | Nothing registered, not found here, not found nearby, search by surfing / in the grass, got away, too dark, signal lost (`dexnav_item_test.py`, row 19) |
| 1 | Second quick view | N/A | GBA has one screen |
| 2 | Every species follows, correct sprites | **HAS** | aarant's followers-legacy: all 386 Gen 3 species with Gen 3 overworld sprites (user approved) |
| 2 | Emotes and interactions | **HAS** | Follower talk: friendship lines, tired line, emotes (`follower_talk.py` 8/8, row 22) |
| 2 | Back to its ball while Surfing and where it can't follow | **HAS** | `follower_parity_test.py`: hidden while surfing, back on land |
| 2 | No object-ID clash or full-slot crash | **HAS** | Audit: local ID 254 clashes with nothing; a failed spawn is skipped. 512-NPC talk sweep, nothing froze |
| 2 | Behaves in cutscenes, Poké Ball animation | **HAS** | `scenesweep.py --follower`: after every story scene the follower is checked (out, visible, next to the player after a couple of steps). 183 scenes, 182 played, 169 good. The 13 others: the Contest Hall fills all 16 object slots, so the follower is skipped there (no crash). 4 house TV / SS Ticket scenes, the Route 104 sail and Route 119's rival, Rustboro's scientist and both Victory Road Wally triggers end at the same place, still in their script, on vanilla too. Routes 127-129 end at sea with the player surfing, where the follower is in its ball |
| 2 | Double battle in the lead; scripted walk during a screen effect | **HAS** | Double battle (Amy & Liv) with the follower out, back after: `follower_parity_test.py` 9/9. Scenes that walk the player during a screen effect (Mt. Pyre summit, Seafloor Cavern, Sky Pillar, Sootopolis, ...): the follower is out, visible and next to the player after all 10 such scenes (Mt. Pyre summit x3, New Mauville door, Sky Pillar outside and top, Seafloor Cavern Kyogre, Sootopolis Rayquaza, Contest Lobby x2) (`screen_effect_scenes.py` 10/10) |
| 3 | PC boxes for a living dex + spares | **HAS** | 16 boxes = 480 slots for 386 species (row 18, `box_test.py` 7/7) |
| 4 | Instant text | **HAS** | Row 3 |
| 4 | Faster EXP bar | **HAS** | Twice vanilla's speed, timed in the emulator (`exp_bar_test.py`): 0.55 vs 1.05 frames per pixel on a 44-EXP level, 0.73 vs 1.30 on a 78-EXP level (the short-level case was at vanilla speed until this round) |
| 4 | Auto-run | **HAS** | Row 5 |
| 4 | Faster eggs | **HAS** | Row 17 |
| 5 | Quick Run | **HAS** | Row 21 |
| 5 | Quick Ball widget | **HAS** | Row 21 |
| 6 | Reusable TMs / two registered items / bag sorting / Repel prompt / Endless Repel | **HAS** | Rows 8, 9, 10, 6, 7 |
| 7 | IV/EV view, Exp. All, vitamins to 252, trade evos, Move Tutor | **HAS** | Rows 11, 12, 13, 14, 16 |
| 7 | HMs without teaching | **HAS** | Cut, Rock Smash and Surf from the field without the move (`hm_field_test.py` 3/3) |
| 8 | Logo; header GUNK + short code | **HAS** | Row 1 and 1b (`GUNK EM`, BPEE) |
| 8 | BPS refuses wrong ROM, sha1s in notes, clean build reproduces | **HAS** | `emerald-tools/release/gunks-emerald-v1.0.bps` and `RELEASE_NOTES_v1.0.md` (remade for ed5ce754) |
| 8 | PokeDaisy address table | **HAS** | `docs/POKEDAISY_COMPAT.md` (generated by `tools/daisy_table.py` from build ed5ce754) |
| 8 | Test hooks only in test builds | **HAS** | The shiny test build is a separate ROM; the release has no warps or test saves |
| 9 | Berry, Feebas spots, swarms info | **HAS** | Berry trees say how many hours until fruit (row 23, `berry_hours_test.py`). Route 119: an empty cast says how many steps the nearest Feebas spot is, checked against the spots worked out from the source; a swarm shows on the DexNav title bar (row 24, `gen3_info_test.py` 3/3) |
| 10 | Save suite (saves, power cuts, damaged saves) | **HAS** | 4/4 every build. Full boxes, Hall of Fame: covered by `box_test.py` and the HoF layout check, not in the suite itself |
| 10 | Gyms, Elite Four, Champion to the Hall of Fame vs vanilla | **HAS** | `gym_hof_run.py`, one chained run on each ROM: the eight leaders, the Elite Four, Wallace, the Hall of Fame (saves), the credits, the title, CONTINUE. Every step matches vanilla (end, badges, game running, field free; game cleared after CONTINUE); flags differ only by FLAG_TEMP_HIDE_FOLLOWER in the Fortree and Mossdeep gyms (Gunk's hides the follower there), vars only by the lottery's random numbers |
| 10 | Story and talk sweeps vs vanilla | **HAS** | 244/244 story NPCs, 512/512 talk, all differences benign |
| 10 | Menus test | **HAS** | `menus_test.py`: every START entry opens and returns with the same heap blocks, SELECT DexNav too; Gunk's 17/17, vanilla 12/12 |
| 10 | Release ROM boots, intro, saves, continues | **HAS** | New game from the title on the release build, save suite on it |
| 10 | README feature list, release notes | **HAS** | README feature list updated with the DexNav and QoL items |

**Still open:** nothing on the checklist. Publishing v1.0 waits for the user's yes.
