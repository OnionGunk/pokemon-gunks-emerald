# Gunk's Emerald against the Gunk's Platinum parity checklist

Checklist: `C:\Projects\gunk-reference\PARITY_CHECKLIST.md` (the bar is Gunk's Platinum v1.0.2). Audited
2026-11-05 on build 13a21380, the first build with the reworked DexNav screen. Evidence names the test, screenshot
or `GUNK_CHANGES.md` row. Tests live in `emerald-tools/emu/suite`.

| Row | Item | Verdict | Evidence / what's missing |
|-----|------|---------|---------------------------|
| 1 | DexNav screen in Platinum's structure | **PARTIAL** | Title bar with area + CAUGHT x/y, info panel (name, status, Found, level range, search level, chain, types, abilities), search/footer panel (R target, A, B), LAND/WATER/HIDDEN grids with "?" unseen, ball mark if caught, cursor (`out/dn_registered.png`, `out/dn_screen.png`). Missing: seen-but-not-caught icons are in colour, not a silhouette |
| 1 | Searches on land and water | **PARTIAL** | Water grid and water searches exist in code (environment WATER, surf tiles); only the land search is emulator-tested (`dexnav_test.py`). Needs a surfing search test |
| 1 | Search levels, chains, better IVs/shinies/items | **HAS** | Search levels in PC storage free space, chain byte, potential stars and shiny rerolls by level/chain (row 19) |
| 1 | Rules (10 s, shake, chain) | **PARTIAL** | 10 s, 2 shakes/s, timeout and other-species resets, flee resets, win/catch adds one (`dexnav_rules_test.py`). To check: menus, Bag and battles must not count towards the 10 s |
| 1 | Shown shiny = given shiny; forms; wild items, Compound Eyes | **PARTIAL** | Peeked roll equals made Pokémon 60/60 (row 19). No map forms in Gen 3. To check: held-item rate with a Compound Eyes lead |
| 1 | No pause when a search starts | **HAS** | Rolls are peeked, the worst case (search level 255) starts in ≤ 1 s (was 5 s) |
| 1 | Every DexNav catch legal in PKHeX | **HAS** | Every DexNav Pokémon VALID as EncounterSlot3 (row 19, `legality_batch.py`) |
| 1 | Key item before the first gym, usable from the Bag, registrable | **PARTIAL** | Given with the Pokédex in Birch's lab (before the first gym), opened from the START menu. Missing: a DEXNAV key item in the Bag that can be used and registered |
| 1 | Only seen Pokémon can be chosen; fishing-only can't; hint says where to press R | **PARTIAL** | Only seen species can be registered or searched; fishing has no grid, so fishing-only species can't be chosen. The hint shows "R POOCHYENA" but not grass/surfing |
| 1 | Field search bar | **PARTIAL** | Shows species, level, a move, ability, held item, potential stars, chain. Missing: shiny mark, nature, search level |
| 1 | Clear messages when R can't search | **PARTIAL** | "couldn't be found nearby", "got away", "too dark", "signal was lost". To check: nothing registered, no grass/water near, search it by surfing instead |
| 1 | Second quick view | N/A | GBA has one screen |
| 2 | Every species follows, correct sprites | **HAS** | aarant's followers-legacy: all 386 Gen 3 species with Gen 3 overworld sprites (user approved) |
| 2 | Emotes and interactions | **HAS** | Follower talk: friendship lines, tired line, emotes (`follower_talk.py` 8/8, row 22) |
| 2 | Back to its ball while Surfing and where it can't follow | **PARTIAL** | aarant's engine hides it while surfing, diving and biking; not emulator-tested yet |
| 2 | No object-ID clash or full-slot crash | **HAS** | Audit: local ID 254 clashes with nothing; a failed spawn is skipped. 512-NPC talk sweep, nothing froze |
| 2 | Behaves in cutscenes, Poké Ball animation | **PARTIAL** | aarant's scripts hide it in some scenes; the story sweep (244/244) found no freezes. Not checked scene by scene |
| 2 | Double battle in the lead; scripted walk during a screen effect | **MISSING** (untested) | Needs a test |
| 3 | PC boxes for a living dex + spares | **HAS** | 16 boxes = 480 slots for 386 species (row 18, `box_test.py` 7/7) |
| 4 | Instant text | **HAS** | Row 3 |
| 4 | Faster EXP bar | **PARTIAL** | 2 px/frame in code; not timed in the emulator |
| 4 | Auto-run | **HAS** | Row 5 |
| 4 | Faster eggs | **HAS** | Row 17 |
| 5 | Quick Run | **HAS** | Row 21 |
| 5 | Quick Ball widget | **HAS** | Row 21 |
| 6 | Reusable TMs / two registered items / bag sorting / Repel prompt / Endless Repel | **HAS** | Rows 8, 9, 10, 6, 7 |
| 7 | IV/EV view, Exp. All, vitamins to 252, trade evos, Move Tutor | **HAS** | Rows 11, 12, 13, 14, 16 |
| 7 | HMs without teaching | **PARTIAL** | Party menu done; the in-world uses (walking up to a tree, boulder, water) not yet tested |
| 8 | Logo; header GUNK + short code | **HAS** | Row 1 and 1b (`GUNK EM`, BPEE) |
| 8 | BPS refuses wrong ROM, sha1s in notes, clean build reproduces | **HAS** | `release/gunks-emerald-v1.0.bps`, RELEASE_NOTES_v1.0.md |
| 8 | PokeDaisy address table | **MISSING** | Needs a table or a README note |
| 8 | Test hooks only in test builds | **HAS** | The shiny test build is a separate ROM; the release has no warps or test saves |
| 9 | Berry, Feebas spots, swarms info | **MISSING** | No in-game info yet |
| 10 | Save suite (saves, power cuts, damaged saves) | **HAS** | 4/4 every build. Full boxes, Hall of Fame: covered by `box_test.py` and the HoF layout check, not in the suite itself |
| 10 | Gyms, Elite Four, Champion to the Hall of Fame vs vanilla | **MISSING** | No full playthrough yet |
| 10 | Story and talk sweeps vs vanilla | **HAS** | 244/244 story NPCs, 512/512 talk, all differences benign |
| 10 | Menus test | **MISSING** | Needs a test |
| 10 | Release ROM boots, intro, saves, continues | **HAS** | New game from the title on the release build, save suite on it |
| 10 | README feature list, release notes | **PARTIAL** | Release notes exist; README feature list to check |
